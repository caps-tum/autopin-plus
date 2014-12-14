/*
 * This file is part of Autopin+.
 *
 * Copyright (C) 2014 Alexander Kurtz <alexander@kurtz.be>

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

#include <AutopinPlus/Strategy/Noop/Main.h>

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/ControlStrategy.h>	// for ControlStrategy
#include <AutopinPlus/Error.h>				// for Error, Error::::BAD_CONFIG, etc
#include <AutopinPlus/Exception.h>			// for Exception
#include <AutopinPlus/ObservedProcess.h>	// for ObservedProcess
#include <AutopinPlus/OSServices.h>			// for OSServices
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
#include <AutopinPlus/PinningHistory.h>		// fo PinningHistory
#include <AutopinPlus/Tools.h>				// for Tools
#include <qatomic_x86_64.h>					// for QBasicAtomicInt::deref
#include <qmutex.h>							// for QMutexLocker
#include <qobjectdefs.h>					// for SIGNAL, SLOT
#include <qset.h>							// for QSet
#include <qstring.h>						// for operator+, QString
#include <qstringlist.h>					// for QStringList
#include <qtimer.h>							// for QTimer

namespace AutopinPlus {
namespace Strategy {
namespace Noop {

Main::Main(const Configuration &config, const ObservedProcess &proc, OSServices &service,
		   const PerformanceMonitor::monitor_list &monitors, AutopinContext &context)
	: ControlStrategy(config, proc, service, monitors, context) {
	name = "noop";
}

void Main::init() {
	ControlStrategy::init();

	context.info("Initializing control strategy " + name);

	// Read and parse the "interval" option
	if (config.configOptionExists(name + ".interval") > 0) {
		try {
			interval = Tools::readInt(config.getConfigOption(name + ".interval"));
			context.info("  - " + name + ".interval = " + QString::number(interval));
		}
		catch (const Exception &e) {
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

void Main::slot_autopinReady() {
	// Now that all initialization is done, ask the ObservedProcess if tracing is supported.
	if (proc.getTrace()) {
		// Process tracing is enabled, so we can rely on the ObservedProcess to notify us if new threads are being
		// created. Therefore, do nothing.
	} else {
		// Process tracing is not enabled, we therefore have to periodically update the list of threads to monitor.
		context.debug(name + ".slot_autopinReady(): Process tracing is disabled, falling back to regular polling for "
							 "determining the threads to monitor.");
		connect(&timer, SIGNAL(timeout()), this, SLOT(slot_timer()));
		timer.setInterval(interval);
		timer.start();
	}
}

void Main::slot_TaskCreated(int tid) {
	// We received a signal telling us that a new task was created, so update the monitors.
	updateMonitors();
}

void Main::slot_TaskTerminated(int tid) {
	// We received a signal telling us that a task was terminated, so update the monitors.
	updateMonitors();
}

void Main::slot_timer() {
	// We receiver a signal telling us that the timer ran out, so update the monitors.
	updateMonitors();
}

void Main::updateMonitors() {
	// Prevent two threads from updating the monitors at the same time.
	QMutexLocker locker(&mutex);

	QSet<int> old_tasks;
	QSet<int> new_tasks;

	// Insert all tasks from the "tasks" variable of the base class into a new set variable.
	for (auto task : tasks) {
		old_tasks.insert(task);
	}

	// Refresh the "tasks" variable of the base class.
	refreshTasks();

	// Now that the "tasks" variable has been refreshed, re-read into another set variable.
	for (auto task : tasks) {
		new_tasks.insert(task);
	}

	// Iterate over all tasks which have been removed...
	for (auto task : old_tasks - new_tasks) {
		// ... and remove them from all monitors.
		for (auto i = monitors.begin(); i != monitors.end(); i++) {
			context.debug(name + ".updateMonitors(): Stopping monitor for dead task " + QString::number(task) + ".");
			(*i)->clear(task);
		}
	}

	// Iterator over all tasks which have been added...
	for (auto task : new_tasks - old_tasks) {
		// ... and add them to all monitors.
		for (auto i = monitors.begin(); i != monitors.end(); i++) {
			context.debug(name + ".updateMonitors(): Starting monitor for new task " + QString::number(task) + ".");
			(*i)->start(task);
		}
	}
}

} // namespace Noop
} // namespace Strategy
} // namespace AutopinPlus
