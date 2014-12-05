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
#include <AutopinPlus/XMLPinningHistory.h>

#include <AutopinPlus/OS/Linux/OSServicesLinux.h>

#include <algorithm>
#include <QChar>
#include <QString>
#include <QStringList>
#include <QFileInfo>

namespace AutopinPlus {

ControlStrategy::ControlStrategy(const Configuration &config, const ObservedProcess &proc, OSServices &service,
								 const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: config(config), proc(proc), service(service), monitors(monitors), context(context), name("ControlStrategy") {}

void ControlStrategy::init() {
	// Setup pinning history
	createPinningHistory();

	// Read environment information for the pinning history
	if (history != nullptr) setPinningHistoryEnv();

	// Initialize the pinning history
	if (history != nullptr) history->init();
}

QString ControlStrategy::getName() { return name; }

void ControlStrategy::slot_TaskCreated(int tid) {}

void ControlStrategy::slot_TaskTerminated(int tid) {}

void ControlStrategy::slot_PhaseChanged(int newphase) {}

void ControlStrategy::slot_UserMessage(int arg, double val) {}

bool ControlStrategy::addPinningToHistory(PinningHistory::autopin_pinning pinning, double value) {
	if (history == nullptr) return false;

	int phase = proc.getExecutionPhase();
	history->addPinning(phase, pinning, value);

	return true;
}

PinningHistory::pinning_list ControlStrategy::readPinnings(QString opt) {
	PinningHistory::pinning_list result;
	QStringList pinnings;

	if (config.configOptionExists(opt) <= 0) {
		context.report(Error::BAD_CONFIG, "option_missing", "No pinning specified");
		return result;
	}

	pinnings = config.getConfigOptionList(opt);

	for (int i = 0; i < pinnings.size(); i++) {
		QStringList pinning = pinnings[i].split(':', QString::SkipEmptyParts, Qt::CaseSensitive);
		PinningHistory::autopin_pinning new_pinning;

		for (int j = 0; j < pinning.size(); j++) {
			bool ok;
			int cpu = pinning[j].toInt(&ok);
			if (ok && cpu >= 0)
				new_pinning.push_back(cpu);
			else {
				context.report(Error::BAD_CONFIG, "option_format", pinnings[i] + " is not a valid pinning");
				return result;
			}
		}

		result.push_back(new_pinning);
	}

	return result;
}

void ControlStrategy::refreshTasks() {
	ProcessTree ptree;

	struct tasksort sort;

	ptree = proc.getProcessTree();
	ProcessTree::autopin_tid_list task_set = ptree.getAllTasks();

	tasks.clear();

	for (const auto &elem : task_set) {
		tasks.push_back(elem);
		sort.sort_tasks[elem] = service.getTaskSortId(elem);
	}

	std::sort(tasks.begin(), tasks.end(), sort);
}

void ControlStrategy::createPinningHistory() {
	int optcount_read = config.configOptionExists("PinningHistory.load");
	int optcount_write = config.configOptionExists("PinningHistory.save");

	if (optcount_read <= 0 && optcount_write <= 0) return;
	if (optcount_read > 1)
		context.report(Error::BAD_CONFIG, "inconsistent",
					   "Specified " + QString::number(optcount_read) + " pinning histories as input");
	if (optcount_write > 1)
		context.report(Error::BAD_CONFIG, "inconsistent",
					   "Specified " + QString::number(optcount_write) + " pinning histories as output");

	QString history_config;
	if (optcount_read > 0)
		history_config = config.getConfigOption("PinningHistory.load");
	else if (optcount_write > 0)
		history_config = config.getConfigOption("PinningHistory.save");

	QFileInfo history_info(history_config);

	if (history_info.suffix() == "xml") {
		history = std::unique_ptr<PinningHistory>(new XMLPinningHistory(config, context));
		return;
	}

	context.report(Error::UNSUPPORTED, "critical",
				   "File type \"." + history_info.suffix() + "\" is not supported by any pinning history");
}

void ControlStrategy::setPinningHistoryEnv() {
	history->setHostname(OS::Linux::OSServicesLinux::getHostname_static());
	history->setConfiguration(config.getName(), config.getConfigOpts());
	QString comm = (proc.getCommChanAddr() == "") ? "Inactive" : "Active";
	QString trace = (proc.getTrace()) ? "Active" : "Inactive";
	history->setObservedProcess(proc.getCmd(), trace, comm, QString::number(proc.getCommTimeout()));
	for (auto &elem : monitors) {
		PinningHistory::monitor_config mconf;
		mconf.name = (elem)->getName();
		mconf.type = (elem)->getType();
		mconf.opts = (elem)->getConfigOpts();

		history->addPerformanceMonitor(mconf);
	}
	history->setControlStrategy(this->getName(), this->getConfigOpts());
}

} // namespace AutopinPlus
