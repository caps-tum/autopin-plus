/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
