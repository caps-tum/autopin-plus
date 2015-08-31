/*
 * builtin-top.c
 *
 * Builtin top command: Display a continuously updated profile of
 * any workload, CPU or specific PID.
 *
 * Copyright (C) 2008, Red Hat Inc, Ingo Molnar <mingo@redhat.com>
 *		 2011, Red Hat Inc, Arnaldo Carvalho de Melo <acme@redhat.com>
 *
 * Improvements and fixes by:
 *
 *   Arjan van de Ven <arjan@linux.intel.com>
 *   Yanmin Zhang <yanmin.zhang@intel.com>
 *   Wu Fengguang <fengguang.wu@intel.com>
 *   Mike Galbraith <efault@gmx.de>
 *   Paul Mackerras <paulus@samba.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include "builtin.h"

#include "perf.h"

#include "util/annotate.h"
#include "util/cache.h"
#include "util/color.h"
#include "util/evlist.h"
#include "util/evsel.h"
#include "util/machine.h"
#include "util/session.h"
#include "util/symbol.h"
#include "util/thread.h"
#include "util/thread_map.h"
#include "util/top.h"
#include "util/util.h"
#include <linux/rbtree.h>
#include "util/parse-options.h"
#include "util/parse-events.h"
#include "util/cpumap.h"
#include "util/xyarray.h"
#include "util/sort.h"
#include "util/intlist.h"
#include "arch/common.h"
#include "util/numa_metrics.h"

#include "util/debug.h"

#include <assert.h>
#include <elf.h>
#include <fcntl.h>

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>

#include <errno.h>
#include <time.h>
#include <sched.h>

#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/mman.h>

#include <linux/unistd.h>
#include <linux/types.h>

static volatile int done;

#define HEADER_LINE_NR  5

static void perf_top__update_print_entries(struct perf_top *top)
{
	top->print_entries = top->winsize.ws_row - HEADER_LINE_NR;
}

static void perf_top__sig_winch(int sig __maybe_unused,
				siginfo_t *info __maybe_unused, void *arg)
{
	struct perf_top *top = arg;

	get_term_dimensions(&top->winsize);
	perf_top__update_print_entries(top);
}

static int perf_top__parse_source(struct perf_top *top, struct hist_entry *he)
{
	struct symbol *sym;
	struct annotation *notes;
	struct map *map;
	int err = -1;

	if (!he || !he->ms.sym)
		return -1;

	sym = he->ms.sym;
	map = he->ms.map;

	/*
	 * We can't annotate with just /proc/kallsyms
	 */
	if (map->dso->symtab_type == DSO_BINARY_TYPE__KALLSYMS &&
	    !dso__is_kcore(map->dso)) {
		pr_err("Can't annotate %s: No vmlinux file was found in the "
		       "path\n", sym->name);
		sleep(1);
		return -1;
	}

	notes = symbol__annotation(sym);
	if (notes->src != NULL) {
		pthread_mutex_lock(&notes->lock);
		goto out_assign;
	}

	pthread_mutex_lock(&notes->lock);

	if (symbol__alloc_hist(sym) < 0) {
		pthread_mutex_unlock(&notes->lock);
		pr_err("Not enough memory for annotating '%s' symbol!\n",
		       sym->name);
		sleep(1);
		return err;
	}

	err = symbol__annotate(sym, map, 0);
	if (err == 0) {
out_assign:
		top->sym_filter_entry = he;
	}

	pthread_mutex_unlock(&notes->lock);
	return err;
}

static void __zero_source_counters(struct hist_entry *he)
{
	struct symbol *sym = he->ms.sym;
	symbol__annotate_zero_histograms(sym);
}

static void ui__warn_map_erange(struct map *map, struct symbol *sym, u64 ip)
{
	struct utsname uts;
	int err = uname(&uts);

	ui__warning("Out of bounds address found:\n\n"
		    "Addr:   %" PRIx64 "\n"
		    "DSO:    %s %c\n"
		    "Map:    %" PRIx64 "-%" PRIx64 "\n"
		    "Symbol: %" PRIx64 "-%" PRIx64 " %c %s\n"
		    "Arch:   %s\n"
		    "Kernel: %s\n"
		    "Tools:  %s\n\n"
		    "Not all samples will be on the annotation output.\n\n"
		    "Please report to linux-kernel@vger.kernel.org\n",
		    ip, map->dso->long_name, dso__symtab_origin(map->dso),
		    map->start, map->end, sym->start, sym->end,
		    sym->binding == STB_GLOBAL ? 'g' :
		    sym->binding == STB_LOCAL  ? 'l' : 'w', sym->name,
		    err ? "[unknown]" : uts.machine,
		    err ? "[unknown]" : uts.release, perf_version_string);
	if (use_browser <= 0)
		sleep(5);
	
	map->erange_warned = true;
}




static void prompt_integer(int *target, const char *msg)
{
	char *buf = malloc(0), *p;
	size_t dummy = 0;
	int tmp;

	fprintf(stdout, "\n%s: ", msg);
	if (getline(&buf, &dummy, stdin) < 0)
		return;

	p = strchr(buf, '\n');
	if (p)
		*p = 0;

	p = buf;
	while(*p) {
		if (!isdigit(*p))
			goto out_free;
		p++;
	}
	tmp = strtoul(buf, NULL, 10);
	*target = tmp;
out_free:
	free(buf);
}

static void prompt_percent(int *target, const char *msg)
{
	int tmp = 0;

	prompt_integer(&tmp, msg);
	if (tmp >= 0 && tmp <= 100)
		*target = tmp;
}



static int symbol_filter(struct map *map __maybe_unused, struct symbol *sym)
{
	const char *name = sym->name;

	/*
	 * ppc64 uses function descriptors and appends a '.' to the
	 * start of every instruction address. Remove it.
	 */
	if (name[0] == '.')
		name++;

	if (!strcmp(name, "_text") ||
	    !strcmp(name, "_etext") ||
	    !strcmp(name, "_sinittext") ||
	    !strncmp("init_module", name, 11) ||
	    !strncmp("cleanup_module", name, 14) ||
	    strstr(name, "_text_start") ||
	    strstr(name, "_text_end"))
		return 1;

	if (symbol__is_idle(sym))
		sym->ignore = true;

	return 0;
}

static void perf_event__process_sample(struct perf_tool *tool,
				       const union perf_event *event,
				       struct perf_evsel *evsel,
				       struct perf_sample *sample,
				       struct machine *machine)
{
	struct perf_top *top = container_of(tool, struct perf_top, tool);
	struct symbol *parent = NULL;
	//u64 ip = sample->ip;
	struct addr_location al;
	int err;
	//numa-an
	union perf_mem_data_src data_src;
	int filter_access=0;
	u64 mask=0xFFF, page_addr;
	struct numa_metrics *nm;

	if (!machine && perf_guest) {
		static struct intlist *seen;

		if (!seen)
			seen = intlist__new(NULL);

		if (!intlist__has_entry(seen, sample->pid)) {
			pr_err("Can't find guest [%d]'s kernel information\n",
				sample->pid);
			intlist__add(seen, sample->pid);
		}
		return;
	}

	if (!machine) {
		pr_err("%u unprocessable samples recorded.\r",
		       top->session->stats.nr_unprocessable_samples++);
		return;
	}

	if (event->header.misc & PERF_RECORD_MISC_EXACT_IP)
		top->exact_samples++;

	if (perf_event__preprocess_sample(event, machine, &al, sample) < 0 ||
	    al.filtered)
		return;

	if (!top->kptr_restrict_warned &&
	    symbol_conf.kptr_restrict &&
	    al.cpumode == PERF_RECORD_MISC_KERNEL) {
		ui__warning(
"Kernel address maps (/proc/{kallsyms,modules}) are restricted.\n\n"
"Check /proc/sys/kernel/kptr_restrict.\n\n"
"Kernel%s samples will not be resolved.\n",
			  !RB_EMPTY_ROOT(&al.map->dso->symbols[MAP__FUNCTION]) ?
			  " modules" : "");
		if (use_browser <= 0)
			sleep(5);
		top->kptr_restrict_warned = true;
	}

	if (al.sym == NULL) {
		const char *msg = "Kernel samples will not be resolved.\n";
		/*
		 * As we do lazy loading of symtabs we only will know if the
		 * specified vmlinux file is invalid when we actually have a
		 * hit in kernel space and then try to load it. So if we get
		 * here and there are _no_ symbols in the DSO backing the
		 * kernel map, bail out.
		 *
		 * We may never get here, for instance, if we use -K/
		 * --hide-kernel-symbols, even if the user specifies an
		 * invalid --vmlinux ;-)
		 */
		if (!top->kptr_restrict_warned && !top->vmlinux_warned &&
		    al.map == machine->vmlinux_maps[MAP__FUNCTION] &&
		    RB_EMPTY_ROOT(&al.map->dso->symbols[MAP__FUNCTION])) {
			if (symbol_conf.vmlinux_name) {
				ui__warning("The %s file can't be used.\n%s",
					    symbol_conf.vmlinux_name, msg);
			} else {
				ui__warning("A vmlinux file was not found.\n%s",
					    msg);
			}

			if (use_browser <= 0)
				sleep(5);
			top->vmlinux_warned = true;
		}
	}

	if (al.sym == NULL || !al.sym->ignore) {
		//struct hist_entry *he;

		err = sample__resolve_callchain(sample, &parent, evsel, &al,
						top->max_stack);
		if (err)
			return;
	
		//numa-an kicks in here to examine the samples we are interested in
		
		if (top->numa_migrate_mode && sample->pid == top->numa_metrics->pid_uo && top->numa_analysis_enabled){
			//get the type of access
			data_src.val = sample->data_src;
			filter_access=filter_local_accesses(&data_src);
			
			
			if(sample->cpu <31 ){
				//statistics and hash table
				top->numa_metrics->process_accesses[sample->cpu]++;
				page_addr=sample->addr & ~mask ;
				nm=top->numa_metrics;
				add_mem_access( nm, (void*)page_addr, sample->cpu);
				
				if(top->numa_migrate_logdetail>5)
					printf("Access detected addr %lux filter %d cpu %d weight %lu \n", 
				page_addr,filter_access, sample->cpu, sample->weight);
				
				if(top->migrate_track_levels){
					 add_lvl_access( nm, &data_src,sample->weight );
				}
			}
			//we care about return value 0
			if(!filter_access){
				top->numa_metrics->remote_accesses[sample->cpu]++;
				
				add_page_2move(top->numa_metrics, page_addr);

				// migration cancelled, instead the L3s are added to the candidates list
				//migrate_res=do_migration(nm, nm->pid_uo, sample);
			}
			//If the sample has a higher cost we save the address
			// This is for analysis on the phase II
			if(sample->weight > SAMPLE_WEIGHT_THRESHOLD){
					add_expensive_access(top->numa_metrics,page_addr);
			}
		              
		}
		
		//since numa-mig does not use gui, the hists are disabled
		//he = perf_evsel__add_hist_entry(evsel, &al, sample);
		//if (he == NULL) {
			//pr_err("Problem incrementing symbol period, skipping event\n");
			//return;
		//}

		//err = hist_entry__append_callchain(he, sample);
		//if (err)
			//return;

		//if (sort__has_sym)
			//perf_top__record_precise_ip(top, he, evsel->idx, ip);
	}

	return;
}

static void perf_top__mmap_read_idx(struct perf_top *top, int idx)
{
	struct perf_sample sample;
	struct perf_evsel *evsel;
	struct perf_session *session = top->session;
	union perf_event *event;
	struct machine *machine;
	u8 origin;
	int ret;

	while ((event = perf_evlist__mmap_read(top->evlist, idx)) != NULL) {
		ret = perf_evlist__parse_sample(top->evlist, event, &sample);
		if (ret) {
			pr_err("Can't parse sample, err = %d\n", ret);
			goto next_event;
		}

		evsel = perf_evlist__id2evsel(session->evlist, sample.id);
		assert(evsel != NULL);

		origin = event->header.misc & PERF_RECORD_MISC_CPUMODE_MASK;

		if (event->header.type == PERF_RECORD_SAMPLE)
			++top->samples;

		switch (origin) {
		case PERF_RECORD_MISC_USER:
			++top->us_samples;
			if (top->hide_user_symbols)
				goto next_event;
			machine = &session->machines.host;
			break;
		case PERF_RECORD_MISC_KERNEL:
			++top->kernel_samples;
			if (top->hide_kernel_symbols)
				goto next_event;
			machine = &session->machines.host;
			break;
		case PERF_RECORD_MISC_GUEST_KERNEL:
			++top->guest_kernel_samples;
			machine = perf_session__find_machine(session,
							     sample.pid);
			break;
		case PERF_RECORD_MISC_GUEST_USER:
			++top->guest_us_samples;
			/*
			 * TODO: we don't process guest user from host side
			 * except simple counting.
			 */
			/* Fall thru */
		default:
			goto next_event;
		}


		if (event->header.type == PERF_RECORD_SAMPLE) {
			perf_event__process_sample(&top->tool, event, evsel,
						   &sample, machine);
		} else if (event->header.type < PERF_RECORD_MAX) {
			hists__inc_nr_events(&evsel->hists, event->header.type);
			machine__process_event(machine, event, &sample);
		} else
			++session->stats.nr_unknown_events;
next_event:
		perf_evlist__mmap_consume(top->evlist, idx);
	}
}

static void perf_top__mmap_read(struct perf_top *top)
{
	int i;

	for (i = 0; i < top->evlist->nr_mmaps; i++)
		perf_top__mmap_read_idx(top, i);
}

static int perf_top__start_counters(struct perf_top *top)
{
	char msg[512];
	struct perf_evsel *counter;
	struct perf_evlist *evlist = top->evlist;
	struct record_opts *opts = &top->record_opts;

	perf_evlist__config(evlist, opts);

	evlist__for_each(evlist, counter) {
try_again:
		if (perf_evsel__open(counter, top->evlist->cpus,
				     top->evlist->threads) < 0) {
			if (perf_evsel__fallback(counter, errno, msg, sizeof(msg))) {
				if (verbose)
					ui__warning("%s\n", msg);
				goto try_again;
			}

			perf_evsel__open_strerror(counter, &opts->target,
						  errno, msg, sizeof(msg));
			ui__error("%s\n", msg);
			goto out_err;
		}
	}

	if (perf_evlist__mmap(evlist, opts->mmap_pages, false) < 0) {
		ui__error("Failed to mmap with %d (%s)\n",
			    errno, strerror(errno));
		goto out_err;
	}

	return 0;

out_err:
	return -1;
}

static int perf_top__setup_sample_type(struct perf_top *top __maybe_unused)
{
	if (!sort__has_sym) {
		if (symbol_conf.use_callchain) {
			ui__error("Selected -g but \"sym\" not present in --sort/-s.");
			return -EINVAL;
		}
	} else if (callchain_param.mode != CHAIN_NONE) {
		if (callchain_register_param(&callchain_param) < 0) {
			ui__error("Can't register callchain params.\n");
			return -EINVAL;
		}
	}

	return 0;
}

 int __cmd_top(struct perf_top *top)
{
	struct record_opts *opts = &top->record_opts;
	//pthread_t thread;
	int ret;
	
	
	
	if(top->migrate_filereport){
		//TODO review this
		//init_report_file(nm);
	}
	
	
	//this has been discontinued in the newer version
	//if (top->launch_command && top->argv_size>0){	
		//launch_command(top->numa_metrics,top->command2_launch, top->argv_size);
	//}

	
	print_info(top->numa_metrics->report, "command : %s \n pid %d \n ",
		top->command_string,top->numa_metrics->pid_uo);
			
	
	top->session = perf_session__new(NULL, false, NULL);
	if (top->session == NULL)
		return -ENOMEM;

	machines__set_symbol_filter(&top->session->machines, symbol_filter);

	if (!objdump_path) {
		ret = perf_session_env__lookup_objdump(&top->session->header.env);
		if (ret)
			goto out_delete;
	}

	ret = perf_top__setup_sample_type(top);
	if (ret)
		goto out_delete;

	machine__synthesize_threads(&top->session->machines.host, &opts->target,
				    top->evlist->threads, false);
	ret = perf_top__start_counters(top);
	if (ret)
		goto out_delete;

	top->session->evlist = top->evlist;
	perf_session__set_id_hdr_size(top->session);

	/*
	 * When perf is starting the traced process, all the events (apart from
	 * group members) have enable_on_exec=1 set, so don't spoil it by
	 * prematurely enabling them.
	 *
	 * XXX 'top' still doesn't start workloads like record, trace, but should,
	 * so leave the check here.
	 */
        if (!target__none(&opts->target))
                perf_evlist__enable(top->evlist);

	/* Wait for a minimal set of events before starting the snapshot */
	poll(top->evlist->pollfd, top->evlist->nr_fds, 100);

	perf_top__mmap_read(top);

	ret = -1;
	//if (pthread_create(&thread, NULL, (use_browser > 0 ? display_thread_tui :
							    //display_thread), top)) {
		//ui__error("Could not create display thread.\n");
		//goto out_delete;
	//}

	if (top->realtime_prio) {
		struct sched_param param;

		param.sched_priority = top->realtime_prio;
		if (sched_setscheduler(0, SCHED_FIFO, &param)) {
			ui__error("Could not set realtime priority.\n");
			goto out_delete;
		}
	}

	while (!done) {
		u64 hits = top->samples;

		perf_top__mmap_read(top);

		if (hits == top->samples)
			ret = poll(top->evlist->pollfd, top->evlist->nr_fds, 100);
		
		if(top->numa_metrics->timer_up)
			done=1;
	}

	ret = 0;
out_delete:
	
	
	kill(top->numa_metrics->pid_uo,9);
	
	printf("\n Report file %s \n", top->numa_metrics->report_filename);
	perf_session__delete(top->session);
	top->session = NULL;

	return ret;
}

static int
callchain_opt(const struct option *opt, const char *arg, int unset)
{
	symbol_conf.use_callchain = true;
	return record_callchain_opt(opt, arg, unset);
}

static int
parse_callchain_opt(const struct option *opt, const char *arg, int unset)
{
	symbol_conf.use_callchain = true;
	return record_parse_callchain_opt(opt, arg, unset);
}

static int
parse_percent_limit(const struct option *opt, const char *arg,
		    int unset __maybe_unused)
{
	struct perf_top *top = opt->value;

	top->min_percent = strtof(arg, NULL);
	return 0;
}

struct perf_top* cmd_top(int argc, const char **argv, const char *prefix __maybe_unused)
{
	int status = -1;
	char errbuf[BUFSIZ];
	struct perf_top top = {
		.count_filter	     = 5,
		.delay_secs	     = 2,
		.numa_sensing_time	= 0,
		.numa_migrate_mode =false,
		.migrate_chunk_size = -1,
		.record_opts = {
			.mmap_pages	= UINT_MAX,
			.user_freq	= UINT_MAX,
			.user_interval	= ULLONG_MAX,
			.freq		= 4000, /* 4 KHz */
			.target		= {
				.uses_mmap   = true,
			},
		},
		.max_stack	     = PERF_MAX_STACK_DEPTH,
		.sym_pcnt_filter     = 5,
	};
	struct perf_top *rtn=malloc(sizeof(struct perf_top));
	struct record_opts *opts = &top.record_opts;
	struct target *target = &opts->target;
	const struct option options[] = {
	OPT_CALLBACK('e', "event", &top.evlist, "event",
		     "event selector. use 'perf list' to list available events",
		     parse_events_option),
	OPT_U64('c', "count", &opts->user_interval, "event period to sample"),
	OPT_STRING('p', "pid", &target->pid, "pid",
		    "profile events on existing process id"),
	OPT_STRING('t', "tid", &target->tid, "tid",
		    "profile events on existing thread id"),
	OPT_BOOLEAN('a', "all-cpus", &target->system_wide,
			    "system-wide collection from all CPUs"),
	OPT_STRING('C', "cpu", &target->cpu_list, "cpu",
		    "list of cpus to monitor"),
	OPT_STRING('k', "vmlinux", &symbol_conf.vmlinux_name,
		   "file", "vmlinux pathname"),
	OPT_BOOLEAN(0, "ignore-vmlinux", &symbol_conf.ignore_vmlinux,
		    "don't load vmlinux even if found"),
	OPT_BOOLEAN('K', "hide_kernel_symbols", &top.hide_kernel_symbols,
		    "hide kernel symbols"),
	OPT_CALLBACK('m', "mmap-pages", &opts->mmap_pages, "pages",
		     "number of mmap data pages",
		     perf_evlist__parse_mmap_pages),
	OPT_INTEGER('0', "realtime", &top.realtime_prio,
		    "collect data with this RT SCHED_FIFO priority"),
	OPT_INTEGER('d', "delay", &top.delay_secs,
		    "number of seconds to delay between refreshes"),
	OPT_BOOLEAN('D', "dump-symtab", &top.dump_symtab,
			    "dump the symbol table used for profiling"),
	OPT_INTEGER('f', "count-filter", &top.count_filter,
		    "only display functions with more events than this"),
	OPT_BOOLEAN(0, "group", &opts->group,
			    "put the counters into a counter group"),
	OPT_BOOLEAN('0', "no-inherit", &opts->no_inherit,
		    "child tasks do not inherit counters"),
	OPT_STRING(0, "sym-annotate", &top.sym_filter, "symbol name",
		    "symbol to annotate"),
	OPT_BOOLEAN('z', "zero", &top.zero, "zero history across updates"),
	OPT_UINTEGER('F', "freq", &opts->user_freq, "profile at this frequency"),
	OPT_INTEGER('E', "entries", &top.print_entries,
		    "display this many functions"),
	OPT_BOOLEAN('U', "hide_user_symbols", &top.hide_user_symbols,
		    "hide user symbols"),
	OPT_BOOLEAN(0, "tui", &top.use_tui, "Use the TUI interface"),
	OPT_BOOLEAN(0, "stdio", &top.use_stdio, "Use the stdio interface"),
	OPT_INCR('v', "verbose", &verbose,
		    "be more verbose (show counter open errors, etc)"),
	OPT_STRING('s', "sort", &sort_order, "key[,key2...]",
		   "sort by key(s): pid, comm, dso, symbol, parent, weight, local_weight,"
		   " abort, in_tx, transaction"),
	OPT_BOOLEAN('n', "show-nr-samples", &symbol_conf.show_nr_samples,
		    "Show a column with the number of samples"),
	OPT_CALLBACK_NOOPT('g', NULL, &top.record_opts,
			   NULL, "enables call-graph recording",
			   &callchain_opt),
	OPT_CALLBACK(0, "call-graph", &top.record_opts,
		     "mode[,dump_size]", record_callchain_help,
		     &parse_callchain_opt),
	OPT_INTEGER(0, "max-stack", &top.max_stack,
		    "Set the maximum stack depth when parsing the callchain. "
		    "Default: " __stringify(PERF_MAX_STACK_DEPTH)),
	OPT_CALLBACK(0, "ignore-callees", NULL, "regex",
		   "ignore callees of these functions in call graphs",
		   report_parse_ignore_callees_opt),
	OPT_BOOLEAN(0, "show-total-period", &symbol_conf.show_total_period,
		    "Show a column with the sum of periods"),
	OPT_STRING(0, "dsos", &symbol_conf.dso_list_str, "dso[,dso...]",
		   "only consider symbols in these dsos"),
	OPT_STRING(0, "comms", &symbol_conf.comm_list_str, "comm[,comm...]",
		   "only consider symbols in these comms"),
	OPT_STRING(0, "symbols", &symbol_conf.sym_list_str, "symbol[,symbol...]",
		   "only consider these symbols"),
	OPT_BOOLEAN(0, "source", &symbol_conf.annotate_src,
		    "Interleave source code with assembly code (default)"),
	OPT_BOOLEAN(0, "asm-raw", &symbol_conf.annotate_asm_raw,
		    "Display raw encoding of assembly instructions (default)"),
	OPT_STRING(0, "objdump", &objdump_path, "path",
		    "objdump binary to use for disassembly and annotations"),
	OPT_STRING('M', "disassembler-style", &disassembler_style, "disassembler style",
		   "Specify disassembler style (e.g. -M intel for intel syntax)"),
	OPT_BOOLEAN('0', "numa-migrate", &top.numa_migrate_mode,
			    "Enable the activation of the numa migrate node"),
	OPT_INTEGER('0', "weighmin", &top.record_opts.weight_min_threshold,
		    "Specifies a minimum threshold for the load latency weight"),
	OPT_INTEGER('0', "numa-repdetail", &top.numa_migrate_logdetail,
		    "Specifies the detail of information to show"),
	OPT_INTEGER('0', "npid", &top.numa_migrate_pid_filter,
		    "Process id that numa-an will watch"),  
	OPT_INTEGER('0', "sensing-time", &top.numa_sensing_time,
		    "Specifies for how long the first sensing phase will execute for"),  
	OPT_BOOLEAN('0', "print-filereport", &top.migrate_filereport,
			    "Generate a file report with the migration statistics"),   
	OPT_STRING('u', "uid", &target->uid_str, "user", "user to profile"),
	OPT_STRING('0', "file-label", &(top.numa_filelabel), "numa-mig-rpt", "prints the label that will be attached to the file"),
	OPT_CALLBACK(0, "percent-limit", &top, "percent",
		     "Don't show entries under that percent", parse_percent_limit),
	OPT_BOOLEAN('0', "mig-just-measure", &top.migrate_just_measure,
		     "Disable the page migration and just gather statistics"),
	OPT_BOOLEAN('0', "track-accesslvls", &top.migrate_track_levels,
		     "In combination with numa-migrate keeps track of the number of accesses per access level "),
	OPT_INTEGER('0', "migrate-chunk", &top.migrate_chunk_size,
		    "Specifies the number of pages that will be simultaneously migrated"), 

	OPT_END()
	};
	const char * const top_usage[] = {
		"perf top [<options>]",
		NULL
	};

	top.command_string=get_command_string(argv,  argc);
	top.evlist = perf_evlist__new();
	if (top.evlist == NULL)
		return NULL;

	argc = parse_options(argc, argv, options, top_usage, PARSE_OPT_STOP_AT_NON_OPTION);

	//will assume that the remaining strings represent the command to launch
	
		top.command2_launch = argv;
		top.argv_size=argc;

			

	if (sort_order == default_sort_order)
		sort_order = "dso,symbol";

	if (setup_sorting() < 0) {
		parse_options_usage(top_usage, options, "s", 1);
		goto out_delete_evlist;
	}

	/* display thread wants entries to be collapsed in a different tree */
	sort__need_collapse = 1;

	if (top.use_stdio)
		use_browser = 0;
	else if (top.use_tui)
		use_browser = 1;

	setup_browser(false);

	status = target__validate(target);
	if (status) {
		target__strerror(target, status, errbuf, BUFSIZ);
		ui__warning("%s\n", errbuf);
	}

	status = target__parse_uid(target);
	if (status) {
		int saved_errno = errno;

		target__strerror(target, status, errbuf, BUFSIZ);
		ui__error("%s\n", errbuf);

		status = -saved_errno;
		goto out_delete_evlist;
	}

	if (target__none(target))
		target->system_wide = true;

	if (perf_evlist__create_maps(top.evlist, target) < 0)
		usage_with_options(top_usage, options);

	if (!top.evlist->nr_entries &&
	    perf_evlist__add_default(top.evlist) < 0) {
		ui__error("Not enough memory for event selector list\n");
		goto out_delete_evlist;
	}

	symbol_conf.nr_events = top.evlist->nr_entries;

	if (top.delay_secs < 1)
		top.delay_secs = 1;

	if (record_opts__config(opts)) {
		status = -EINVAL;
		goto out_delete_evlist;
	}

	top.sym_evsel = perf_evlist__first(top.evlist);

	symbol_conf.priv_size = sizeof(struct annotation);

	symbol_conf.try_vmlinux_path = (symbol_conf.vmlinux_name == NULL);
	if (symbol__init() < 0)
		return NULL;

	sort__setup_elide(stdout);

	get_term_dimensions(&top.winsize);
	if (top.print_entries == 0) {
		struct sigaction act = {
			.sa_sigaction = perf_top__sig_winch,
			.sa_flags     = SA_SIGINFO,
		};
		perf_top__update_print_entries(&top);
		sigaction(SIGWINCH, &act, NULL);
	}
	if (top.numa_migrate_mode==true){
		top.record_opts.sample_weight=true;
		top.record_opts.sample_address=true;
	}
	//commented due to change in methos
	//status = __cmd_top(&top);
	top.numa_analysis_enabled=true;
out_delete_evlist:
	//commented due to chage in method
	//perf_evlist__delete(top.evlist);
	memcpy(rtn,&top,sizeof(struct perf_top));
	return rtn;
}
