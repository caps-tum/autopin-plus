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

#include <AutopinPlus/Strategy/History/Main.h>

namespace AutopinPlus {
namespace Strategy {
namespace History {

Main::Main(const Configuration &config, const ObservedProcess &proc, OSServices &service,
		   const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: ControlStrategy(config, proc, service, monitors, context), current_pinning(0), best_pinning(-1),
	  notifications(false) {

	// Setup timer
	init_timer.setSingleShot(true);
	connect(&init_timer, SIGNAL(timeout()), this, SLOT(slot_startPinning()));

	this->name = "history";
}

void Main::init() {
	ControlStrategy::init();

	context.info("Initializing control strategy history");

	// Set standard values
	init_time = 15;
	openmp_icc = false;
	skip.clear();
	notification_interval = 0;

	// get data from history
	// check if generated from autopin1
	if (history->getStrategy() != "autopin1")
		context.report(Error::BAD_CONFIG, "invalid_value", "Invalid strategy in history file");

	// Read user values from the configuration
	auto it = history->getStrategyOption("init_time");
	if (it != history->getStrategyOptions().end()) init_time = it->second.first().toInt();

	it = history->getStrategyOption("openmp_icc");
	if (it != history->getStrategyOptions().end()) openmp_icc = it->second.first() == "true";

	it = history->getStrategyOption("skip");
	if (it != history->getStrategyOptions().end()) skip_str = it->second;

	it = history->getStrategyOption("notification_interval");
	if (it != history->getStrategyOptions().end()) notification_interval = it->second.first().toInt();

	for (int i = 0; i < skip_str.size(); i++) {
		QString entry = skip_str[i];
		bool ok;
		int entry_int = entry.toInt(&ok);
		if (!ok) context.report(Error::BAD_CONFIG, "invalid_value", "Invalid id for skipped thread: " + entry);
		skip.insert(entry_int);
	}

	if (init_time < 0)
		context.report(Error::BAD_CONFIG, "invalid_value", "Invalid init time: " + QString::number(init_time));

	context.info("  - Init time: " + QString::number(init_time));
	if (openmp_icc) context.info("  - OpenMP/ICC support is enabled");
	if (!skip.empty()) context.info("  - These tasks will be skipped: " + skip_str.join(" "));

	if (proc.getCommChanAddr() != "")
		context.info("  - Minimum phase notification interval: " + QString::number(notification_interval));

	init_timer.setInterval(init_time * 1000);
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(Configuration::configopt("init_time", QStringList(QString::number(init_time))));

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

	context.info("");
	context.info("Apply pinning.");

	applyPinning(history->getBestPinning(0).first);
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

		pinned_tasks.erase(it);
	}
}

void Main::slot_PhaseChanged(int newphase) {
	// Restart the current pinning if a new execution phase
	// is reported and notifications are enabled

	if (notifications) {
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
			return true;
		}
		return false;
	});

	pinned_tasks.erase(new_end, pinned_tasks.end());

	if (terminated_tasks.size() > 0) context.info("Terminated tasks: " + terminated_tasks.join(" "));

	if (pinned_tasks.empty()) context.report(Error::STRATEGY, "no_task", "All pinned tasks have terminated");
}

} // namespace History
} // namespace Strategy
} // namespace AutopinPlus
