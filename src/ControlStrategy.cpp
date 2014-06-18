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

#include "ControlStrategy.h"

#include <QChar>
#include <QString>
#include <QStringList>
#include <algorithm>

ControlStrategy::ControlStrategy(Configuration *config, ObservedProcess *proc, OSServices *service,
								 PerformanceMonitor::monitor_list monitors, PinningHistory *history,
								 const AutopinContext &context)
	: config(config), proc(proc), service(service), monitors(std::move(monitors)), history(history), context(context),
	  name("ControlStrategy") {}

QString ControlStrategy::getName() { return name; }

void ControlStrategy::slot_TaskCreated(int tid) {}

void ControlStrategy::slot_TaskTerminated(int tid) {}

void ControlStrategy::slot_PhaseChanged(int newphase) {}

void ControlStrategy::slot_UserMessage(int arg, double val) {}

bool ControlStrategy::addPinningToHistory(PinningHistory::autopin_pinning pinning, double value) {
	if (history == nullptr) return false;

	int phase = proc->getExecutionPhase();
	history->addPinning(phase, pinning, value);

	return true;
}

PinningHistory::pinning_list ControlStrategy::readPinnings(QString opt) {
	PinningHistory::pinning_list result;
	QStringList pinnings;

	if (config->configOptionExists(opt) <= 0)
		REPORT(Error::BAD_CONFIG, "option_missing", "No pinning specified", result);

	pinnings = config->getConfigOptionList(opt);

	for (int i = 0; i < pinnings.size(); i++) {
		QStringList pinning = pinnings[i].split(':', QString::SkipEmptyParts, Qt::CaseSensitive);
		PinningHistory::autopin_pinning new_pinning;

		for (int j = 0; j < pinning.size(); j++) {
			bool ok;
			int cpu = pinning[j].toInt(&ok);
			if (ok && cpu >= 0)
				new_pinning.push_back(cpu);
			else
				REPORT(Error::BAD_CONFIG, "option_format", pinnings[i] + " is not a valid pinning", result);
		}

		result.push_back(new_pinning);
	}

	return result;
}

void ControlStrategy::refreshTasks() {
	ProcessTree ptree;

	struct tasksort sort;

	CHECK_ERRORV(ptree = proc->getProcessTree());
	ProcessTree::autopin_tid_list task_set = ptree.getAllTasks();

	tasks.clear();

	for (const auto &elem : task_set) {
		tasks.push_back(elem);
		CHECK_ERRORV(sort.sort_tasks[elem] = service->getTaskSortId(elem));
	}

	std::sort(tasks.begin(), tasks.end(), sort);
}
