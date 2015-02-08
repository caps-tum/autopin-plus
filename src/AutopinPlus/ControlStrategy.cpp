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

#include <AutopinPlus/ControlStrategy.h>
#include <AutopinPlus/OS/OSServices.h>
#include <AutopinPlus/OS/CpuInfo.h>

#include <algorithm>
#include <QChar>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

namespace AutopinPlus {

ControlStrategy::Pinning ControlStrategy::pinning;
QMutex ControlStrategy::mutex;

ControlStrategy::ControlStrategy(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
								 const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: config(config), proc(proc), service(service), monitors(monitors), context(context), name("ControlStrategy") {}

const ControlStrategy::Task ControlStrategy::emptyTask = {0, 0};

bool ControlStrategy::Task::operator==(const ControlStrategy::Task &rhs) const {
	return (this->tid == rhs.tid) && (this->pid == rhs.pid);
}

bool ControlStrategy::Task::operator!=(const ControlStrategy::Task &rhs) const {
	return (this->tid != rhs.tid) || (this->pid != rhs.pid);
}

bool ControlStrategy::Task::isCpuFree() const { return (*this == emptyTask); }

void ControlStrategy::init() {
	if (pinning.empty()) {
		int cpuCount = OS::CpuInfo::getCpuCount();
		for (int i = 0; i < cpuCount; i++) pinning.push_back(emptyTask);
	}
}

QString ControlStrategy::getName() { return name; }

void ControlStrategy::slot_watchdogReady() {
	// Now that all initialization is done, ask the ObservedProcess if
	// tracing is supported.
	if (!proc.getTrace() && interval >= 1) {
		// Process tracing is not enabled, we therefore have to
		// periodically update the list of threads to monitor.
		context.debug(
			name +
			": Process tracing is disabled, falling back to regular polling for determining the threads to monitor.");
		connect(&timer, SIGNAL(timeout()), this, SLOT(slot_timer()));
		timer.setInterval(interval);
		timer.start();
	}
}

void ControlStrategy::slot_TaskCreated(int tid) {
	tasks.push_back(tid);
	changePinning();
}

void ControlStrategy::slot_TaskTerminated(int tid) {
	auto it_tasks = std::find(tasks.begin(), tasks.end(), tid);
	if (it_tasks != tasks.end()) tasks.erase(it_tasks);

	QMutexLocker ml(&mutex);
	Task t = {proc.getPid(), tid};
	auto it_pinning = std::find(pinning.begin(), pinning.end(), t);
	if (it_pinning != pinning.end()) *it_pinning = emptyTask;

	ml.unlock();
	changePinning();
}

void ControlStrategy::slot_PhaseChanged(int) {}
void ControlStrategy::slot_UserMessage(int, double) {}

ControlStrategy::Pinning ControlStrategy::getPinning(const Pinning &pinning) { return pinning; }

void ControlStrategy::changePinning() {
	QMutexLocker ml(&mutex);
	int pid = proc.getPid();
	Pinning new_pinning = getPinning(ControlStrategy::pinning);

	for (uint i = 0; i < pinning.size(); i++) {
		Task new_task = new_pinning[i];
		Task old_task = pinning[i];
		if (new_task != old_task) {
			// Test whether this ControlStrategy is allowed to
			// overwrite this pinning.
			if (!old_task.isCpuFree() && old_task.pid != pid) {
				context.report(Error::STRATEGY, "wrong_cpu",
							   "ControlStrategy." + name +
								   " tried to overwrite a pinning from another strategy. Bailing out!");
			} else {
				if (old_task.pid == pid) context.warn("ControlStrategy." + name + " is overwritting its own pinning!");
				int ret = service.setAffinity(new_task.tid, i);
				if (ret != 0) {
					context.report(Error::STRATEGY, "set_affinity", "Could not pin thread " +
																		QString::number(new_task.tid) + " to cpu " +
																		QString::number(i));
				} else {
					pinning[i] = new_task;
					context.info("Pinned task " + QString::number(new_task.tid) + " to cpu " + QString::number(i));
				}
			}
		}
	}
}

void ControlStrategy::refreshTasks() {
	ProcessTree ptree;

	ptree = proc.getProcessTree();
	ProcessTree::autopin_tid_list task_set = ptree.getAllTasks();

	tasks.clear();

	for (const auto &elem : task_set) {
		tasks.push_back(elem);
	}
}

void ControlStrategy::slot_timer() {
	QSet<int> old_tasks;
	QSet<int> new_tasks;

	for_each(tasks.begin(), tasks.end(), [&old_tasks](int tid) { old_tasks.insert(tid); });
	refreshTasks();
	for_each(tasks.begin(), tasks.end(), [&new_tasks](int tid) { new_tasks.insert(tid); });

	for (auto tid : old_tasks - new_tasks) slot_TaskTerminated(tid);

	for (auto tid : new_tasks - old_tasks) slot_TaskCreated(tid);
}

int ControlStrategy::getCpuByTask(int tid) {
	QMutexLocker ml(&mutex);
	Task t = {proc.getPid(), tid};

	for (uint cpu = 0; cpu < pinning.size(); cpu++) {
		if (t == pinning[cpu]) return cpu;
	}

	return -1;
}
} // namespace AutopinPlus
