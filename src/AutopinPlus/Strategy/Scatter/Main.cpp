/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Strategy/Scatter/Main.h>

#include <AutopinPlus/Error.h>	 // for Error, Error::::BAD_CONFIG, etc
#include <AutopinPlus/Exception.h> // for Exception
#include <AutopinPlus/Tools.h>	 // for Tools
#include <QString>				   // for operator+, QString
#include <AutopinPlus/OS/CpuInfo.h>

namespace CpuInfo = AutopinPlus::OS::CpuInfo;

namespace AutopinPlus {
namespace Strategy {
namespace Scatter {

Main::Main(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
		   const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: ControlStrategy(config, proc, service, monitors, context) {
	name = "scatter";
}

void Main::init() {
	ControlStrategy::init();

	context.info("Initializing control strategy " + name);

	// Read and parse the "interval" option
	if (config.configOptionExists(name + ".interval") > 0) {
		try {
			interval = Tools::readInt(config.getConfigOption(name + ".interval"));
			context.info("  - " + name + ".interval = " + QString::number(interval));
		} catch (const Exception &) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'interval' option.");
			return;
		}
	}

	for (int node = 0; node < CpuInfo::getNodeCount(); node++) pinCount.push_back(0);
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(Configuration::configopt("interval", QStringList(QString::number(interval))));

	return result;
}

void Main::slot_TaskCreated(int tid) {
	new_task_tid = tid;
	// We received a signal telling us that a new task is created, so
	// we need to update the pinning.
	ControlStrategy::slot_TaskCreated(tid);
	new_task_tid = 0;
}

void Main::slot_TaskTerminated(int tid) {
	int cpu = getCpuByTask(tid);
	int node = CpuInfo::getNodeByCpu(cpu);
	pinCount[node]--;
	ControlStrategy::slot_TaskTerminated(tid);
}

ControlStrategy::Pinning Main::getPinning(const Pinning &current_pinning) {
	if (new_task_tid == 0) return current_pinning;

	Pinning result = current_pinning;

	int pin_cpu_pos = -1;
	for (auto node : sort_indexes(pinCount)) {
		auto cpus = CpuInfo::getCpusByNode(node);
		for (auto cpu : cpus) {
			Task task = current_pinning[cpu];
			if (task.isCpuFree()) {
				pin_cpu_pos = cpu;
				pinCount[node]++;
				goto LoopEnd;
			}
		}
	}
LoopEnd:

	if (pin_cpu_pos != -1) {
		Task t;
		t.pid = proc.getPid();
		t.tid = new_task_tid;
		result[pin_cpu_pos] = t;
	}

	return result;
}

} // namespace Scatter
} // namespace Strategy
} // namespace AutopinPlus
