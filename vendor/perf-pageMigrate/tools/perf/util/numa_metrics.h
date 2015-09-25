#ifndef __NUMAMM_HIST_H
#define __NUMAMM_HIST_H
#include <stdio.h>
#include <uthash.h>
#include <stdbool.h>
#include "types.h"
#include "event.h" 
#include "util/top.h"




#define WEIGHT_BUCKETS_NR 19
#define WEIGHT_BUCKET_INTERVAL 50
#define LINE_SIZE 500
#define SAMPLE_WEIGHT_THRESHOLD 600

 #define CORE_SIB_FMT \
         "/sys/devices/system/cpu/cpu%d/topology/core_siblings_list"
 #define THRD_SIB_FMT \
         "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list"

struct numa_metrics {
	int n_cpus;
	int n_cores;
	unsigned int pid_uo;
	int *remote_accesses;
	int *process_accesses;
	int *cpu_to_processor;
	int logging_detail_level;
	int number_pages2move;
	int access_by_weight[WEIGHT_BUCKETS_NR];
	int	migrate_chunk_size;
	struct page_stats *page_accesses;
	struct access_stats *lvl_accesses;
	struct freq_stats *freq_accesses;
	struct l3_addr *pages_2move;
	struct exp_access *expensive_accesses;
	int moved_pages;
	FILE *report;
	bool gather_candidates;
	char* report_filename;
	char* command2_launch;
	const char* file_label;
	bool timer_up;
	};

struct page_stats{
	int *proc_accesses;
	void* page_addr;
	UT_hash_handle hh;
};

struct access_stats{
	int count;
	int mem_lvl;
	UT_hash_handle hh;
};

struct freq_stats{
	int count;
	int freq;
	UT_hash_handle hh;
};

struct cpu_topo {
	u32 core_sib;
	u32 thread_sib;
	char **core_siblings;
	char **thread_siblings;
};


//used to track the l3 access on the measurement phase
struct l3_addr{
	void* page_addr;
	struct l3_addr *next;
};

struct exp_access{
	void* page_addre;
	struct exp_access *next;
	struct perf_sample samp;
	UT_hash_handle hh;
};


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

void sort_entries(struct numa_metrics *nm);


void init_processor_mapping(struct numa_metrics *multiproc_info, struct cpu_topo *topol);

void add_mem_access( struct numa_metrics *multiproc_info, void *page_addr, int accessing_cpu);

int filter_local_accesses(union perf_mem_data_src *entry);

void  print_migration_statistics(struct numa_metrics *nm);

void add_lvl_access( struct numa_metrics *multiproc_info, union perf_mem_data_src *entry, int weight );
	
void print_access_info(struct numa_metrics *multiproc_info);

char* print_access_type(int entry);

void init_report_file(struct numa_metrics *nm);

void close_report_file(struct numa_metrics *nm);

int launch_command(const char** argv, int argc);

void print_info(FILE* file, const char* format, ...);

char ** put_end_params(const char **argv,int argc);

char* get_command_string(const char ** argv, int argc);

long id_sort(struct page_stats *a, struct page_stats *b);

void add_page_2move(struct numa_metrics *nm,u64 addr);

void do_great_migration(struct numa_metrics *nm);

void add_freq_access(struct numa_metrics *nm, int frequency);

double wtime(void);
	
void add_expensive_access(struct numa_metrics *nm,u64 addr);

struct cpu_topo *build_cpu_topology(void);

int* get_cpu_interval(int max_cores, char* siblings );
 
int __cmd_top(struct perf_top *top);


#endif
