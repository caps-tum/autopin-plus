/*
 * This file is part of Autopin+.
 *
 * Copyright (C) 2015 Lukas Fürmetz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AutopinPlus/Strategy/Scatter/Main.h>

#include <AutopinPlus/Error.h>	 // for Error, Error::::BAD_CONFIG, etc
#include <AutopinPlus/Exception.h> // for Exception
#include <AutopinPlus/Tools.h>	 // for Tools
#include <QString>				   // for operator+, QString

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

ControlStrategy::Pinning Main::getPinning(const Pinning &current_pinning) const {
	if (new_task_tid == 0) return current_pinning;

	Pinning result = current_pinning;
	int pid = proc.getPid();

	uint max_distance = 0;
	uint min_balance = result.size();
	int pin_cpu_pos = -1;

	for (uint i = 0; i < result.size(); i++) {
		Task task = current_pinning[i];
		if (task.isCpuFree()) {
			if (pin_cpu_pos == -1) pin_cpu_pos = i;

			// Search forward for the next pinned task.
			uint distance_forward = 0;
			for (uint j = i + 1; j < result.size(); j++) {
				if (current_pinning[j].pid == pid) {
					distance_forward = j - i;
					break;
				}
			}

			uint distance_backward = 0;
			// Search backward for the last pinned task.
			for (int w = i - 1; w >= 0; w--) {
				if (current_pinning[w].pid == pid) {
					distance_backward = i - w;
					break;
				}
			}

			// If the distance is than the current max distance,
			// overwrite the pinning.
			//
			// If the distance equals the max distance, than the
			// current balance between the forward distance and the
			// backward distance comes into account.  This is
			// necessary, so a free cpu with both a forward distance
			// and a backwards distance of 2 are preferred to a free
			// cpu with forward distance of 4 and a backward distance
			// of 0.
			uint distance = distance_forward + distance_backward;
			uint balance = std::abs((int)(distance_forward - distance_backward));
			if ((distance > max_distance) || ((distance == max_distance) && (balance < min_balance))) {
				max_distance = distance;
				min_balance = balance;
				pin_cpu_pos = i;
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
} // namespace Scatter
} // namespace Strategy
} // namespace AutopinPlus
