
	//the home proc c#include "sort.h"
#include <stdio.h>
#include "hist.h"
#include "symbol.h"
#include "numa_metrics.h"

#include <numaif.h>
#include <time.h>
#include <signal.h>
#include <omp.h>
#include <uthash.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "evlist.h"


int* get_cpu_interval(int max_cores, char* siblings ){
	int* sibling_array,i;
	if(max_cores < 1) return NULL;
	char *interval1,*interval2;
	char *savep1,*savep2;
	char* tok=strtok_r(siblings, ",",&savep1),*tokcpy;
	int high, low;
	
	sibling_array=malloc(sizeof(int)*max_cores);
	
	//initialize array
	
	for (i=0; i<max_cores; i++) sibling_array[i]=-1;
		
	
	if(tok==NULL) return 0;
	do{
		tokcpy=(char*)malloc(sizeof(char)*strlen(tok));
		strcpy(tokcpy,tok);
		//process subtoken, that must be in the form #-#
		interval1=strtok_r(tokcpy,"-",&savep2);
		if(interval1!=NULL){
			interval2=strtok_r(NULL,"-",&savep2);
			if(interval2!=NULL){
			//process interval	
			low=atoi(interval1);
			high=atoi(interval2);
			if(low<high && high>0 && low>=0 && high<max_cores && low < max_cores && high < max_cores){
				for(i=low; i<=high; i++) 
				sibling_array[i]=1;
			}
			
			}
		} 
		tok=strtok_r(NULL, ",",&savep1);
	}while(tok !=NULL);
	
	return sibling_array;
}

double wtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, 0);

  return tv.tv_sec+1e-6*tv.tv_usec;
}

int freq_sort(struct freq_stats *a, struct freq_stats *b) {
    return a->freq - b->freq;
}

void add_expensive_access(struct numa_metrics *nm,u64 addr){
	struct l3_addr *new_entry;
	
	new_entry=malloc(sizeof(struct l3_addr));
	memset(new_entry,0,sizeof(struct l3_addr));
	new_entry->page_addr=(void *)addr;
	
	if(nm->expensive_accesses){
		new_entry->next=nm->expensive_accesses;
	}
	
	//nm->expensive_accesses++;
	nm->expensive_accesses=new_entry;
}

void add_page_2move(struct numa_metrics *nm,u64 addr){
	struct l3_addr *new_entry;
	
	new_entry=malloc(sizeof(struct l3_addr));
	memset(new_entry,0,sizeof(struct l3_addr));
	new_entry->page_addr=(void *)addr;
	
	if(nm->pages_2move){
		new_entry->next=nm->pages_2move;
	}
	
	nm->number_pages2move++;
	nm->pages_2move=new_entry;
	
}

void print_migration_statistics(struct numa_metrics *nm){
	struct page_stats *current,*tmp;
	struct freq_stats *crr,*tmpf;
	//TODO number cpus
	int i=0, total_accesses=0, remote_accesses=0,freq;
	float rem2loc =0;
	char lbl[30];
	
	for(i=0; i<32; i++){
		total_accesses+= nm->process_accesses[i];
		remote_accesses+= nm->remote_accesses[i];
	}
	total_accesses= total_accesses==0 ? 1 : total_accesses;
	rem2loc=(100*(float)remote_accesses/(float)total_accesses); 
	sprintf(lbl,"mig-rpt.txt");
	
	if(nm->file_label && strlen(nm->file_label)>0)
		sprintf(lbl,"%s",nm->file_label);
	
	print_info(nm->report,"\n\n\n\t\t MIGRATION STATISTICS \n");
	
	print_info(nm->report,"%s: sampled accesses: %d\n %s: remote accesses: %d \n %s pzn remote %f \n\n\n", 
		lbl,total_accesses, lbl, remote_accesses,lbl,rem2loc);
	print_info(nm->report,"%s moved pages: %d \n",lbl,nm->moved_pages);
	
	for(i=0; i<32; i++){
		print_info(nm->report,"%s: CPU- %d	sampled- %d \t remote- %d \n",lbl,i, nm->process_accesses[i], nm->remote_accesses[i]);
	}
	
	
	sort_entries(nm);
	HASH_ITER(hh, nm->page_accesses, current, tmp) {
		freq=current->proc0_acceses + current->proc1_acceses;
		add_freq_access(nm,freq);
		if(nm->report)	
			fprintf(nm->report," Accessed %p count %d %d \n",current->page_addr, current->proc0_acceses,current->proc1_acceses);
			
	}
	//sort the frequencies
	HASH_SORT( nm->freq_accesses, freq_sort );
	
	HASH_ITER(hh, nm->freq_accesses, crr, tmpf) {
		print_info(nm->report,"%s:pages accessed:%d:%d \n",lbl,crr->freq,crr->count);
	}
	
}
void do_great_migration(struct numa_metrics *nm){
	struct l3_addr *current=nm->pages_2move;
	void **pages;
	int ret,count=0,*nodes,*status,i,succesfully_moved=0,destination_node=0,greatest_count=0;
	double tinit=0, tfin=0;
	struct page_stats *sear=NULL;

	
	pages=malloc(sizeof(void*) * nm->number_pages2move);
	status=malloc(sizeof(int) * nm->number_pages2move);
	nodes=malloc(sizeof(int) * nm->number_pages2move);
	//consolidates the page addresses into a single page address package
	while(current){
	//	printf("%p \n", current->page_addr);
		HASH_FIND_PTR( nm->page_accesses,&(current->page_addr),sear );
		if(!sear){
			printf("cannot find entry %lux,this should not happen \n",current->page_addr);
			current=current->next;
			continue;
		}
		//The destination is the node with the greatest number of accesses
		for(i=0; i<nm->n_cpus; i++){
			if(sear->proc_accesses[i]>greatest_count){
					greatest_count=sear->proc_accesses[i];
					destination_node=i;
			}
		}
		*(pages+count)=(void *)current->page_addr;
		current=current->next;	
		count++;

		*(nodes+count)=destination_node;
	}
	
	if(nm->migrate_chunk_size >0 )
		count=nm->migrate_chunk_size;
		
	tinit=wtime();
	ret= 	move_pages(nm->pid_uo, count, pages, nodes, status,0);
	
	if (ret!=0){
		printf("move_pages returned an error %d",errno);
	}
	
	//check the new home of the pages
	ret= 	move_pages(nm->pid_uo, count, pages, NULL, status,0);
	
	for(i=0; i<count;i++){
			if(status[i] >=0 && status[i]<nm->n_cpus)
			succesfully_moved++;		
		}
	
	tfin=wtime()-tinit;
	
	nm->moved_pages=count;
	printf("%d pages moved successfully, move pages time %f \n",succesfully_moved,tfin);
	//TODO take move decission 
}

int do_migration(struct numa_metrics *nm, int pid, struct perf_sample *sample){
	
	u64 mask,addr;
	int count,*nodes,node,calling_cpu,home_count, remote_count;
	void *page, **pages;
	int* status,st;
	long ret;
	struct page_stats *sear=NULL,*hashtable;
	union perf_mem_data_src data_src; 

	
	st=-1;
	
	data_src.val=sample->data_src;
	
	if (!data_src.val)
		return -1;

		
	//Assuming page size is 4K the 12 LSBs are discharged to 
	//obtain the page number
	//TODO page number must be parametrized
	mask= 0xFFF; // 12 bytes
	addr=sample->addr;
	addr= addr & ~mask ;
	count=1;
	node=0;
	nodes=NULL;
	page= (void *) addr;
	pages=&page;
	status=&st;
	
	hashtable=nm->page_accesses;
	HASH_FIND_PTR( hashtable,&page,sear );
	if(!sear) return 0;
	
	if(nm->logging_detail_level > 3)
		printf("Hash table lookup %p %d %d **\n", sear->page_addr ,sear->proc0_acceses,sear->proc1_acceses );
	
	ret= move_pages(pid, count, pages, nodes, status,0);
	
	//only if the pages query is successful and the home node is different than the node that queries the address the page is migrated
	if (ret != 0){
		if(nm->logging_detail_level > 3)	
		printf ("Error on query\n");
		return 0;
		
	}
	//an only be different to the requesting proc and this home proc must be 0 or 1
	calling_cpu=nm->cpu_to_processor[sample->cpu];
	if ( calling_cpu== st ){
		if(nm->logging_detail_level > 3)	
		printf ("Page is already at requesting node (L3 hit) %d \n",status[0]);
		return 0;
		
	}
	if ( st < 0 || st>1  ){
		if(nm->logging_detail_level > 3)	
		printf ("Problematic status %d \n",status[0]);
		return 0;
		
	}
		
	if(nm->logging_detail_level > 3)	
		printf ("Move candidate:  home proc %d - requesting proc %d (core %d)\n ",
	status[0], nm->cpu_to_processor[sample->cpu],sample->cpu);		
	//the page is only moved if the number of accesses from the calling processor is greater than on the home
	home_count= status[0]==0 ? sear->proc0_acceses : sear->proc1_acceses;
	remote_count= status[0]==0 ? sear->proc1_acceses : sear->proc0_acceses;
	
	if(	remote_count <= home_count) {
		if(nm->logging_detail_level > 3)	
		printf ("Moving criteria not met\n");
		return;
	}
	
	//determine the new home processor
	node = nm->cpu_to_processor[sample->cpu]; 
	nodes=&node;
	
	//we want to move the page to its new home
	ret= 	move_pages(pid, count, pages, nodes, status,0);
	
	if (ret!=0){
		printf("error moving page \n");
		return;
	}
	
	//we will query again for the home of the page
	nodes=NULL;
	ret= move_pages(pid, count, pages, nodes, status,0);
		if(nm->logging_detail_level > 3)
		printf ("Page %p moved new home %d\n ",page,status[0] );	
	if(ret==0 && status[0] >=0)
		nm->moved_pages++;
	return ret;
	
}

//used by numa-an to determine the type of access

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



void init_processor_mapping(struct numa_metrics *nm, struct cpu_topo *topol){

	int i,j,*siblings;
	int max_cores=0;;
	int *core_to_node;

	//the new part begins here
	max_cores=numa_num_configured_cpus();
	core_to_node=malloc(sizeof(int)*max_cores);
	for(j=0; j<max_cores; j++){
		*(core_to_node+j)=-1;
	}
	
	for(i=0; i< nm->n_cpus;i++){
		siblings=get_cpu_interval(max_cores, topol->core_siblings[i]);
		if(!siblings) continue;
		for(j=0; j<max_cores; j++)
			*(core_to_node+j) = siblings[j]==1 ? i : *(core_to_node+j) ; 
			free(siblings);
	}
	
	for(int i=0; i<max_cores;i++) 
	nm->cpu_to_processor[i]=core_to_node[i];
	
	free(core_to_node);
}

void add_lvl_access( struct numa_metrics *multiproc_info, union perf_mem_data_src *entry, int weight ){
	struct access_stats *current=NULL;
	int key=(int)entry->mem_lvl;
	int weight_bucket=weight / WEIGHT_BUCKET_INTERVAL;
	
	HASH_FIND_INT(multiproc_info->lvl_accesses, &key,current);
	weight_bucket=weight_bucket > (WEIGHT_BUCKETS_NR-1) ? WEIGHT_BUCKETS_NR-1 : weight_bucket;
	
	multiproc_info->access_by_weight[weight_bucket]++;
	if(!current){
		current=malloc(sizeof(struct access_stats));
		current->count=0;
		current->mem_lvl=(int)entry->mem_lvl;
		HASH_ADD_INT(multiproc_info->lvl_accesses,mem_lvl,current);
	}
	
	current->count++;
	
}

void add_freq_access(struct numa_metrics *nm, int frequency){
	struct freq_stats *current=NULL; 
	HASH_FIND_INT(nm->freq_accesses, &frequency,current);
	
	if(!current){
			current=malloc(sizeof(struct freq_stats));
			memset(current,0,sizeof(struct freq_stats));
			current->freq=frequency;
			HASH_ADD_INT( nm->freq_accesses, freq, current );
	}
	
	current->count++;
	
	
}

void add_mem_access( struct numa_metrics *multiproc_info, void *page_addr, int accessing_cpu){
	struct page_stats *current=NULL; 
	struct page_stats *cursor=NULL; 
	int proc;

	//search the node for apparances

	HASH_FIND_PTR(multiproc_info->page_accesses, &page_addr,current);
	proc=multiproc_info->cpu_to_processor[accessing_cpu];
	if (current==NULL){
		current=malloc(sizeof(struct page_stats));
		current->proc0_acceses=0;
		current->proc1_acceses=0;
		current->proc_accesses=malloc(sizeof(int)*multiproc_info->n_cpus);
		memset(current->proc_accesses,0,sizeof(int)*multiproc_info->n_cpus);
		
		current->page_addr=page_addr;
		
		HASH_ADD_PTR(multiproc_info->page_accesses,page_addr,current);
		//printf ("add %d \n",HASH_COUNT(multiproc_info->page_accesses));
		
		for(cursor=multiproc_info->page_accesses; cursor != NULL; cursor=cursor->hh.next) {
        //printf("%p\n", cursor->page_addr);
		}
	}
	
	/* With the new processor mapping this part should be removed*/
	if (proc==0){
		current->proc0_acceses++;
	}else if(proc==1){
		current->proc1_acceses++;
	}
	
	/*Here begins the new page access bookkeeping*/
	current->proc_accesses[proc]++;
}

void print_access_info(struct numa_metrics *multiproc_info ){
	struct access_stats *current=NULL,*tmp;
	FILE *file=multiproc_info->report;
	int i=0,ubound,lbound;
	char * lbl;
	
	lbl= !multiproc_info->file_label || strlen(multiproc_info->file_label)<1 ? " " : multiproc_info->file_label;  
	
	HASH_ITER(hh, multiproc_info->lvl_accesses, current, tmp) {
		print_info(multiproc_info->report,"%s:LEVEL %d count %d %s \n", lbl, current->mem_lvl, current->count,print_access_type(current->mem_lvl) );
	}
	print_info(multiproc_info->report, "\n\n\n Access breakdown by weight \n\n");
	
	for(i=0; i<WEIGHT_BUCKETS_NR; i++){
		lbound=i*WEIGHT_BUCKET_INTERVAL;
		ubound=(i+1)*WEIGHT_BUCKET_INTERVAL;
		print_info(multiproc_info->report,"%s :%d-%d: %d\n", lbl,lbound,ubound, multiproc_info->access_by_weight[i]);
	}
	
}

//This method is based on util/sort.c:hist_entry__lvl_snprintf
char* print_access_type(int entry)
{
	char out[500];
	size_t sz = 500 - 1; /* -1 for null termination */
	size_t i, l = 0;
	u64 m =  PERF_MEM_LVL_NA;
	u64 hit, miss;


	m  = (u64)entry;

	out[0] = '\0';

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

long id_sort(struct page_stats *a, struct page_stats *b) {
    long rst= (long )a->page_addr - (long)b->page_addr;
    return rst;
}

void sort_entries(struct numa_metrics *nm){
	printf("\n Sorting page access entries ... \n");
	HASH_SORT( nm->page_accesses, id_sort );
}

void init_report_file(struct numa_metrics *nm){
	time_t ctime;
	int label=0;
	FILE *file;
	char fname[30];
	
	time(&ctime);
	label=(int)ctime%10000;
	sprintf(fname,"mig-rpt-%d.txt",label);
	
	if(nm->file_label && strlen(nm->file_label)>0)
		sprintf(fname,"%s.txt",nm->file_label);
		
	file = fopen(fname, "w");
	
	if (file == NULL) {
		fprintf(stderr, "Can't open output file %s!\n", fname);
		return;
	}
	fprintf(stdout,"Will output report to %s \n", fname);
	nm->report=file;
	nm->report_filename=fname;	

}

void close_report_file(struct numa_metrics *nm){
	if(nm->report){
		fclose(nm->report);
	}
}

//makes sure that the list of arguments ends with a null pointer
char ** put_end_params(char **argv,int argc){
	char ** list_args=malloc(sizeof(char*)*(argc+1));
	int i;
	
	for(i=0; i<argc; i++){
		*(list_args+i)=*(argv+i);
	}
	
	*(list_args+argc)=NULL;
	
	return list_args;
}

int launch_command( char** argv, int argc){
	int pid,ret=0;
	char ** args;
	if(argc< 1 || !argv[0])
		return;
	args=put_end_params(argv,argc);
	if ((pid = fork()) == 0){
  
           setenv("OMP_NUM_THREADS","2",0);
           setenv("GOMP_CPU_AFFINITY", "7,14",1);
           system("sleep 0.5");

           execv(argv[0],args);
          
          printf ("\n Child has ended execution \n");
           _exit(2);
           
	   }
    else{
           printf("Command launched with pid %d \n",pid);
           return pid;
	}
	
}

void print_info(FILE* file, const char* format, ...){
	va_list a_list;
	va_start( a_list, format );
		if(file!=NULL)
			vfprintf(file,format,a_list);
		
	va_end(a_list);
	
	va_start( a_list, format );

		vprintf(format,a_list);
		
	va_end(a_list);

}

char* get_command_string(const char ** argv, int argc){
	int ii=0,length=0,size=0;
	char* str;
	
	
	for( ii=0; ii<argc;ii++){
		length+=strlen(argv[ii]);
	}
	size=sizeof(char)*length+argc+1;
	str=malloc(size);
	memset(str,0,size);
	for( ii=0; ii<argc;ii++){
		strcat(str,argv[ii]);
		strcat(str," ");
	}
	
	return str;
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
