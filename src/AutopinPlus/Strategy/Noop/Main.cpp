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
#include <AutopinPlus/OS/OSServices.h>		// for OSServices
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
#include <AutopinPlus/Tools.h>				// for Tools
#include <qmutex.h>							// for QMutexLocker
#include <qset.h>							// for QSet
#include <qstring.h>						// for operator+, QString
#include <qstringlist.h>					// for QStringList
#include <qtimer.h>							// for QTimer

namespace AutopinPlus {
namespace Strategy {
namespace Noop {

Main::Main(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
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
	ControlStrategy::slot_TaskCreated(tid);
	// Add the task to the monitors
	for (auto i = monitors.begin(); i != monitors.end(); i++) {
		context.debug(name + ".updateMonitors(): Starting monitor for new task " + QString::number(tid) + ".");
		(*i)->start(tid);
	}
}

void Main::slot_TaskTerminated(int tid) {
	ControlStrategy::slot_TaskTerminated(tid);
	// Remove the task from the monitors
	for (auto i = monitors.begin(); i != monitors.end(); i++) {
		context.debug(name + ".updateMonitors(): Stopping monitor for dead task " + QString::number(tid) + ".");
		(*i)->clear(tid);
	}
}
} // namespace Noop
} // namespace Strategy
} // namespace AutopinPlus
