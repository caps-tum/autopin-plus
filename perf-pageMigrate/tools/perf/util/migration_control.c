#include "migration_control.h"
#include "numa_metrics.h"
#include "util/top.h"
#include "util/util.h"

#include <pthread.h>


//This represents the main logic of the tool

int run_numa_analysis(void *arg){
	struct perf_top** tops ;
	pthread_t aux_thread;
	int j;
	
	tops=(struct perf_top**)arg;
	
	if(tops[0]->numa_metrics->pid_uo==0){
			tops[0]->numa_metrics->pid_uo=tops[0]->numa_migrate_pid_filter;
	} 
	
	if(tops[0]->numa_metrics->pid_uo==0){
		printf("MIG-CTRL>does not have a valid pid to track \”");
	}
	//phase 1: will run the tool for a specific measurement period
	printf("MIG-CTRL> children thread launched\n");
	
	
	if(pthread_create(&aux_thread,NULL,measure_aux_thread,(void *) *(tops))){
		printf("MIG-CTRL> could not create aux thread \n");
	}
	
	//launch top and gather data
	__cmd_top(*(tops));
	
	pthread_join(aux_thread, NULL); 
	
}

//This is an O^2 algorithm
//not intended for production use!
void run_expensive_access_analysis(struct numa_metrics* nm){
	struct l3_addr *start=nm->pages_2move,*tom_cursor;
	struct exp_access *current=nm->expensive_accesses;
	int repeated=0,counted=0;
	union perf_mem_data_src ds;
	
	
	u64 dsrc;
	
	while(current){
		tom_cursor=start;
		while(tom_cursor){
			if(current->page_addr==tom_cursor->page_addr){
				repeated++;
				dsrc=current->samp.data_src;
				ds.val=dsrc;
				//printf("ea %s \n",print_access_type(ds.mem_lvl));
				goto next_acc;			
			}
			tom_cursor=tom_cursor->next;
			
		}
		next_acc: counted++;
		current=current->next;
	}
	
	printf("%d Slow access as part of move page out of %d \n", repeated,counted);
}
int wait_watch_process(int seconds,struct numa_metrics* nm){
	int i=0,exit=0;
	int sigres,*st=0,errn;
	//every second it wakes up to make sure the process is alive

	while(!exit){
		sleep(1);
		sigres=kill(nm->pid_uo,0);
		waitpid(nm->pid_uo,st,WNOHANG);
		if(sigres==-1 ){
			errn=errno;
			if (errn== ESRCH || errn==ECHILD){
					return -1;
				}
			}
		exit= (seconds > 1 && ++i<seconds) || seconds < 1 ? 0 : 1;
	}
	return 0;
}

void * measure_aux_thread(void *arg){
	struct perf_top *top=(struct perf_top*) arg;
	int sleep_time=top->numa_sensing_time,wait_res;
	char* newlabel;
	sleep_time=sleep_time<1 ?  DEFAULT_SENSING_TIME  : sleep_time;
	
	
	
	wait_res=wait_watch_process(sleep_time,top->numa_metrics);
	if(wait_res) goto end_noproc;
	printf("MIG-CTRL> End of sampling period\n");
	top->numa_analysis_enabled=false;
	
	do_great_migration(top->numa_metrics);
	//print the overall statistics right after moving pages
	print_migration_statistics(top->numa_metrics);
	if(top->migrate_track_levels){
		print_access_info(top->numa_metrics);
	}
	
	top->numa_metrics->page_accesses=NULL;
	top->numa_metrics->lvl_accesses=NULL;
	top->numa_metrics->freq_accesses=NULL;
	top->numa_metrics->moved_pages=0;
	top->numa_metrics->expensive_accesses=NULL;
	//TODO adjust to core size
	memset(top->numa_metrics->remote_accesses ,0,32);
	memset(top->numa_metrics->process_accesses ,0,32);
	memset(top->numa_metrics->access_by_weight ,0,WEIGHT_BUCKETS_NR);
	newlabel=(char *)malloc((strlen(top->numa_metrics->file_label)*sizeof(char))+2);
	strcpy(newlabel,top->numa_metrics->file_label);
	strcat(newlabel,"-2"),
	top->numa_metrics->file_label=newlabel;
	printf("MIG-CTRL> Call page migration \n");
	
	top->numa_analysis_enabled=true;
	wait_res=wait_watch_process(-1,top->numa_metrics);

	
	print_migration_statistics(top->numa_metrics);
	run_expensive_access_analysis(top->numa_metrics);
	if(top->migrate_track_levels){
		print_access_info(top->numa_metrics);
	}

	if(top->migrate_filereport){
		//TODO review this
		//close_report_file(nm);
	}
end_noproc:
	printf("MIG-CTRL> End of measurement due to end of existing process");
	top->numa_metrics->timer_up=true;
	return NULL;
}

/*
 * Entry point to the application can run and analyze the process 
 * specified as parameter or can attach to an already existing process  
 */
 
int init_numa_analysis(int mode, int pid,const char **command_args,int command_argc ){
	struct perf_top *tops[2];
	pthread_t numatool_thread;

	struct numa_metrics* nm;
	
	struct cpu_topo *cpu_topo;
	
	page_size = sysconf(_SC_PAGE_SIZE);
	//Numa-migrate stuff is initialized here
	nm=malloc(sizeof(struct numa_metrics));
	memset(nm, 0, sizeof(struct numa_metrics));
	nm->page_accesses=NULL;
	nm->lvl_accesses=NULL;
	nm->pages_2move=NULL;

	cpu_topo=build_cpu_topology();
	nm->n_cpus=numa_num_configured_nodes();
	init_processor_mapping(nm,cpu_topo);
	
	if (mode== RUN_ATTACHED){
		if(pid==0)
			return NUMATOOL_ERROR;	
	}


	tops[0]=cmd_top(command_argc,command_args,NULL);
	if (!tops[0])
		return NUMATOOL_ERROR;
		
	//initialization of the numa_metrics struct
	nm->logging_detail_level=tops[0]->numa_migrate_logdetail;
	nm->moved_pages=0;
	tops[0]->numa_metrics=nm;
	nm->pid_uo=tops[0]->numa_migrate_pid_filter;
	nm->file_label=tops[0]->numa_filelabel;
	nm->timer_up=false;
	nm->migrate_chunk_size=tops[0]->migrate_chunk_size;
	nm->file_label="";
	
	
	//in case of using run_command will launch the process
		
	if(mode==RUN_COMMAND){
		nm->pid_uo=launch_command(command_args, command_argc);
	}
	
	if(pthread_create(&numatool_thread,NULL,run_numa_analysis, tops)){
		return NUMATOOL_ERROR; 
	}
	
	pthread_join(numatool_thread, NULL); 
		return NUMATOOL_SUCCESS;
}


