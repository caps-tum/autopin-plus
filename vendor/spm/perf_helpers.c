#include <stdio.h>
#include <stdlib.h>
#include <linux/perf_event.h>
#include <string.h>
#include <unistd.h>

#include "spm.h"
//perf/util/util.h
#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })

static const char * const mem_lvl[] = {
	"N/A",
	"HIT",
	"MISS",
	"L1",
	"LFB",
	"L2",
	"L3",
	"Local RAM",
	"Remote RAM (1 hop)",
	"Remote RAM (2 hops)",
	"Remote Cache (1 hop)",
	"Remote Cache (2 hops)",
	"I/O",
	"Uncached",
};

#define NUM_MEM_LVL (sizeof(mem_lvl)/sizeof(const char *))

int filter_local_accesses(union perf_mem_data_src *entry){
	//This method is based on util/sort.c:hist_entry__lvl_snprintf
	u64 m =  PERF_MEM_LVL_NA;
	u64 hit, miss;
	//masks defined according to the declaration of mem_lvl
	u64 l1_mask= 0x1 << 3;
	u64 lfb_mask= 0x1 << 4;
	u64 l2_mask= 0x1 << 5;
	u64 l3_mask= 0x1 << 6;
	int init_remote=8;
	int end_remote=11;
	int i=0;
	
	
	if (entry->val)
		m  = entry->mem_lvl;
	
	hit = m & PERF_MEM_LVL_HIT;
	miss = m & PERF_MEM_LVL_MISS;	
	m &= ~(PERF_MEM_LVL_HIT|PERF_MEM_LVL_MISS); // bits 1 and 2 are cleared
	
	//L1, L2 and LFB accesses are discarded
	if( (l1_mask & m) || (l2_mask & m) || (lfb_mask & m)){
		return 1;
	}
	// L3 hits are filtered
	if( (l3_mask & m) && hit){
		return 1;
	}
	
	//l3 misses are filtered
	if( (l3_mask & m) && miss){
		return 0;
	}
	
	//Remote accesses are not filtered
	for (i=init_remote; i<=end_remote;  i++){
		if( (1<<i) & m )
			return 0;
	}
	//any other accesses are filtered but passed as different information
	return 2;
}

char* print_access_type(int entry)
{
	char* out;
	size_t sz = 500 - 1; /* -1 for null termination */
	size_t i, l = 0;
	unsigned int m =  PERF_MEM_LVL_NA;
	unsigned int hit, miss;
	out=malloc(500*sizeof(char));

	m  = (unsigned int)entry;

	out[0] = '\0';
	out[499]='\0'; 
	hit = m & PERF_MEM_LVL_HIT;
	miss = m & PERF_MEM_LVL_MISS;

	/* already taken care of */
	m &= ~(PERF_MEM_LVL_HIT|PERF_MEM_LVL_MISS);

	for (i = 0; m && i < NUM_MEM_LVL; i++, m >>= 1) {
		if (!(m & 0x1))
			continue;
		if (l) {
			strcat(out, " or ");
			l += 4;
		}
		strncat(out, mem_lvl[i], sz - l);
		l += strlen(mem_lvl[i]);
	}
	if (*out == '\0')
		strcpy(out, "N/A");
	if (hit)
		strncat(out, " hit", sz - l);
	if (miss)
		strncat(out, " miss", sz - l);

	return  out;
}


static void free_cpu_topo(struct cpu_topo *tp)
{
	u32 i;

	if (!tp)
		return;

	for (i = 0 ; i < tp->core_sib; i++)
		zfree(&tp->core_siblings[i]);

	for (i = 0 ; i < tp->thread_sib; i++)
		zfree(&tp->thread_siblings[i]);

	free(tp);
}


static int build_cpu_topo(struct cpu_topo *tp, int cpu)
{
	FILE *fp;
	char filename[MAXPATHLEN];
	char *buf = NULL, *p;
	size_t len = 0;
	ssize_t sret;
	u32 i = 0;
	int ret = -1;

	sprintf(filename, CORE_SIB_FMT, cpu);
	fp = fopen(filename, "r");
	if (!fp)
		goto try_threads;

	sret = getline(&buf, &len, fp);
	fclose(fp);
	if (sret <= 0)
		goto try_threads;

	p = strchr(buf, '\n');
	if (p)
		*p = '\0';

	for (i = 0; i < tp->core_sib; i++) {
		if (!strcmp(buf, tp->core_siblings[i]))
			break;
	}
	if (i == tp->core_sib) {
		tp->core_siblings[i] = buf;
		tp->core_sib++;
		buf = NULL;
		len = 0;
	}
	ret = 0;

try_threads:
	sprintf(filename, THRD_SIB_FMT, cpu);
	fp = fopen(filename, "r");
	if (!fp)
		goto done;

	if (getline(&buf, &len, fp) <= 0)
		goto done;

	p = strchr(buf, '\n');
	if (p)
		*p = '\0';

	for (i = 0; i < tp->thread_sib; i++) {
		if (!strcmp(buf, tp->thread_siblings[i]))
			break;
	}
	if (i == tp->thread_sib) {
		tp->thread_siblings[i] = buf;
		tp->thread_sib++;
		buf = NULL;
	}
	ret = 0;
done:
	if(fp)
		fclose(fp);
	free(buf);
	return ret;
}

 struct cpu_topo *build_cpu_topology(void)
{
	struct cpu_topo *tp;
	void *addr;
	u32 nr, i;
	size_t sz;
	long ncpus;
	int ret = -1;

	ncpus = sysconf(_SC_NPROCESSORS_CONF);
	if (ncpus < 0)
		return NULL;

	nr = (u32)(ncpus & UINT_MAX);

	sz = nr * sizeof(char *);

	addr = calloc(1, sizeof(*tp) + 2 * sz);
	if (!addr)
		return NULL;

	tp = addr;

	addr += sizeof(*tp);
	tp->core_siblings = addr;
	addr += sz;
	tp->thread_siblings = addr;

	for (i = 0; i < nr; i++) {
		ret = build_cpu_topo(tp, i);
		if (ret < 0)
			break;
	}
	if (ret) {
		free_cpu_topo(tp);
		tp = NULL;
	}
	return tp;
}
