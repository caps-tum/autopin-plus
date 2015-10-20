/*
 * launcher.c
 * 
 * Copyright 2015 Do <do@nux>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * untitled
 */

#include <inttypes.h>

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <linux/unistd.h>

#include "spm.h"



#define KERNEL_ADDR_START	0xffffffff80000000
#define INVALID_FD	-1
#define INVALID_CONFIG	(uint64_t)(-1)
#define PF_MAP_NPAGES_MAX           1024
#define PF_MAP_NPAGES_MIN           64
#define PF_MAP_NPAGES_NORMAL        256


#define rmb() asm volatile("lock; addl $0,0(%%esp)" ::: "memory")

typedef enum {
	PRECISE_NORMAL = 0,
	PRECISE_HIGH,
	PRECISE_LOW
} precise_type_t;



typedef enum {
	COUNT_INVALID = -1,
	COUNT_CORE_CLK = 0,
	COUNT_RMA,
	COUNT_CLK,
	COUNT_IR,
	COUNT_LMA
} count_id_t;

typedef struct _pf_profiling_rec {
	unsigned int pid;
	unsigned int tid;
	uint64_t period;
	count_value_t countval;
	unsigned int ip_num;
	uint64_t ips[IP_NUM];
} pf_profiling_rec_t;

typedef struct _pf_conf {
	count_id_t count_id;
	uint32_t type;
	uint64_t config;
	uint64_t config1;
	uint64_t sample_period;
} pf_conf_t;



struct prof_config{
	perf_cpu_t *cpus;
};

precise_type_t g_precise;
static int s_mapsize, s_mapmask;
int g_pagesize;
int end_s;

 void pagesize_init(void)
{
	g_pagesize = getpagesize();
}

int pf_ringsize_init(void)
{
	switch (g_precise) {
	case PRECISE_HIGH:
		s_mapsize = g_pagesize * (PF_MAP_NPAGES_MAX + 1);
		s_mapmask = (g_pagesize * PF_MAP_NPAGES_MAX) - 1;
		break;
		
	case PRECISE_LOW:
		s_mapsize = g_pagesize * (PF_MAP_NPAGES_MIN + 1);
		s_mapmask = (g_pagesize * PF_MAP_NPAGES_MIN) - 1;
		break;

	default:
		s_mapsize = g_pagesize * (PF_MAP_NPAGES_NORMAL + 1);
		s_mapmask = (g_pagesize * PF_MAP_NPAGES_NORMAL) - 1;
		break;	
	}

	return (s_mapsize - g_pagesize);
}


static void ll_recbuf_update(pf_ll_rec_t *rec_arr, int *nrec, pf_ll_rec_t *rec)
{
	int i;

	if ((rec->pid == 0) || (rec->tid == 0)) {
		/* Just consider the user-land process/thread. */
		return;	
	}

	/*
	 * The size of array is enough.
	 */
	i = *nrec;
	memcpy(&rec_arr[i], rec, sizeof (pf_ll_rec_t));
	
	if(*nrec == BUFFER_SIZE-1){
		*nrec=0;
	}else{
		*nrec += 1;
	}
	
}

 static void mmap_buffer_skip(struct perf_event_mmap_page *header, int size)
{
	int data_head;

	data_head = header->data_head;
	//rmb();

	if ((header->data_tail + size) > data_head) {
		size = data_head - header->data_tail;
	}

	header->data_tail += size;
}

static void
mmap_buffer_reset(struct perf_event_mmap_page *header)
{
	int data_head;

	data_head = header->data_head;;
	rmb();

	header->data_tail = data_head;
}

static int
mmap_buffer_read(struct perf_event_mmap_page *header, void *buf, size_t size)
{
	void *data;
	uint64_t data_head, data_tail;
	int data_size, ncopies;
	
	/*
	 * The first page is a meta-data page (struct perf_event_mmap_page),
	 * so move to the second page which contains the perf data.
	 */
	data = (void *)header + g_pagesize;

	/*
	 * data_tail points to the position where userspace last read,
	 * data_head points to the position where kernel last add.
	 * After read data_head value, need to issue a rmb(). 
	 */
	data_tail = header->data_tail;
	data_head = header->data_head;
	//DO: I had to comment the following because it generates a segmentation fault
	//asm volatile("lock; addl $0,0(%%esp)" ::: "memory");

	/*
	 * The kernel function "perf_output_space()" guarantees no data_head can
	 * wrap over the data_tail.
	 */
	if ((data_size = data_head - data_tail) < size) {
		return (-1);
	}

	data_tail &= s_mapmask;

	/*
	 * Need to consider if data_head is wrapped when copy data.
	 */
	if ((ncopies = (s_mapsize - data_tail)) < size) {
		memcpy(buf, data + data_tail, ncopies);
		memcpy(buf + ncopies, data, size - ncopies);
	} else {
		memcpy(buf, data + data_tail, size);
	}

	header->data_tail += size;	
	return (0);
}


static int
ll_sample_read(struct perf_event_mmap_page *mhdr, int size,
	pf_ll_rec_t *rec)
{
	struct { uint32_t pid, tid; } id;
	uint64_t addr, cpu, weight, nr, value, *ips,origin,time,ip,period;
	int i, j, ret = -1;
	union perf_mem_data_src dsrc;

	/*
	 * struct read_format {
	 *	{ u32	pid, tid; }
	 *	{ u64	addr; }
	 *	{ u64	cpu; }
	 *	[ u64	nr; }
	 *	{ u64   ips[nr]; }
	 *	{ u64	weight; }
	 * };
	 */
	
	if (mmap_buffer_read(mhdr, &ip, sizeof (ip)) == -1) {
		printf( "ll_sample_read: read ip failed.\n");
		goto L_EXIT;
	}
	size -= sizeof (ip);
	
	if (mmap_buffer_read(mhdr, &id, sizeof (id)) == -1) {
		printf("ll_sample_read: read pid/tid failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (id);
	
	if (mmap_buffer_read(mhdr, &time, sizeof (time)) == -1) {
		printf("ll_sample_read: read time failed.\n");
		goto L_EXIT;
	}
	
	size -= sizeof (time);
	if (mmap_buffer_read(mhdr, &addr, sizeof (addr)) == -1) {
		printf("ll_sample_read: read addr failed.\n");
		goto L_EXIT;
	}
	
	size -= sizeof (addr);

	if (mmap_buffer_read(mhdr, &cpu, sizeof (cpu)) == -1) {
		printf( "ll_sample_read: read cpu failed.\n");
		goto L_EXIT;
	}
	size -= sizeof (cpu);
	
	if (mmap_buffer_read(mhdr, &period, sizeof (period)) == -1) {
		printf( "ll_sample_read: read period failed.\n");
		goto L_EXIT;
	}
	size -= sizeof (period);
	
	if (mmap_buffer_read(mhdr, &weight, sizeof (weight)) == -1) {
		printf("ll_sample_read: read weight failed.\n");
		goto L_EXIT;
	}
	
	size -= sizeof (weight);

	
	if (mmap_buffer_read(mhdr, &dsrc, sizeof (dsrc)) == -1) {
		printf("ll_sample_read: read origin failed.\n");
		goto L_EXIT;
	}
	//printf("%d %s %lu %d %lu -",dsrc.mem_lvl, print_access_type(dsrc.mem_lvl),weight,id.pid, dsrc);
	
	size -= sizeof (dsrc);
	
	rec->ip_num = j;
	rec->pid = id.pid;
	rec->tid = id.tid;
	rec->addr = addr;
	rec->cpu = cpu;
	rec->latency = weight;
	rec->data_source=dsrc;	
	ret = 0;

L_EXIT:
	if (size > 0) {
		mmap_buffer_skip(mhdr, size);
		printf( "ll_sample_read: skip %d bytes, ret=%d\n",
			size, ret);
	}

	return (ret);
}

static int
pf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd,
	unsigned long flags)
{
	return (syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags));
}

void pf_resource_free(struct _perf_cpu *cpu)
{
	int i;

	for (i = 0; i < COUNT_NUM; i++) {
		if (cpu->fds[i] != INVALID_FD) {
			close(cpu->fds[i]);
			cpu->fds[i] = INVALID_FD;
		}
	}

	if (cpu->map_base != MAP_FAILED) {
		munmap(cpu->map_base, cpu->map_len);
		cpu->map_base = MAP_FAILED;
		cpu->map_len = 0;
	}
}

static void
profiling_recbuf_update(pf_profiling_rec_t *rec_arr, int *nrec,
	pf_profiling_rec_t *rec)
{
	int i;

	if ((rec->pid == 0) || (rec->tid == 0)) {
		/* Just consider the user-land process/thread. */
		return;	
	}

	/*
	 * The buffer of array is enough, don't need to consider overflow.
	 */
	i = *nrec;
	memcpy(&rec_arr[i], rec, sizeof (pf_profiling_rec_t));
	*nrec += 1;
}

static uint64_t
scale(uint64_t value, uint64_t time_enabled, uint64_t time_running)
{
	uint64_t res = 0;

	if (time_running > time_enabled) {
		printf( "time_running > time_enabled\n");
	}

	if (time_running) {
		res = (uint64_t)((double)value * (double)time_enabled / (double)time_running);
	}

	return (res);
}

static int
profiling_sample_read(struct perf_event_mmap_page *mhdr, int size,
	pf_profiling_rec_t *rec)
{
	struct { uint32_t pid, tid; } id;
	count_value_t *countval = &rec->countval;
	uint64_t time_enabled, time_running, nr, value, *ips;
	int i, j, ret = -1;

	/*
	 * struct read_format {
	 *	{ u32	pid, tid; }
	 *	{ u64	nr; }
	 *	{ u64	time_enabled; }
	 *	{ u64	time_running; }
	 *	{ u64	cntr[nr]; }
	 *	[ u64	nr; }
	 *	{ u64   ips[nr]; }
	 * };
	 */
	if (mmap_buffer_read(mhdr, &id, sizeof (id)) == -1) {
		printf( "profiling_sample_read: read pid/tid failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (id);

	if (mmap_buffer_read(mhdr, &nr, sizeof (nr)) == -1) {
		printf( "profiling_sample_read: read nr failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (nr);

	if (mmap_buffer_read(mhdr, &time_enabled, sizeof (time_enabled)) == -1) {
		printf("profiling_sample_read: read time_enabled failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (time_enabled);

	if (mmap_buffer_read(mhdr, &time_running, sizeof (time_running)) == -1) {
		printf( "profiling_sample_read: read time_running failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (time_running);

	 
	for (i = 0; i < nr; i++) {
		if (mmap_buffer_read(mhdr, &value, sizeof (value)) == -1) {
			printf( "profiling_sample_read: read value failed.\n");
			goto L_EXIT;
		}

		size -= sizeof (value);

		/*
		 * Prevent the inconsistent results if share the PMU with other users
		 * who multiplex globally.
		 */
		value = scale(value, time_enabled, time_running);
		countval->counts[i] = value;
		printf("v %lu ", value);
	}
	printf("\n");

	if (mmap_buffer_read(mhdr, &nr, sizeof (nr)) == -1) {
		printf( "profiling_sample_read: read nr failed.\n");
		goto L_EXIT;
	}

	size -= sizeof (nr);

	j = 0;
	ips = rec->ips;
	for (i = 0; i < nr; i++) {
		if (j >= IP_NUM) {
			break;
		}

		if (mmap_buffer_read(mhdr, &value, sizeof (value)) == -1) {
			printf( "profiling_sample_read: read value failed.\n");
			return (-1);
		}

		size -= sizeof (value);
		
		if (value < KERNEL_ADDR_START) {
			/*
			 * Only save the user-space address.
			 */
			ips[j] = value;
			j++;
		}
	}

	rec->ip_num = j;
	rec->pid = id.pid;
	rec->tid = id.tid;
	ret = 0;
	
L_EXIT:	
	if (size > 0) {
		mmap_buffer_skip(mhdr, size);
		printf("profiling_sample_read: skip %d bytes, ret=%d\n",
			size, ret);
	}

	return (ret);
}

void pf_profiling_record(struct _perf_cpu *cpu, pf_profiling_rec_t *rec_arr,
	int *nrec)
{
	struct perf_event_mmap_page *mhdr = cpu->map_base;
	struct perf_event_header ehdr;
	pf_profiling_rec_t rec;
	int size;

	if (nrec != NULL) {
		*nrec = 0;
	}

	for (;;) {
		if (mmap_buffer_read(mhdr, &ehdr, sizeof(ehdr)) == -1) {
   	    	return;
 		}

		if ((size = ehdr.size - sizeof (ehdr)) <= 0) {			
			mmap_buffer_reset(mhdr);
			return;
		}

		if ((ehdr.type == PERF_RECORD_SAMPLE) && (rec_arr != NULL)) {
			printf("-  %d cpu --",cpu->cpuid );
			if (profiling_sample_read(mhdr, size, &rec) == 0) {
				profiling_recbuf_update(rec_arr, nrec, &rec);
			} else {
				/* No valid record in ring buffer. */
				return;	
			}
		} else {
			mmap_buffer_skip(mhdr, size);
		}
	}
}

int pf_profiling_start(struct _perf_cpu *cpu, count_id_t count_id)
{
	if (cpu->fds[count_id] != INVALID_FD) {
		return (ioctl(cpu->fds[count_id], PERF_EVENT_IOC_ENABLE, 0));
	}
	
	return (0);
}

int
pf_profiling_stop(struct _perf_cpu *cpu, count_id_t count_id)
{
	if (cpu->fds[count_id] != INVALID_FD) {
		return (ioctl(cpu->fds[count_id], PERF_EVENT_IOC_DISABLE, 0));
	}
	
	return (0);
}

int pf_ll_setup(struct _perf_cpu *cpu, struct sampling_settings *ss )
{
	struct perf_event_attr attr;
	int *fds = cpu->fds;

	memset(&attr, 0, sizeof (attr));
	attr.type = 4;
	attr.config = 461;
	attr.config1 = 148;
	attr.sample_period = ss->ll_sampling_period;
	attr.precise_ip = 1;
	attr.exclude_guest = 1;
	attr.sample_type = PERF_SAMPLE_IP |PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_ADDR | PERF_SAMPLE_CPU |
		PERF_SAMPLE_PERIOD | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC  ; 
	attr.disabled = 1;

	if ((fds[0] = pf_event_open(&attr, -1, cpu->cpuid, -1, 0)) < 0) {
		printf("pf_ll_setup: pf_event_open is failed for CPU%d\n", cpu->cpuid);
		fds[0] = INVALID_FD;
		return (-1);
	}
	
	if ((cpu->map_base = mmap(NULL, s_mapsize, PROT_READ | PROT_WRITE,
		MAP_SHARED, fds[0], 0)) == MAP_FAILED) {
		close(fds[0]);
		fds[0] = INVALID_FD;
		return (-1);
	}

	cpu->map_len = s_mapsize;
	cpu->map_mask = s_mapmask;
	return (0);
}


void
pf_ll_record(struct _perf_cpu *cpu, pf_ll_rec_t *rec_arr, int *nrec)
{
	struct perf_event_mmap_page *mhdr = cpu->map_base;
	struct perf_event_header ehdr;
	pf_ll_rec_t rec;
	int size;

	if (nrec == NULL) {
		printf("erroneous reference to record position");
	}

	for (;;) {
		if (mmap_buffer_read(mhdr, &ehdr, sizeof(ehdr)) == -1) {
			/* No valid record in ring buffer. */
   	    	return;
 		}
	
		if ((size = ehdr.size - sizeof (ehdr)) <= 0) {
			return;
		}

		if ((ehdr.type == PERF_RECORD_SAMPLE) && (rec_arr != NULL)) {
			if (ll_sample_read(mhdr, size, &rec) == 0) {
				ll_recbuf_update(rec_arr, nrec, &rec);
			} else {
				/* No valid record in ring buffer. */
				return;	
			}
		} else {
			mmap_buffer_skip(mhdr, size);
		}
		//printf("read sample %d value %d ",*nrec,rec_arr[*nrec].latency, ehdr.size);
	}
}

int
pf_ll_start(struct _perf_cpu *cpu)
{
	if (cpu->fds[0] != INVALID_FD) {
		return (ioctl(cpu->fds[0], PERF_EVENT_IOC_ENABLE, 0));
	}
	
	return (0);
}

int
pf_ll_stop(struct _perf_cpu *cpu)
{
	if (cpu->fds[0] != INVALID_FD) {
		return (ioctl(cpu->fds[0], PERF_EVENT_IOC_DISABLE, 0));
	}
	
	return (0);
}
	
static void
cpu_init(perf_cpu_t *cpu)
{
	int i;

	for (i = 0; i < COUNT_NUM; i++) {
		cpu->fds[i] = INVALID_FD;	
	}

	cpu->map_base = MAP_FAILED;
}


int pf_profiling_setup(struct _perf_cpu *cpu, int idx, pf_conf_t *conf)
{
	struct perf_event_attr attr;
	int *fds = cpu->fds;
	int group_fd;

	memset(&attr, 0, sizeof (attr));
	attr.type = conf->type;	
	attr.config = conf->config;
	attr.config1 = conf->config1;
	attr.sample_period = conf->sample_period;
	attr.sample_type = PERF_SAMPLE_TID | PERF_SAMPLE_READ |
		PERF_SAMPLE_CALLCHAIN;
	attr.read_format = PERF_FORMAT_GROUP |
		PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

	if (idx == 0) {
		attr.disabled = 1;
		group_fd = -1;
	} else {
		group_fd = fds[0];;
	}

	if ((fds[idx] = pf_event_open(&attr, -1, cpu->cpuid, group_fd, 0)) < 0) {
		printf( "pf_profiling_setup: pf_event_open is failed "
			"for CPU%d, COUNT %d\n", cpu->cpuid, idx);
		fds[idx] = INVALID_FD;
		return (-1);
	}
	
	if (idx == 0) {
		if ((cpu->map_base = mmap(NULL, s_mapsize, PROT_READ | PROT_WRITE,
			MAP_SHARED, fds[0], 0)) == MAP_FAILED) {
			close(fds[0]);
			fds[0] = INVALID_FD;
			return (-1);	
		}

		cpu->map_len = s_mapsize;
		cpu->map_mask = s_mapmask;
	} else {
        if (ioctl(fds[idx], PERF_EVENT_IOC_SET_OUTPUT, fds[0]) != 0) {
			printf( "pf_profiling_setup: "
				"PERF_EVENT_IOC_SET_OUTPUT is failed for CPU%d, COUNT%d\n",
				cpu->cpuid, idx);
			close(fds[idx]);
			fds[idx] = INVALID_FD;
			return (-1);
		}
	}

	return (0);
}


static int
cpu_profiling_setup(perf_cpu_t *cpu, void *arg)
{
	
	pf_conf_t evt1={ .type=PERF_TYPE_RAW,.config= 0x5301B7,  .config1=0x67f800001, .count_id= COUNT_RMA, .sample_period=10000};
	pf_conf_t evt2={ .type=PERF_TYPE_RAW,.config= 0x5301BB,  .config1=0x600400001, .count_id= COUNT_LMA, .sample_period=10000};
	pf_conf_t conf_arr[2]={evt1,evt2} ;
	int i, ret = 0;

	cpu_init(cpu);
	for (i = 0; i < COUNT_NUM; i++) {
		if (conf_arr[i].config == INVALID_CONFIG) {
			/*
			 * Invalid config is at the end of array.
			 */
			 printf("init err 1\n");
			break;
		}

		if (pf_profiling_setup(cpu, i, &conf_arr[i]) != 0) {
			ret = -1;
			printf("init err \n");
			break;
		}

	}

	if (ret != 0) {
		pf_resource_free(cpu);
	}

	return (ret);
}



//TODO return values
int setup_sampling(struct sampling_settings *ss){
	for(int i=0; i<ss->n_cores; i++){
		memset((ss->cpus_ll+i),0,sizeof(perf_cpu_t));
		memset((ss->cpus_pf+i),0,sizeof(perf_cpu_t));
		ss->cpus_ll[i].cpuid=i;
		ss->cpus_pf[i].cpuid=i;
		cpu_profiling_setup(ss->cpus_pf+i,NULL);
		pf_ll_setup(ss->cpus_ll+i,ss);		
	}
	
	return  0;
}

//TODO return values
int start_sampling(struct sampling_settings *ss){
	for(int i=0; i<ss->n_cores; i++){
		pf_profiling_start((ss->cpus_pf+i),0);
		pf_profiling_start((ss->cpus_pf+i),1);
		pf_ll_start((ss->cpus_ll+i));
		
	}
	return 0;
}	

int stop_sampling(struct sampling_settings *ss){
	for(int i=0; i<ss->n_cores; i++){
		pf_profiling_stop((ss->cpus_pf+i),0);
		pf_profiling_stop((ss->cpus_pf+i),1);
		pf_ll_stop((ss->cpus_ll+i));
		
	}
	return 0;
	
}
	


void consume_sample(struct sampling_settings *st,  pf_ll_rec_t *record, int current){
	
	int core=record[current].cpu;
	if(record[current].cpu <0 || record[current].cpu >= st->n_cores){
		return;
	}
	//TODO counter with mismatching number of cpus
	
	st->metrics.total_samples++;
	//TODO also get samples from the sampling process, detect high overhead
	if(record[current].pid != st->pid_uo){
		return; }
	
	st->metrics.process_samples[core]++;
	int access_type= filter_local_accesses(&(record[current].data_source));
	//TODO this depends on the page size
	u64 mask=0xFFF;
	u64 page_sampled=record[current].addr & ~mask ;
	
	add_mem_access( st, (void*)page_sampled, core);
	add_lvl_access( st, &(record[current].data_source),record[current].latency );
	if(!access_type){
		st->metrics.remote_samples[core]++;
		add_page_2move(st,page_sampled );
	}
}

//TODO return values
int read_samples(struct sampling_settings *ss, pf_ll_rec_t *record){
	int nrec=0,nrec2=0;
	int bef=0, wr_diff=0,current;
	pf_profiling_rec_t* record_pf=malloc(sizeof(pf_profiling_rec_t)*1000);
	for(;;){
	readagain:	for(int i=0; i<ss->n_cores; i++){
			bef=nrec;
			pf_ll_record((ss->cpus_ll+i),record,&nrec);
			//pf_profiling_record((ss->cpus_pf+i),record_pf,&nrec2);
			wr_diff=nrec >= bef ? nrec > bef : nrec+BUFFER_SIZE-bef;
			
			while(wr_diff>0){
				current=nrec-wr_diff;
				current = current < 0 ? BUFFER_SIZE+current : current;
				//here we consume the sample
				//printf("%lu %d %d *", (record + current)->latency, nrec, current);
				consume_sample(ss,record,current);
				current=current != BUFFER_SIZE-1 ? current++ : 0;
				wr_diff--;
			}
			if(ss->end_recording)
				return 0;
		}
	}
	
	return 0;
}


void* controlsamp(void *arg){
	printf("control on \n");
	sleep(20);
	end_s=1;
	printf("end of measurement \n");
}
void init_globals(){
	pagesize_init();
	g_precise=PRECISE_HIGH;
	pf_ringsize_init();
}
	


