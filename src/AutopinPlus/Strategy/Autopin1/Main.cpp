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

#include <AutopinPlus/Strategy/Autopin1/Main.h>

namespace AutopinPlus {
namespace Strategy {
namespace Autopin1 {

Main::Main(const Configuration &config, const ObservedProcess &proc, OS::Linux::OSServicesLinux &service,
		   const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: ControlStrategy(config, proc, service, monitors, context), current_pinning(0), best_pinning(-1), monitor(nullptr),
	  notifications(false) {
	// Setup timers
	init_timer.setSingleShot(true);
	connect(&init_timer, SIGNAL(timeout()), this, SLOT(slot_startPinning()));

	warmup_timer.setSingleShot(true);
	connect(&warmup_timer, SIGNAL(timeout()), this, SLOT(slot_startMonitor()));

	measure_timer.setSingleShot(true);
	connect(&measure_timer, SIGNAL(timeout()), this, SLOT(slot_stopPinning()));

	this->name = "autopin1";
}

void Main::init() {

	ControlStrategy::init();

	context.info("Initializing control strategy autopin1");

	QString config_prefix = "autopin1.";

	// Read the pinnings from the configuration
	pinnings = readPinnings(config_prefix + "schedule");

	// Select a performance monitor
	if (!monitors.empty() && (*monitors.begin())->getValType() != PerformanceMonitor::UNKNOWN) {
		monitor = monitors.begin()->get();
		monitor_type = monitor->getValType();
		context.info("  - Using performance monitor \"" + monitor->getName() + "\"");
	} else
		context.report(Error::BAD_CONFIG, "no_monitor", "No performance monitor found!");

	// Set standard values
	init_time = 15;
	warmup_time = 15;
	measure_time = 30;
	openmp_icc = false;
	skip.clear();
	notification_interval = 0;

	// Read user values from the configuration
	if (config.configOptionExists(config_prefix + "init_time") > 0)
		init_time = config.getConfigOptionInt(config_prefix + "init_time");

	if (config.configOptionExists(config_prefix + "warmup_time") > 0)
		warmup_time = config.getConfigOptionInt(config_prefix + "warmup_time");

	if (config.configOptionExists(config_prefix + "measure_time") > 0)
		measure_time = config.getConfigOptionInt(config_prefix + "measure_time");

	if (config.configOptionBool(config_prefix + "openmp_icc"))
		openmp_icc = config.getConfigOptionBool(config_prefix + "openmp_icc");

	if (config.configOptionExists(config_prefix + "skip") > 0)
		skip_str = config.getConfigOptionList(config_prefix + "skip");

	if (config.configOptionExists(config_prefix + "notification_interval") > 0)
		notification_interval = config.getConfigOptionInt(config_prefix + "notification_interval");

	for (int i = 0; i < skip_str.size(); i++) {
		QString entry = skip_str[i];
		bool ok;
		int entry_int = entry.toInt(&ok);
		if (!ok) context.report(Error::BAD_CONFIG, "invalid_value", "Invalid id for skipped thread: " + entry);
		skip.insert(entry_int);
	}

	if (init_time < 0)
		context.report(Error::BAD_CONFIG, "invalid_value", "Invalid init time: " + QString::number(init_time));
	if (warmup_time < 0)
		context.report(Error::BAD_CONFIG, "invalid_value", "Invalid warmup time: " + QString::number(warmup_time));
	if (measure_time <= 0)
		context.report(Error::BAD_CONFIG, "invalid_value", "Invalid measure time: " + QString::number(measure_time));

	context.info("  - Init time: " + QString::number(init_time));
	context.info("  - Warmup time: " + QString::number(warmup_time));
	context.info("  - Measure time: " + QString::number(measure_time));
	if (openmp_icc) context.info("  - OpenMP/ICC support is enabled");
	if (!skip.empty()) context.info("  - These tasks will be skipped: " + skip_str.join(" "));

	if (proc.getCommChanAddr() != "")
		context.info("  - Minimum phase notification interval: " + QString::number(notification_interval));

	init_timer.setInterval(init_time * 1000);
	warmup_timer.setInterval(warmup_time * 1000);
	measure_timer.setInterval(measure_time * 1000);
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(Configuration::configopt("init_time", QStringList(QString::number(init_time))));
	result.push_back(Configuration::configopt("warmup_time", QStringList(QString::number(warmup_time))));
	result.push_back(Configuration::configopt("measure_time", QStringList(QString::number(measure_time))));

	if (openmp_icc)
		result.push_back(Configuration::configopt("openmp_icc", QStringList("true")));
	else
		result.push_back(Configuration::configopt("openmp_icc", QStringList("false")));

	result.push_back(Configuration::configopt("skip", skip_str));

	result.push_back(
		Configuration::configopt("notification_interval", QStringList(QString::number(notification_interval))));

	return result;
}

void Main::slot_autopinReady() {
	context.info("Set phase notification interval");
	proc.setPhaseNotificationInterval(notification_interval);

	context.info("Waiting " + QString::number(init_time) + " seconds (init time)");

	// Start timer
	init_timer.start();
}

void Main::slot_startPinning() {
	// Refresh the list with tasks
	refreshTasks();

	// Enable notifications for new tasks
	notifications = true;

	PinningHistory::autopin_pinning &new_pinning = pinnings[current_pinning];
	context.info("");
	QString msg =
		"Test pinning " + QString::number(current_pinning + 1) + " of " + QString::number(pinnings.size()) + ":";

	for (auto &elem : new_pinning) msg += " " + QString::number(elem);

	context.info(msg);

	// Pin threads
	applyPinning(new_pinning);

	// Start timer
	context.info("Waiting " + QString::number(warmup_time) + " seconds (warmup time)");

	warmup_timer.start();
}

void Main::slot_startMonitor() {
	// Start timer
	measure_start.invalidate();
	measure_start.start();

	context.info("Start performance monitoring");
	checkPinnedTasks();

	for (auto it = pinned_tasks.begin(); it != pinned_tasks.end(); it++) {
		monitor->start(it->tid);
		it->start = measure_start.elapsed();
		it->stop = -1;
	}

	// Start timer
	context.info("Waiting " + QString::number(measure_time) + " seconds (measure time)");

	measure_timer.start();
}

void Main::slot_stopPinning() {
	notifications = false;

	context.info("Reading results of pinning " + QString::number(current_pinning + 1));
	checkPinnedTasks();

	double current_result = 0;

	for (auto &elem : pinned_tasks) {
		if (elem.stop == -1) {
			elem.stop = measure_start.elapsed();
			elem.result = monitor->stop(elem.tid);
		}
		double tres = elem.result;
		elem.result = tres / (elem.stop - elem.start);
		context.info("  :: Result for task " + QString::number(elem.tid) + ": " + QString::number(elem.result));
		current_result += elem.result;
	}

	current_result = current_result / pinned_tasks.size();

	switch (monitor_type) {
	case PerformanceMonitor::montype::MAX:
		if (best_pinning == -1) {
			best_performance = current_result;
			best_pinning = current_pinning;
		} else if (current_result > best_performance) {
			best_performance = current_result;
			best_pinning = current_pinning;
		}
		break;
	case PerformanceMonitor::montype::MIN:
		if (best_pinning == -1) {
			best_performance = current_result;
			best_pinning = current_pinning;
		} else if (current_result < best_performance) {
			best_performance = current_result;
			best_pinning = current_pinning;
		}
		break;
	default:
		// This shouldn't happen, we explicitely checked them monitor type in the init() function.
		context.report(Error::UNSUPPORTED, "critical",
					   name + ".slot_stopPinning(): Performance monitor has unknown type.");
		return;
	}

	context.info("Result of pinning " + QString::number(current_pinning + 1) + ": " + QString::number(current_result));
	addPinningToHistory(pinnings[current_pinning], current_result);
	pinned_tasks.clear();

	current_pinning++;
	if ((uint)current_pinning < pinnings.size()) {
		QTimer::singleShot(0, this, SLOT(slot_startPinning()));
	} else {
		context.info("");
		context.info("All pinnings have been tested");
		context.info("Applying best pinning: " + QString::number(best_pinning + 1));
		refreshTasks();
		applyPinning(pinnings[best_pinning]);
		context.info("Control strategy autopin1 has finished");
		if (history != nullptr) history->deinit();
	}
}

void Main::slot_TaskCreated(int tid) {
	// Only pin new tasks when the measurement is currently running
	if (notifications) {
		auto it = std::find_if(pinned_tasks.begin(), pinned_tasks.end(),
							   [tid](const pinned_task &t) { return t.tid == tid; });

		if (it != pinned_tasks.end()) return;

		unsigned int i = pinned_tasks.size();
		PinningHistory::autopin_pinning pinning = pinnings[current_pinning];
		if (i < pinning.size()) {
			int j = tasks.size();
			if (skip.find(j) != skip.end()) {
				context.info("  :: Not pinning task " + QString::number(tid) + " (skipped)");
			} else if (j == 1 && openmp_icc) {
				context.info("  :: Not pinning task " + QString::number(tid) + " (icc thread)");
			} else {
				context.info("  :: Pinning task " + QString::number(tid) + " to core " + QString::number(pinning[i]));
				service.setAffinity(tid, pinning[i]);

				pinned_task new_entry;
				new_entry.tid = tid;

				// Start monitor if measurement is already running
				if (measure_timer.isActive()) {
					monitor->start(tid);
					new_entry.start = measure_start.elapsed();
					new_entry.stop = -1;
				}

				pinned_tasks.push_back(new_entry);
			}
		}
		tasks.push_back(tid);
	}
}

void Main::slot_TaskTerminated(int tid) {
	if (notifications) {
		auto it = std::find_if(pinned_tasks.begin(), pinned_tasks.end(),
							   [tid](const pinned_task &t) { return t.tid == tid; });

		if (it == pinned_tasks.end()) return;

		it->result = monitor->stop(tid);

		if (measure_timer.isActive()) {
			it->stop = measure_start.elapsed();
		} else {
			pinned_tasks.erase(it);
		}
	}
}

void Main::slot_PhaseChanged(int newphase) {
	// Restart the current pinning if a new execution phase
	// is reported and notifications are enabled

	if (notifications) {
		// Stop all timers
		warmup_timer.stop();
		measure_timer.stop();

		// Clear the list of pinned tasks
		pinned_tasks.clear();

		context.info("");
		context.info("New execution phase - restart the current pinning");

		notifications = false;

		QTimer::singleShot(0, this, SLOT(slot_startPinning()));
	}
}

void Main::applyPinning(PinningHistory::autopin_pinning pinning) {
	// i counts the pinnings
	// j counts the tasks
	unsigned int i = 0, j = 0;
	while (i < pinning.size() && j < tasks.size()) {
		if (skip.find(j) != skip.end()) {
			context.info("  :: Not pinning task " + QString::number(tasks[j]) + " (skipped)");
		} else if (j == 1 && openmp_icc) {
			context.info("  :: Not pinning task " + QString::number(tasks[j]) + " (icc thread)");
		} else {
			pinned_task new_entry;
			new_entry.tid = tasks[j];
			context.info("  :: Pinning task " + QString::number(tasks[j]) + " to core " + QString::number(pinning[i]));
			service.setAffinity(tasks[j], pinning[i]);
			pinned_tasks.push_back(new_entry);
			i++;
		}

		j++;
	}
}

void Main::checkPinnedTasks() {
	// There is no need to check for terminated tasks if process tracing is enabled
	if (proc.getTrace()) return;

	ProcessTree proc_tree;
	ProcessTree::autopin_tid_list running_tasks;
	QStringList terminated_tasks;

	proc_tree = proc.getProcessTree();

	running_tasks = proc_tree.getAllTasks();

	auto new_end = std::remove_if(pinned_tasks.begin(), pinned_tasks.end(), [&](const pinned_task &t) {
		const int tid = t.tid;

		if (running_tasks.find(tid) == running_tasks.end()) {
			terminated_tasks.push_back(QString::number(tid));
			monitor->clear(tid);
			return true;
		}
		return false;
	});

	pinned_tasks.erase(new_end, pinned_tasks.end());

	if (terminated_tasks.size() > 0) context.info("Terminated tasks: " + terminated_tasks.join(" "));

	if (pinned_tasks.empty()) context.report(Error::STRATEGY, "no_task", "All pinned tasks have terminated");
}

} // namespace Autopin1
} // namespace Strategy
} // namespace AutopinPlus
