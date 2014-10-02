/*
 * Autopin+ - Automatic thread-to-core-pinning tool
 * Copyright (C) 2012 LRR
 *
 * Author:
 * Florian Walter
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact address:
 * LRR (I10)
 * Technische Universitaet Muenchen
 * Boltzmannstr. 3
 * D-85784 Garching b. Muenchen
 * http://autopin.in.tum.de
 */

#include <AutopinPlus/PerformanceMonitor.h>

namespace AutopinPlus {

PerformanceMonitor::PerformanceMonitor(QString name, Configuration *config, const AutopinContext &context)
	: config(config), context(context), valtype(UNKNOWN), name(name) {}

PerformanceMonitor::~PerformanceMonitor() {}

PerformanceMonitor::montype PerformanceMonitor::getValType() { return valtype; }

QString PerformanceMonitor::getType() { return type; }

QString PerformanceMonitor::getName() { return name; }

void PerformanceMonitor::start(ProcessTree::autopin_tid_list tasks) {
	for (const auto &task : tasks) {
		CHECK_ERRORV(start(task));
	}
}

PerformanceMonitor::autopin_measurements PerformanceMonitor::value(ProcessTree::autopin_tid_list tasks) {
	autopin_measurements result;

	for (const auto &task : tasks) {
		std::pair<int, double> respair;
		respair.first = task;
		CHECK_ERROR(respair.second = value(task), result);
		result.insert(respair);
	}

	return result;
}

PerformanceMonitor::autopin_measurements PerformanceMonitor::stop(ProcessTree::autopin_tid_list tasks) {
	autopin_measurements result;

	for (const auto &task : tasks) {
		std::pair<int, double> respair;
		respair.first = task;
		CHECK_ERROR(respair.second = stop(task), result);
		result.insert(respair);
	}

	return result;
}

void PerformanceMonitor::clear(ProcessTree::autopin_tid_list tasks) {
	for (const auto &task : tasks) clear(task);
}

PerformanceMonitor::autopin_measurements PerformanceMonitor::stop() {
	ProcessTree::autopin_tid_list tasks;
	autopin_measurements result;

	tasks = getMonitoredTasks();
	CHECK_ERROR(result = stop(tasks), result);

	return result;
}

void PerformanceMonitor::clear() {
	ProcessTree::autopin_tid_list tasks;
	autopin_measurements result;

	tasks = getMonitoredTasks();
	clear(tasks);
}

} // namespace AutopinPlus
