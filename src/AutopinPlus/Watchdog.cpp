/*
 * Autopin+ - Automatic thread-to-core-pinning tool
 * Copyright (C) 2014 LRR
 *
 * Author:
 * Lukas Fürmetz
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

#include <AutopinPlus/Watchdog.h>
#include <AutopinPlus/Logger/External/Main.h>
#include <AutopinPlus/Monitor/ClustSafe/Main.h>
#include <AutopinPlus/Monitor/GPerf/Main.h>
#include <AutopinPlus/Monitor/Perf/Main.h>
#include <AutopinPlus/Monitor/Random/Main.h>
#include <AutopinPlus/Strategy/Autopin1/Main.h>
#include <AutopinPlus/Strategy/History/Main.h>
#include <AutopinPlus/Strategy/Noop/Main.h>
#include <AutopinPlus/XMLPinningHistory.h>
#include <QFileInfo>
#include <QString>
#include <QTimer>
#include <QList>

/*
 * Every implementation of OSServices must provide a static method
 * for reading the hostname of the system. The mapping to the right
 * OSServices class is done with a marco (see os_linux).
 */
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>

namespace AutopinPlus {

Watchdog::Watchdog(Configuration *config, OSServices *service, const AutopinContext &context)
	: config(config), service(service), context(context), process(nullptr), strategy(nullptr), history(nullptr) {}

Watchdog::~Watchdog() {
	delete strategy;

	for (auto logger : loggers) delete logger;
	for (auto &elem : monitors) delete elem;

	delete process;
	delete service;
	delete history;
}

void Watchdog::slot_watchdogRun() {

	// Read configuration
	context.biginfo("  > Initializing performance monitors");

	// Setup and initialize performance monitors
	CHECK_ERRORV(createPerformanceMonitors());
	for (auto &elem : monitors) CHECK_ERRORV((elem)->init());

	context.biginfo("  > Initializing performance monitors");

	// Setup and initialize observed process
	process = new ObservedProcess(config, service, context);
	CHECK_ERRORV(process->init());

	// Setup pinning history
	CHECK_ERRORV(createPinningHistory());

	// Setup and initialize pinning strategy
	CHECK_ERRORV(createControlStrategy());
	CHECK_ERRORV(strategy->init());

	// Setup and initialize data loggers
	context.info("  > Initializing data loggers");

	CHECK_ERRORV(createDataLoggers());
	for (auto logger : loggers) {
		CHECK_ERRORV(logger->init());
	}

	// Setup global Qt connections
	createComponentConnections();

	// Read environment information for the pinning history
	if (history != nullptr) setPinningHistoryEnv();

	// Initialize the pinning history
	if (history != nullptr) history->init();

	context.info("\nConnecting to the observed process ...");

	// Starting observed process
	CHECK_ERRORV(process->start());

	context.info("\nStarting control strategy ...");
}

void Watchdog::createPerformanceMonitors() {
	QStringList config_monitors = config->getConfigOptionList("PerformanceMonitors");
	QStringList existing_ids;

	if (config_monitors.empty()) REPORTV(Error::BAD_CONFIG, "option_missing", "No performance monitor configured");

	for (int i = 0; i < config_monitors.size(); i++) {
		QString current_monitor = config_monitors[i], current_type;

		if (existing_ids.contains(current_monitor))
			REPORTV(Error::BAD_CONFIG, "inconsistent",
					"The identifier " + current_monitor + " is already assigned to another monitor");

		int numtypes = config->configOptionExists(current_monitor + ".type");

		if (numtypes <= 0) {
			REPORTV(Error::BAD_CONFIG, "option_missing",
					"Type for monitor \"" + current_monitor + "\" is not specified");
		} else if (numtypes > 1) {
			REPORTV(Error::BAD_CONFIG, "inconsistent",
					"Specified " + QString::number(numtypes) + " types for monitor " + current_monitor);
		}

		current_type = config->getConfigOption(current_monitor + ".type");

		if (current_type == "clustsafe") {
			PerformanceMonitor *new_mon = new Monitor::ClustSafe::Main(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		if (current_type == "gperf") {
			PerformanceMonitor *new_mon = new Monitor::GPerf::Main(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		if (current_type == "perf") {
			PerformanceMonitor *new_mon = new Monitor::Perf::Main(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		if (current_type == "random") {
			PerformanceMonitor *new_mon = new Monitor::Random::Main(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		REPORTV(Error::UNSUPPORTED, "critical", "Performance monitor type \"" + current_type + "\" is not supported");
	}
}

void Watchdog::createPinningHistory() {
	int optcount_read = config->configOptionExists("PinningHistory.load");
	int optcount_write = config->configOptionExists("PinningHistory.save");

	if (optcount_read <= 0 && optcount_write <= 0) return;
	if (optcount_read > 1)
		REPORTV(Error::BAD_CONFIG, "inconsistent",
				"Specified " + QString::number(optcount_read) + " pinning histories as input");
	if (optcount_write > 1)
		REPORTV(Error::BAD_CONFIG, "inconsistent",
				"Specified " + QString::number(optcount_write) + " pinning histories as output");

	QString history_config;
	if (optcount_read > 0)
		history_config = config->getConfigOption("PinningHistory.load");
	else if (optcount_write > 0)
		history_config = config->getConfigOption("PinningHistory.save");

	QFileInfo history_info(history_config);

	if (history_info.suffix() == "xml") {
		history = new XMLPinningHistory(config, context);
		return;
	}

	REPORTV(Error::UNSUPPORTED, "critical",
			"File type \"." + history_info.suffix() + "\" is not supported by any pinning history");
}

void Watchdog::createControlStrategy() {
	int optcount = config->configOptionExists("ControlStrategy");

	if (optcount <= 0) REPORTV(Error::BAD_CONFIG, "option_missing", "No control strategy configured");
	if (optcount > 1)
		REPORTV(Error::BAD_CONFIG, "inconsistent", "Specified " + QString::number(optcount) + " control strategies");

	QString strategy_config = config->getConfigOption("ControlStrategy");

	if (strategy_config == "autopin1") {
		strategy = new Strategy::Autopin1::Main(config, process, service, monitors, history, context);
		return;
	}

	if (strategy_config == "history") {
		strategy = new Strategy::History::Main(config, process, service, monitors, history, context);
		return;
	}

	if (strategy_config == "noop") {
		strategy = new Strategy::Noop::Main(config, process, service, monitors, history, context);
		return;
	}

	REPORTV(Error::UNSUPPORTED, "", "Control strategy \"" + strategy_config + "\" is not supported");
}

void Watchdog::createDataLoggers() {
	for (auto logger : config->getConfigOptionList("DataLoggers")) {
		if (logger == "external") {
			loggers.append(new Logger::External::Main(config, monitors, context));
		} else {
			REPORTV(Error::UNSUPPORTED, "critical", "Data logger \"" + logger + "\" is not supported");
			return;
		}
	}
}

void Watchdog::setPinningHistoryEnv() {
	history->setHostname(OS::Linux::OSServicesLinux::getHostname_static());
	history->setConfiguration(config->getName(), config->getConfigOpts());
	QString comm = (process->getCommChanAddr() == "") ? "Inactive" : "Active";
	QString trace = (process->getTrace()) ? "Active" : "Inactive";
	history->setObservedProcess(process->getCmd(), trace, comm, QString::number(process->getCommTimeout()));
	for (auto &elem : monitors) {
		PinningHistory::monitor_config mconf;
		mconf.name = (elem)->getName();
		mconf.type = (elem)->getType();
		mconf.opts = (elem)->getConfigOpts();

		history->addPerformanceMonitor(mconf);
	}
	history->setControlStrategy(strategy->getName(), strategy->getConfigOpts());
}

void Watchdog::createComponentConnections() {
	// Connections between the OSServices and the ObservedProcess
	connect(service, SIGNAL(sig_TaskCreated(int)), process, SLOT(slot_TaskCreated(int)));
	connect(service, SIGNAL(sig_TaskTerminated(int)), process, SLOT(slot_TaskTerminated(int)));
	connect(service, SIGNAL(sig_CommChannel(autopin_msg)), process, SLOT(slot_CommChannel(autopin_msg)));

	// Connections between the ObservedProcess and the ControlStrategy
	connect(process, SIGNAL(sig_TaskCreated(int)), strategy, SLOT(slot_TaskCreated(int)));
	connect(process, SIGNAL(sig_TaskTerminated(int)), strategy, SLOT(slot_TaskTerminated(int)));
	connect(process, SIGNAL(sig_PhaseChanged(int)), strategy, SLOT(slot_PhaseChanged(int)));
	connect(process, SIGNAL(sig_UserMessage(int, double)), strategy, SLOT(slot_UserMessage(int, double)));
}

} // namespace AutopinPlus
