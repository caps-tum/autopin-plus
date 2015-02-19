/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/PerformanceMonitor.h>

#include <AutopinPlus/Error.h>
#include <AutopinPlus/Exception.h> // for Exception
#include <utility>				   // for pair

namespace AutopinPlus {

PerformanceMonitor::PerformanceMonitor(QString name, const Configuration &config, AutopinContext &context)
	: config(config), context(context), valtype(UNKNOWN), name(name) {}

PerformanceMonitor::~PerformanceMonitor() {}

PerformanceMonitor::montype PerformanceMonitor::getValType() { return valtype; }

QString PerformanceMonitor::getType() { return type; }

QString PerformanceMonitor::getName() { return name; }

QString PerformanceMonitor::getUnit() { return ""; }

void PerformanceMonitor::start(ProcessTree::autopin_tid_list tasks) {
	for (const auto &task : tasks) {
		start(task);
	}
}

PerformanceMonitor::autopin_measurements PerformanceMonitor::value(ProcessTree::autopin_tid_list tasks) {
	autopin_measurements result;

	for (const auto &task : tasks) {
		std::pair<int, double> respair;
		respair.first = task;
		respair.second = value(task);
		if (context.isError()) {
			return result;
		}

		result.insert(respair);
	}

	return result;
}

PerformanceMonitor::autopin_measurements PerformanceMonitor::stop(ProcessTree::autopin_tid_list tasks) {
	autopin_measurements result;

	for (const auto &task : tasks) {
		std::pair<int, double> respair;
		respair.first = task;
		respair.second = stop(task);
		if (context.isError()) {
			return result;
		}

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
	result = stop(tasks);
	if (context.isError()) {
		return result;
	}

	return result;
}

void PerformanceMonitor::clear() {
	ProcessTree::autopin_tid_list tasks;
	autopin_measurements result;

	tasks = getMonitoredTasks();
	clear(tasks);
}

PerformanceMonitor::montype PerformanceMonitor::readMontype(const QString &string) {
	PerformanceMonitor::montype result;

	if (string.toLower() == "max") {
		result = montype::MAX;
	} else if (string.toLower() == "min") {
		result = montype::MIN;
	} else if (string.toLower() == "unknown") {
		result = montype::UNKNOWN;
	} else {
		throw Exception("PerformanceMonitor::readMontype(" + string +
						") failed: Must be one of 'MAX', 'MIN', 'UNKNOWN'.");
	}

	return result;
}

QString PerformanceMonitor::showMontype(const montype &type) {
	QString result;

	switch (type) {
	case montype::MAX:
		result = "MAX";
		break;
	case montype::MIN:
		result = "MIN";
		break;
	case montype::UNKNOWN:
		result = "UNKNOWN";
		break;
	default:
		throw Exception("PerformanceMonitor::showMontype(" + QString::number(type) + ") failed: Invalid montype.");
	}

	return result;
}

} // namespace AutopinPlus
