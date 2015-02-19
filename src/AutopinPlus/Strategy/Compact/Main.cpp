/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Strategy/Compact/Main.h>

#include <AutopinPlus/Error.h>	 // for Error, Error::::BAD_CONFIG, etc
#include <AutopinPlus/Exception.h> // for Exception
#include <AutopinPlus/Tools.h>	 // for Tools
#include <QString>				   // for operator+, QString
#include <AutopinPlus/OS/CpuInfo.h>

namespace CpuInfo = AutopinPlus::OS::CpuInfo;

namespace AutopinPlus {
namespace Strategy {
namespace Compact {

Main::Main(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
		   const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: ControlStrategy(config, proc, service, monitors, context) {
	name = "compact";
}

void Main::init() {
	ControlStrategy::init();

	context.info("Initializing control strategy " + name);

	// Read and parse the "interval" option
	if (config.configOptionExists(name + ".interval") > 0) {
		try {
			interval = Tools::readInt(config.getConfigOption(name + ".interval"));
			context.info("  - " + name + ".interval = " + QString::number(interval));
		} catch (const Exception &e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'interval' option.");
			return;
		}
	}
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

ControlStrategy::Pinning Main::getPinning(const Pinning &current_pinning) {
	if (new_task_tid == 0) return current_pinning;

	Pinning result = current_pinning;
	int pid = proc.getPid();

	uint min_distance = result.size();
	int pin_cpu_pos = -1;

	for (uint i = 0; i < result.size(); i++) {
		Task task = current_pinning[i];
		if (task.isCpuFree()) {
			if (pin_cpu_pos == -1) pin_cpu_pos = i;

			for (uint j = 0; j < result.size(); j++) {
				if (current_pinning[j].pid == pid) {
					uint distance = CpuInfo::getCpuDistance(i, j);
					if (distance < min_distance) {
						min_distance = distance;
						pin_cpu_pos = i;
					}
				}
			}
		}
	}

	if (pin_cpu_pos != -1) {
		Task t;
		t.pid = pid;
		t.tid = new_task_tid;
		result[pin_cpu_pos] = t;
	}

	return result;
}
} // namespace Compact
} // namespace Strategy
} // namespace AutopinPlus
