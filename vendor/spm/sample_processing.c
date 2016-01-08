
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <numaif.h>
#include <unistd.h>
#include <numa.h>
#include <time.h>
#include <sys/time.h>
#include "spm.h"
#include <errno.h>


/** @brief gets the time
 **/
double wtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, 0);

  return tv.tv_sec+1e-6*tv.tv_usec;
}

/** @brief adds a memory access from a sample to the hash table of accesses
 * @param ss Overal configuration of the sampling and container of the metrics
 * @param page_addr Memory location where the access was made
 * @param accessing_cpu id of the core that made the access
 */
 
void add_mem_access( struct sampling_settings *ss, void *page_addr, int accessing_cpu){
	struct page_stats *current=NULL; 
	int proc;

	//search the node for apparances

	HASH_FIND_PTR(ss->metrics.page_accesses, &page_addr,current);
	proc=ss->core_to_cpu[accessing_cpu];
	if (current==NULL){
		current=malloc(sizeof(struct page_stats));
		//one position created for every numa node
		current->proc_accesses=malloc(sizeof(int)*ss->n_cpus);
		memset(current->proc_accesses,0,sizeof(int)*ss->n_cpus);
		
		current->page_addr=page_addr;
		
		HASH_ADD_PTR(ss->metrics.page_accesses,page_addr,current);
		current->last_access=accessing_cpu;
		
	}else{
		//The page is being calling from other node as in the last time
		if(accessing_cpu != current->last_access){
			current->home_flips++;
			current->last_access=accessing_cpu;
		}
	}

	/*Here begins the new page access bookkeeping*/
	current->proc_accesses[proc]++;


}


/** @brief adds a level access to the count of accesses that were made to that level, useful for statistics
 * @param ss Overal configuration of the sampling and container of the metrics
 * @param entry Data adress where the access was made
 * @param weight Cost of the access, as reported by the sample
 */
 
void add_lvl_access( struct sampling_settings *ss, union perf_mem_data_src *entry, int weight ){
	struct access_stats *current=NULL;
	int key=(int)entry->mem_lvl;
	int weight_bucket=weight / WEIGHT_BUCKET_INTERVAL;
	
	HASH_FIND_INT(ss->metrics.lvl_accesses, &key,current);
	weight_bucket=weight_bucket > (WEIGHT_BUCKETS_NR-1) ? WEIGHT_BUCKETS_NR-1 : weight_bucket;
	
	ss->metrics.access_by_weight[weight_bucket]++;
	if(!current){
		current=malloc(sizeof(struct access_stats));
		current->count=0;
		current->mem_lvl=(int)entry->mem_lvl;
		HASH_ADD_INT(ss->metrics.lvl_accesses,mem_lvl,current);
	}
	
	current->count++;
	
}


/** @brief makes sure that the list of arguments ends with a null pointer
 * @param argv array of char*
 * @param argc number of arguments present
 */
 
char ** put_end_params(const char **argv,int argc){
	char ** list_args=malloc(sizeof(char*)*(argc+1));
	int i;
	
	for(i=0; i<argc; i++){
		*(list_args+i)=(char*)*(argv+i);
	}
	
	*(list_args+argc)=NULL;
	
	return list_args;
}

/** @brief makes sure that the list of arguments ends with a null pointer
 * @param argv array of char*
 * @param argc number of arguments present
 */
 
int launch_command( const char** argv, int argc){
	int pid;
	char ** args;
	if(argc< 1 || !argv[0])
		return -1;
	args=put_end_params(argv,argc);
	if ((pid = fork()) == 0){
		execv(argv[0],args);
		printf ("MIG-CTRL> \n Child has ended execution \n");
		_exit(2);  
	   }
     else{
           printf("MIG-CTRL> Command launched with pid %d \n",pid);
           return pid;
	}
	
}
/**
 * @brief sorting function
 **/ 
static int freq_sort(struct freq_stats *a, struct freq_stats *b) {
    return a->freq - b->freq;
}

/**
 * @brief sorting function
 **/ 
long id_sort(struct page_stats *a, struct page_stats *b) {
    long rst= (long )a->page_addr - (long)b->page_addr;
    return rst;
}

/** @brief Adds a page to the list of pages to be moved
 * @param ss global configuration of the sampling
 * @param addr Address of the page to add to rhe list
 */
void add_page_2move(struct sampling_settings *ss, u64 addr){
	struct l3_addr *new_entry;
	
	new_entry=malloc(sizeof(struct l3_addr));
	memset(new_entry,0,sizeof(struct l3_addr));
	new_entry->page_addr=(void *)addr;
	
	if(ss->pages_2move){
		new_entry->next=ss->pages_2move;
	}
	
	ss->number_pages2move++;
	ss->pages_2move=new_entry;

	
}

/** @brief Used to compute the frequency of accesses for a certain number of accesses to a page
 * @param ss global configuration of the sampling
 * @param addr Address of the page to add to rhe list
 */
void add_freq_access(struct sampling_settings *ss, int frequency){
	struct freq_stats *current=NULL; 
	HASH_FIND_INT(ss->metrics.freq_accesses, &frequency,current);
	
	if(!current){
			current=malloc(sizeof(struct freq_stats));
			memset(current,0,sizeof(struct freq_stats));
			current->freq=frequency;
			HASH_ADD_INT( ss->metrics.freq_accesses, freq, current );
	}
	
	current->count++;
	
	
}


/** @brief Prints on screen the statistics of a sampling interval
 * @param ss global configuration of the sampling
 */
void print_statistics(struct sampling_settings *ss){
		struct sampling_metrics m=ss->metrics;
		struct page_stats *current,*tmp;
		struct access_stats *crr_lvl,*lvltmp;
		struct freq_stats *crr,*tmpf;
		int i,freq,total_proc_samples=0;
		int ubound,lbound;
		char  *lblstr, *lbl="TODO-setlabel";
		int flips[21];
		memset(flips,0,sizeof(int)*21);
		lbl = !ss->output_label ? lbl : ss->output_label;
		printf("\t\t\t MIGRATION STATISTICS\n\n");
		printf("\t %s total samples: %d \n\n",lbl, m.total_samples);		
		
		for(i=0; i<ss->n_cores;i++){
				total_proc_samples+=m.process_samples[i];
				printf("%s cpu %d:  %d / %d \n",lbl,  i,  m.remote_samples[i],m.process_samples[i]);
		}
		printf("\t %s total samples %d sampling %d PID %d process samples: %d \n\n\n",lbl,ss->total_samples,ss->sampling_samples,ss->pid_uo, m.total_samples);
		printf("BREAKDOWN BY LOAD LATENCY\n\n");
		for(i=0; i<WEIGHT_BUCKETS_NR; i++){
			lbound=i*WEIGHT_BUCKET_INTERVAL;
			ubound=(i+1)*WEIGHT_BUCKET_INTERVAL;
			printf("%s :%d-%d: %d\n", lbl,lbound,ubound, ss->metrics.access_by_weight[i]);
		}
		printf("\n\n BREAKDOWN BY ACCESS LEVEL\n\n");
		HASH_ITER(hh, ss->metrics.lvl_accesses, crr_lvl, lvltmp) {
			lblstr=print_access_type(crr_lvl->mem_lvl);
			printf("%s:LEVEL count %d %s \n", lbl,  crr_lvl->count, lblstr);
		}
		HASH_SORT( ss->metrics.page_accesses, id_sort );
		
		HASH_ITER(hh, ss->metrics.page_accesses, current, tmp) {
			freq=0;
				for(i=0; i<ss->n_cpus; i++){
					freq+=current->proc_accesses[i];
				}
			if(current->home_flips>=0 && current->home_flips<20)
				flips[current->home_flips]++;
			else if(current->home_flips>19)
				flips[20]++;
			add_freq_access(ss,freq);	
		}
		
		HASH_SORT( ss->metrics.freq_accesses, freq_sort );
		printf("\n\n BREAKDOWN BY FREQUENCY OF PAGE ACCESSES \n\n");
		HASH_ITER(hh, ss->metrics.freq_accesses, crr, tmpf) {
			printf("%s:pages accessed:%d:%d \n",lbl,crr->freq,crr->count);
		}

		printf("CONTENTION ANALYSIS \n");
		for(i=0; i<21;i++){
			printf("Pages switched home % d times: %d \n", i, flips[i]);
		}
}


/** @brief Makes the migration of the pages that were put in the candidate migration list.
 * Makes the migration of the pages that were put in the candidate migration list.
 * First it checks that the candidate pages are indeed remote accesses
 * For those remote, a query is made to see if the page should be moved or left on the current node
 * at the end the migration is done
 *  @param ss global configuration of the sampling
 */
 
void do_great_migration(struct sampling_settings *ss){
	struct l3_addr *current=ss->pages_2move;
	void **pages;
	int ret,count=0,count2=0,*nodes,*nodes_query, *status,i,succesfully_moved=0,destination_node=0,greatest_count=0;
	int move_pending=0,move_size,already_moved=0,same_home=0,*destinations,significant_threshold=0;
	double tinit=0, tfin=0;
	struct page_stats *sear=NULL;
	char* thrswitch;
	
	
	pages=malloc(sizeof(void*) * ss->number_pages2move);
	status=malloc(sizeof(int) * ss->number_pages2move);
	nodes_query=malloc(sizeof(int) * ss->number_pages2move);
	memset(nodes_query, 0, sizeof(int) * ss->number_pages2move);
	destinations=malloc(sizeof(int)*ss->n_cpus);
	memset(destinations,0,sizeof(int)*ss->n_cpus);
	
	printf("Candidate accesses %d \n",ss->number_pages2move);
	//get the home of the current target pages
	while(current){
		*(pages+count)=(void *)current->page_addr;
		current=current->next;	
		count++;
	}

	ret=move_pages(ss->pid_uo, count, pages , NULL,nodes_query,0);
	printf("MIG> pages query returned %d \n",ret);

	if(ret!=0){
		printf("ERRNO %d \n",errno);
	}

	count=0;
	//This is the criteria for when to move a sample
	thrswitch=getenv("SPM_SIG_THRESH");
	if(thrswitch && atoi(thrswitch)>1){
		printf("evvar %s %d- \n",thrswitch,atoi(thrswitch));
		significant_threshold=atoi(thrswitch);
		printf("MIG> Will only move pages visited more thann %d times\n",significant_threshold);
	}
	memset(pages, 0, sizeof(int) * ss->number_pages2move);
	current=ss->pages_2move;
	nodes=malloc(sizeof(int) * ss->number_pages2move);
	//consolidates the page addresses into a single page address bundle
	while(current){
		HASH_FIND_PTR( ss->metrics.page_accesses,&(current->page_addr),sear );
		if(!sear){
			printf("cannot find entry %p when moving pages \n",current->page_addr);
			current=current->next;
			count2++;
			continue;
		}
		greatest_count=significant_threshold;
		destination_node=0;

		//The destination is the node with the greatest number of accesses
		for(i=0; i<ss->n_cpus; i++){
			if(sear->proc_accesses[i]>greatest_count){
					greatest_count=sear->proc_accesses[i];
					destination_node=i;
			}
		}

		//we ignore the pages where the destination is where they already are
		//For this if is that we need count 2
		if(*(nodes_query+count2)==destination_node){
			same_home++;
			count2++;
			current=current->next;	
			continue;
		}
		destinations[destination_node]++;
		*(pages+count)=(void *)current->page_addr;
		current=current->next;	
		count++;
		count2++;
		*(nodes+count)=destination_node;
	}

	printf("MIG> The initial query has found %d pages to move out of %d,(%d,%d) candidates \n",count,count2+same_home,same_home,count2);
	printf("Destination breakdown: ");
	for(i=0; i<ss->n_cpus; i++){
		printf(" - %d %d ",i, destinations[i] );
	}
	printf("\n");
	move_pending=count;
	
	do{
		tinit=wtime();
		
		if(move_pending>MAX_SINGLE_MIGRATE){
			move_size=MAX_SINGLE_MIGRATE;
		}else{
			move_size=move_pending; 
		}
		ret= 	move_pages(ss->pid_uo, move_size, pages+already_moved, nodes+already_moved, status+already_moved,0);
		if (ret!=0){
			printf("move_pages returned an error %d \n",errno);
		}
		move_pending-=move_size;
		tfin=wtime()-tinit;
		printf("Moved %d pages in %f, already moved %d pages and %d pages to move \n",move_size,tfin, already_moved,move_pending);
		already_moved+=move_size;
	}while(move_pending>0);
	ss->number_pages2move-=count+count2+same_home;
	
	//check the new home of the pages
	ret= 	move_pages(ss->pid_uo, move_size, pages, NULL, status,0);
	
	
	for(i=0; i<count;i++){
			if(status[i] >=0 && status[i]<ss->n_cpus)
				succesfully_moved++;		
	
	}
		
	ss->moved_pages+=succesfully_moved;
	

}
/** @brief Used to parse the list of cores belonging to a cpunode.
 *  @param max_cores the maximum nuber of cores in the system
 *  @param siblings A string that contains the intervals of cores in the sor X-Y, A-B,... where A,B,X,Y are positive integers
 */
 

int* get_cpu_interval(int max_cores, char* siblings ){
	int* sibling_array,i;
	char *savep1,*savep2;
	char* tok=strtok_r(siblings, ",",&savep1),*tokcpy;
	int high, low;
	char *interval1,*interval2;
	if(max_cores < 1) return NULL;
	
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

/** @brief Used to get whe location of every core in the system, that is to which numa node it belongs
 *  @param ss global configuration of the sampling
 * @param cpu_topo Core distribution as returned by the helper method
 */
 
void init_processor_mapping(struct sampling_settings *ss, struct cpu_topo *topol){

	int i,j,*siblings;
	int *core_to_node;
	int max_cores=0;
	
	//the new part begins here
	max_cores=ss->n_cores;
	core_to_node=malloc(sizeof(int)*max_cores);
	for(j=0; j<max_cores; j++){
		*(core_to_node+j)=-1;
	}
	
	for(i=0; i< ss->n_cpus;i++){
		siblings=get_cpu_interval(max_cores, topol->core_siblings[i]);
		if(!siblings) continue;
		for(j=0; j<max_cores; j++)
			*(core_to_node+j) = siblings[j]==1 ? i : *(core_to_node+j) ; 
			free(siblings);
	}
	
	for(i=0; i<max_cores;i++) 
	ss->core_to_cpu[i]=core_to_node[i];
	
	free(core_to_node);
}

/** @brief Frees memory used by the metrics
 */
 
void free_metrics(struct sampling_metrics *sm){
	struct page_stats *current,*tmp;
	struct access_stats *crr_lvl,*lvltmp;
	struct freq_stats *crr,*tmpf;
	if(sm->process_samples)
		free(sm->process_samples);
	if(sm->remote_samples)
		free(sm->remote_samples);
		
		//disabled because of aparent segfault problems with this freeing
	if(sm->page_accesses){
		HASH_ITER(hh, sm->page_accesses, current, tmp) {
				if(current){
				//unlink from the list
				//HASH_DEL( sm->page_accesses, current);
				//printf("%p ",current->page_addr);
				//chao
				//free(current);
				}
		}
	}
	if(sm->lvl_accesses){
	HASH_ITER(hh, sm->lvl_accesses, crr_lvl, lvltmp) {
			if(crr_lvl){
				//unlink from the list
				HASH_DEL( sm->lvl_accesses, crr_lvl);
				//chao
				free(crr_lvl);
				}
		}
	}
	
	if(sm->freq_accesses){
			HASH_ITER(hh, sm->freq_accesses, crr, tmpf) {
				if(crr){
					//unlink from the list
					HASH_DEL( sm->freq_accesses, crr);
					//chao
					free(crr);
					}
				}
		}
	
//	free(sm);
	
}

/** @brief Used to update a reading by a performance register
 * @param st The overall configuration containing the metrics
 * @param current the id of the meqasure to update
 * @param record contains the new reading
 * @param cpu contains the cpu that generated the new record
 */
 
 
void update_pf_reading(struct sampling_settings *st,  pf_profiling_rec_t *record, int current, struct _perf_cpu *cpu){
	pf_profiling_rec_t sample;
	int ncpu=cpu->cpuid;
	
	uint64_t val;
	sample=record[current];

	//updates the found value

	for(int i=0; i<COUNT_NUM; i++){
		val=sample.countval.counts[i];
		*(st->metrics.pf_read_values+ncpu*COUNT_NUM+i)=val;
	}

	if(wtime()-st->time_last_read>1){
		calculate_pf_diff(st);
		st->time_last_read=wtime();
	}

}

/** @brief Calculated the difference between two performance readings which is the actual change
 * @param st The overall configuration containing the metrics

 */
void calculate_pf_diff(struct sampling_settings *st){
	struct perf_info *current;

	for(int j=0; j<st->n_cores; j++){
		current=malloc(sizeof (struct perf_info));
		current->values=malloc(sizeof(int)*COUNT_NUM);
		current->time=wtime()-st->start_time ;
		//it does the respective linking
		if(!st->metrics.perf_info_first[j]){
			st->metrics.perf_info_first[j]=current;
		}
		if(!st->metrics.perf_info_last[j]){
			st->metrics.perf_info_last[j]=current;
		}else{
			st->metrics.perf_info_last[j]->next=current;
			st->metrics.perf_info_last[j]=current;
		}
		st->metrics.number_pf_samples++;
		for(int i=0; i<COUNT_NUM; i++){


			*(st->metrics.pf_diff_values+j*COUNT_NUM+i)=*(st->metrics.pf_read_values+j*COUNT_NUM+i)-*(st->metrics.pf_last_values+j*COUNT_NUM+i);
			*(st->metrics.pf_last_values+j*COUNT_NUM+i)=*(st->metrics.pf_read_values+j*COUNT_NUM+i);
			current->values[i]=*(st->metrics.pf_diff_values+j*COUNT_NUM+i);
			st->metrics.running_accum[i]+=current->values[i];
		}
		//printf("\n");
	}

}

/** @brief This method is the entry point for processing an incoming pebs load latency sample
 * @param st The overall configuration containing the metrics
 * @param record the record to process
	@param current The current sample in the circular buffer to process
 */
 
void consume_sample(struct sampling_settings *st,  pf_ll_rec_t *record, int current){

	int core=record[current].cpu;
	if((int)record[current].cpu >=  st->n_cores){
		return;
	}

	if(st->disable_ll) return;
	
	if(getpid() == (int)record[current].pid){
		st->sampling_samples++; 
	}
		
	st->total_samples++;
	st->metrics.total_samples++;
	if((int)record[current].pid != st->pid_uo){
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
/**
 * @brief Prints the performance information second by second. The environmen variable SPM_PRINT_PERFORMANCE  must be declared in order for the method to be called
 **/
void print_performance(struct perf_info **firsts, struct sampling_settings *st ){
		int out=1,i,j,first;
		struct perf_info **currents=malloc(st->n_cores*sizeof(struct perf_info));
		double ltime,val;
		for(i=0; i<st->n_cores; i++){
			currents[i]=firsts[i];
		}
		uint64_t *accum=malloc(sizeof(uint64_t)*COUNT_NUM);
		memset(accum,0,sizeof(uint64_t)*COUNT_NUM);
		first=1;
		printf("COUNTER READINGS \n\n");
		while(out){
			//will retrieve the info for very cpu
			out=0;
			for(i=0; i<st->n_cores; i++){
				if(currents[i]){
					out++;
					
					if(!first){
						printf("%d %f ", i, currents[i]->time);
						for(j=0; j<COUNT_NUM; j++){

								printf(" %lu ", currents[i]->values[j] );
								accum[j]+=currents[i]->values[j];

						}
						
						printf("\n");
					}
					ltime=currents[i]->time;
					currents[i]=currents[i]->next;
					
				}
			}
			first=0;
			
			if(!first){
				printf("AVE: %f ",ltime);
				for(j=0; j<COUNT_NUM; j++){
					val= out != 0 ? (float) accum[j]/out :0 ;
					printf("%lu ",(unsigned long)val );
				}
			printf(" \n");
			}
			memset(accum,0,sizeof(uint64_t)*COUNT_NUM);
		}
}
