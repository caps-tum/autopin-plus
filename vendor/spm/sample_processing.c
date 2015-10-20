
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <numaif.h>
#include <unistd.h>
#include <numa.h>
#include <time.h>
#include <sys/time.h>
#include "spm.h"


double wtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, 0);

  return tv.tv_sec+1e-6*tv.tv_usec;
}

void add_mem_access( struct sampling_settings *ss, void *page_addr, int accessing_cpu){
	struct page_stats *current=NULL; 
	struct page_stats *cursor=NULL; 
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
		//printf ("add %d \n",HASH_COUNT(ss->metrics.page_accesses));
		
	}

	/*Here begins the new page access bookkeeping*/
	current->proc_accesses[proc]++;
	//printf("%d", proc);
}

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

//makes sure that the list of arguments ends with a null pointer
char ** put_end_params(const char **argv,int argc){
	char ** list_args=malloc(sizeof(char*)*(argc+1));
	int i;
	
	for(i=0; i<argc; i++){
		*(list_args+i)=(char*)*(argv+i);
	}
	
	*(list_args+argc)=NULL;
	
	return list_args;
}

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

static int freq_sort(struct freq_stats *a, struct freq_stats *b) {
    return a->freq - b->freq;
}

long id_sort(struct page_stats *a, struct page_stats *b) {
    long rst= (long )a->page_addr - (long)b->page_addr;
    return rst;
}

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

void print_statistics(struct sampling_settings *ss){
		struct sampling_metrics m=ss->metrics;
		struct page_stats *current,*tmp;
		struct access_stats *crr_lvl,*lvltmp;
		struct freq_stats *crr,*tmpf;
		int i,freq,total_proc_samples=0;
		int ubound,lbound;
		char  *lblstr, *lbl="TODO-setlabel";
		lbl = !ss->output_label ? lbl : ss->output_label;
		printf("\t\t\t MIGRATION STATISTICS\n\n");
		printf("\t %s total samples: %d \n\n",lbl, m.total_samples);		
		
		for(i=0; i<ss->n_cores;i++){
				total_proc_samples+=m.process_samples[i];
				printf("%s cpu %d:  %d / %d \n",lbl,  i,  m.remote_samples[i],m.process_samples[i]);
		}
		printf("\t %s PID %d total process samples: %d \n\n\n",lbl,ss->pid_uo, m.total_samples);
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
				//printf("record ");
			add_freq_access(ss,freq);	
		}
		
		HASH_SORT( ss->metrics.freq_accesses, freq_sort );
		printf("\n\n BREAKDOWN BY FREQUENCY OF PAGE ACCESSES \n\n");
		HASH_ITER(hh, ss->metrics.freq_accesses, crr, tmpf) {
			printf("%s:pages accessed:%d:%d \n",lbl,crr->freq,crr->count);
		}
	}

void do_great_migration(struct sampling_settings *ss){
	struct l3_addr *current=ss->pages_2move;
	void **pages;
	int ret,count=0,count2=0,*nodes,*nodes_query, *status,i,succesfully_moved=0,destination_node=0,greatest_count=0;
	double tinit=0, tfin=0;
	struct page_stats *sear=NULL;

	printf("l3accesses %d \n",ss->number_pages2move);
	pages=malloc(sizeof(void*) * ss->number_pages2move);
	status=malloc(sizeof(int) * ss->number_pages2move);
	nodes_query=malloc(sizeof(int) * ss->number_pages2move);
	memset(nodes_query, 0, sizeof(int) * ss->number_pages2move);
	//get the home of the current target pages
	while(current){
		*(pages+count)=(void *)current->page_addr;
		current=current->next;	
		count++;
	}
	
	move_pages(ss->pid_uo, count, pages, nodes_query, status,0);
	
	count=0;
	memset(pages, 0, sizeof(int) * ss->number_pages2move);
	current=ss->pages_2move;
	nodes=malloc(sizeof(int) * ss->number_pages2move);
	//consolidates the page addresses into a single page address bundle
	while(current){
	//	printf("%p \n", current->page_addr);
		HASH_FIND_PTR( ss->metrics.page_accesses,&(current->page_addr),sear );
		if(!sear){
			printf("cannot find entry %p when moving pages \n",current->page_addr);
			current=current->next;
			count2++;
			continue;
		}
		//The destination is the node with the greatest number of accesses
		for(i=0; i<ss->n_cpus; i++){
			if(sear->proc_accesses[i]>greatest_count){
					greatest_count=sear->proc_accesses[i];
					destination_node=i;
			}
		}
		//we ignore the pages where the destination is where they already are
		if(*(nodes_query+count2)==destination_node){
			count2++;
			current=current->next;	
			continue;
		}
		
		*(pages+count)=(void *)current->page_addr;
		current=current->next;	
		count++;
		count2++;
		*(nodes+count)=destination_node;
		
	}
	
	//if(ss->migrate_chunk_size >0 )
	//	count=ss->migrate_chunk_size;
		
	tinit=wtime();
	ret= 	move_pages(ss->pid_uo, count, pages, nodes, status,0);
	
	if (ret!=0){
		printf("move_pages returned an error %d \n",errno);
	}
	
	//check the new home of the pages
	ret= 	move_pages(ss->pid_uo, count, pages, NULL, status,0);
	tfin=wtime()-tinit;
	
	for(i=0; i<count;i++){
			if(status[i] >=0 && status[i]<ss->n_cpus)
				succesfully_moved++;		
			//printf(" %d ",status[i]);
			
			//if(i%20==0) printf ("\n");	
		}
	
	
	
	ss->moved_pages=succesfully_moved;
	printf("%d lem/rte accesses, attempt to move %d, %d pages moved successfully, move pages time %f \n",count2,count,succesfully_moved,tfin);
 
}

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

void free_metrics(struct sampling_metrics *sm){
	struct page_stats *current,*tmp;
	struct access_stats *crr_lvl,*lvltmp;
	struct freq_stats *crr,*tmpf;
	if(sm->process_samples)
		free(sm->process_samples);
	if(sm->remote_samples)
		free(sm->remote_samples);
		
	if(sm->page_accesses){
		HASH_ITER(hh, sm->page_accesses, current, tmp) {
				if(current){
				//unlink from the list
				
				HASH_DEL( sm->page_accesses, current);
				//chao
				free(current);
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
