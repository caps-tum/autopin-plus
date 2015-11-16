/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Monitor/PageMigrate/Main.h>

#include <QTime>
#include <stdlib.h>
#include "spm.h"



namespace AutopinPlus {
namespace Monitor {
namespace PageMigrate {

Main::Main(QString name, const Configuration &config, AutopinContext &context)
	: PerformanceMonitor(name, config, context) {
	type = "page-migration";
	started=false;
	//parameters for the memory access sampling
	period=1000;
	min_weight=50;
	sensing_time=30;
}


	
void Main::init() {
	context.info("Initializing monitor \"" + name + "\" (pageMigrate)");

	//Must be left, this value is read by the strategy
	valtype = MAX;
	// Set standard values


	if (config.configOptionExists(name + ".sampling_period") == 1) period = config.getConfigOptionInt(name + ".sampling_period");
	if (config.configOptionExists(name + ".min_weight") == 1) min_weight = config.getConfigOptionInt(name + ".min_weight");
	if (config.configOptionExists(name + ".sensing_time") == 1) sensing_time = config.getConfigOptionInt(name + ".sensing_time");
	

}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	
	result.push_back(Configuration::configopt("valtype", QStringList("MAX")));
	return result;
}

void Main::start(int tid) { 
	struct sampling_settings st;

	memset(&st,0,sizeof(struct sampling_settings));
	context.info("My pid " + QString::number(monitored_pid));
	st.ll_sampling_period=period;
	st.ll_weight_threshold=min_weight;
	st.measure_time=sensing_time;
	st.pid_uo=monitored_pid;
	init_spm(&st);
}

double Main::value(int tid) {
	
	return tid;
}

double Main::stop(int tid) {
	double result = value(tid);
	if (context.isError()) {
		return 0;
	}


	return result;
}

void Main::clear(int tid) {
	if (rands.find(tid) != rands.end()) rands.erase(tid);
}

ProcessTree::autopin_tid_list Main::getMonitoredTasks() {
	ProcessTree::autopin_tid_list result;

	return result;
}


} // namespace PageMigrate
} // namespace Monitor
} // namespace AutopinPlus
