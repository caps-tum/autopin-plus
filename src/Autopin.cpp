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

#include "Autopin.h"

#include <QTimer>
#include <QString>
#include <QFileInfo>

#include "OSServicesLinux.h"

#include "Strategy_autopin1.h"
#include "Strategy_history.h"

Autopin::Autopin(int &argc, char **argv)
	: QCoreApplication(argc, argv), outchan(nullptr), err(nullptr), config(nullptr), service(nullptr), proc(nullptr),
	  strategy(nullptr), history(nullptr) {}

Autopin::~Autopin() {
	delete strategy;

	for (auto &elem : monitors) delete elem;

	delete proc;
	delete service;
	delete history;
	delete config;
	delete err;
	delete outchan;
}

void Autopin::slot_autopinSetup() {
	// Create output channel
	outchan = new OutputChannel();
	// outchan->enableDebug();

	// Create error handler
	err = new Error();

	// Create autpin context
	context = AutopinContext(outchan, err, 0);

	// Start message
	QString startup_msg, host;

	host = OSServicesLinux::getHostname_static();

	startup_msg = applicationName() + " " + applicationVersion() + " started with pid " +
				  QString::number(applicationPid()) + " on " + host + "!\n" + "Running with Qt " + qVersion();
	context.biginfo("\n" + startup_msg);

	// Read configuration
	context.biginfo("\nReading configuration ...");
	config = new StandardConfiguration(this->argc(), this->argv(), context);
	CHECK_ERRORV(config->init());

	// Check if autopin+ output shall be written to a file
	if (config->configOptionExists("Logfile") == 1) {
		QString logpath = config->getConfigOption("Logfile");
		context.biginfo("\nWriting autopin+ output to " + logpath + "\n");
		if (!outchan->writeToFile(logpath)) REPORTV(Error::SYSTEM, "file_open", "Cannot open file " + logpath);

		// Write header to output file
		context.biginfo(startup_msg);
	}

	context.biginfo("\nInitializing environment ...");

	// Setup and initialize os services
	CHECK_ERRORV(createOSServices());
	CHECK_ERRORV(service->init());

	context.info("  > Initializing performance monitors");
	// Setup and initialize performance monitors
	CHECK_ERRORV(createPerformanceMonitors());
	for (auto &elem : monitors) CHECK_ERRORV((elem)->init());

	// Setup and initialize observed process
	proc = new ObservedProcess(config, service, context);
	CHECK_ERRORV(proc->init());

	// Setup pinning history
	CHECK_ERRORV(createPinningHistory());

	// Setup and initialize pinning strategy
	CHECK_ERRORV(createControlStrategy());
	CHECK_ERRORV(strategy->init());

	// Setup global Qt connections
	createComponentConnections();

	// Read environment information for the pinning history
	if (history != nullptr) setPinningHistoryEnv();

	// Initialize the pinning history
	if (history != nullptr) history->init();

	context.biginfo("\nConnecting to the observed process ...");
	// Starting observed process
	CHECK_ERRORV(proc->start());

	context.biginfo("\nStarting control strategy ...");

	emit sig_autopinReady();
}

void Autopin::slot_autopinCleanup() {
	context.biginfo("\nCleaning up ...");
	if (proc != nullptr) proc->deinit();
	if (history != nullptr) history->deinit();
	context.biginfo("\nExiting ...");
}

void Autopin::createOSServices() { service = new OSServicesLinux(context); }

void Autopin::createPerformanceMonitors() {
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

		if (current_type == "perf") {
			PerformanceMonitor *new_mon = new Monitor_perf(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		if (current_type == "random") {
			PerformanceMonitor *new_mon = new Monitor_random(current_monitor, config, context);
			monitors.push_back(new_mon);
			continue;
		}

		REPORTV(Error::UNSUPPORTED, "critical", "Performance monitor type \"" + current_type + "\" is not supported");
	}
}

void Autopin::createPinningHistory() {
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

void Autopin::createControlStrategy() {
	int optcount = config->configOptionExists("ControlStrategy");

	if (optcount <= 0) REPORTV(Error::BAD_CONFIG, "option_missing", "No control strategy configured");
	if (optcount > 1)
		REPORTV(Error::BAD_CONFIG, "inconsistent", "Specified " + QString::number(optcount) + " control strategies");

	QString strategy_config = config->getConfigOption("ControlStrategy");

	if (strategy_config == "autopin1") {
		strategy = new Strategy_autopin1(config, proc, service, monitors, history, context);
		return;
	}
	if (strategy_config == "history") {
		strategy = new Strategy_history(config, proc, service, monitors, history, context);
		return;
	}

	REPORTV(Error::UNSUPPORTED, "", "Control strategy \"" + strategy_config + "\" is not supported");
}

void Autopin::setPinningHistoryEnv() {
	history->setHostname(OSServicesLinux::getHostname_static());
	history->setConfiguration(config->getName(), config->getConfigOpts());
	QString comm = (proc->getCommChanAddr() == "") ? "Inactive" : "Active";
	QString trace = (proc->getTrace()) ? "Active" : "Inactive";
	history->setObservedProcess(proc->getCmd(), trace, comm, QString::number(proc->getCommTimeout()));
	for (auto &elem : monitors) {
		PinningHistory::monitor_config mconf;
		mconf.name = (elem)->getName();
		mconf.type = (elem)->getType();
		mconf.opts = (elem)->getConfigOpts();

		history->addPerformanceMonitor(mconf);
	}
	history->setControlStrategy(strategy->getName(), strategy->getConfigOpts());
}

void Autopin::createComponentConnections() {
	// Connections between the OSServices and the ObservedProcess
	connect(service, SIGNAL(sig_TaskCreated(int)), proc, SLOT(slot_TaskCreated(int)));
	connect(service, SIGNAL(sig_TaskTerminated(int)), proc, SLOT(slot_TaskTerminated(int)));
	connect(service, SIGNAL(sig_CommChannel(autopin_msg)), proc, SLOT(slot_CommChannel(autopin_msg)));

	// Connections between the ObservedProcess and the ControlStrategy
	connect(proc, SIGNAL(sig_TaskCreated(int)), strategy, SLOT(slot_TaskCreated(int)));
	connect(proc, SIGNAL(sig_TaskTerminated(int)), strategy, SLOT(slot_TaskTerminated(int)));
	connect(proc, SIGNAL(sig_PhaseChanged(int)), strategy, SLOT(slot_PhaseChanged(int)));
	connect(proc, SIGNAL(sig_UserMessage(int, double)), strategy, SLOT(slot_UserMessage(int, double)));

	// Connections between Autopin and the ControlStrategy
	connect(this, SIGNAL(sig_autopinReady()), strategy, SLOT(slot_autopinReady()));
}
