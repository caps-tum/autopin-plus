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

#include <AutopinPlus/Autopin.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/OS/OSServices.h>
#include <AutopinPlus/OS/SignalDispatcher.h>
#include <AutopinPlus/MQTTClient.h>
#include <AutopinPlus/Monitor/ClustSafe/Main.h>
#include <QFileInfo>
#include <QString>
#include <memory>
#include <iostream>
#include <iomanip>

// For getopt, GNU getopt only
#include <getopt.h>

/* Macro for exiting the application.
 * The return is needed as QApplication::exit() does return
 * from its invokation.
 */
#define EXIT(x) \
	exit(x);    \
	return;

using AutopinPlus::OS::SignalDispatcher;
using AutopinPlus::OS::OSServices;

namespace AutopinPlus {

Autopin::Autopin(int &_argc, char *const *_argv)
	: QCoreApplication(_argc, const_cast<char **>(_argv)), context(std::string("global")), argc(_argc), argv(_argv) {}

void Autopin::slot_autopinSetup() {

	struct option long_options[] = {
		{"conf", 1, NULL, 'c'}, {"daemon", 0, NULL, 'd'}, {"version", 0, NULL, 'v'}, {"help", 0, NULL, 'h'}};

	// Parsing commandline arguments
	std::list<QString> configPaths;
	QString globalConfigPath = "";

	int opt;
	while ((opt = getopt_long(argc, argv, "vhdc:", long_options, NULL)) != -1) {
		switch (opt) {
		case ('v'):
			printVersion();
			EXIT(0);
		case ('h'):
			printHelp();
			EXIT(0);
		case ('c'):
			configPaths.push_back(optarg);
			break;
		case ('d'):
			isDaemon = true;
			break;
		case ('?'):
			std::cout << "\n";
			printHelp();
			EXIT(2);
		}
	}

	if (argv[optind] != nullptr) {
		globalConfigPath = QString(argv[optind]);
	}

	// Start message
	QString startup_msg, qt_msg, host;

	host = OSServices::getHostname_static();

	startup_msg = applicationName() + " " + applicationVersion() + " started with pid " +
				  QString::number(applicationPid()) + " on " + host + "!";
	context.info(startup_msg);

	qt_msg = QString("Running with Qt") + qVersion();
	context.info(qt_msg);

	// Load global config file
	Configuration *globalConfig = nullptr;

	if (globalConfigPath != "") {
		context.info("Reading global configuration");

		QFile globalConfigFile(globalConfigPath);
		if (globalConfigFile.open(QIODevice::ReadOnly)) {
			QTextStream stream(&globalConfigFile);
			globalConfig = new StandardConfiguration(stream.readAll(), context);
			globalConfig->init();
		} else {
			context.report(Error::FILE_NOT_FOUND, "config_file",
						   "Could not read global configuration \"" + globalConfigPath + "\"");
		}
	}

	if (globalConfig != nullptr) {
		if (isDaemon) {
			// Setting up MQTT Communcation
			context.info("Setting up MQTT communication");
			std::string error_message = MQTTClient::init(*globalConfig);

			if (error_message != "") {
				context.error(QString::fromStdString(error_message));
				EXIT(1);
			} else {
				connect(&MQTTClient::getInstance(), SIGNAL(sig_receivedProcessConfig(QString)), this,
						SLOT(slot_runProcess(QString)));
			}
		}

		AutopinPlus::Monitor::ClustSafe::Main::init_static(*globalConfig, context);

		delete globalConfig;
	}

	// Setting up signal Handlers
	context.info("Setting up signal handlers");
	if (SignalDispatcher::setupSignalHandler() != 0) {
		context.report(Error::SYSTEM, "sigset", "Cannot setup signal handling");
		EXIT(1);
	}

	// Read configuration
	context.info("Reading configurations ...");

	for (const auto configPath : configPaths) {
		QFile configFile(configPath);
		if (configFile.open(QIODevice::ReadOnly)) {
			QTextStream stream(&configFile);
			std::unique_ptr<Configuration> config(new StandardConfiguration(stream.readAll(), context));
			config->init();

			Watchdog *watchdog = new Watchdog(std::move(config));
			connect(this, SIGNAL(sig_autopinReady()), watchdog, SLOT(slot_watchdogRun()));
			connect(watchdog, SIGNAL(sig_watchdogStop()), this, SLOT(slot_watchdogStop()));

			watchdogs.push_back(watchdog);
		} else {
			context.report(Error::FILE_NOT_FOUND, "config_file", "Could not read configuration \"" + configPath + "\"");
		}
	}

	emit sig_autopinReady();
}

void Autopin::slot_watchdogStop() {
	QObject *ptr = sender();
	Watchdog *watchdog = static_cast<Watchdog *>(ptr);

	watchdogs.remove(watchdog);
	watchdog->deleteLater();

	if (!isDaemon && watchdogs.empty()) EXIT(0);
}

void Autopin::slot_runProcess(const QString configText) {
	std::unique_ptr<Configuration> config(new StandardConfiguration(configText, context));
	config->init();

	Watchdog *watchdog = new Watchdog(std::move(config));
	connect(watchdog, SIGNAL(sig_watchdogStop()), this, SLOT(slot_watchdogStop()));

	watchdogs.push_back(watchdog);

	watchdog->slot_watchdogRun();
}

void Autopin::printHelp() {
	printVersion();
	std::cout << "\nUsage: " << applicationName().toStdString() << " [OPTION] [GLOBAL_CONFIG_FILE]\n";
	std::cout << "Options:\n";
	std::cout << std::left << std::setw(30) << "  -c, --config=CONFIG_FILE"
			  << "Read process-configuration options from file.\n";
	std::cout << std::left << std::setw(30) << ""
			  << "There can be multiple occurrences of this option!\n";
	std::cout << std::left << std::setw(30) << "  -d, --daemon"
			  << "Run autopin as a daemon.\n";
	std::cout << std::left << std::setw(30) << "  -v, --version"
			  << "Prints version and exit\n";
	std::cout << std::left << std::setw(30) << "  -h, --help"
			  << "Prints this help and exit\n";
}

void Autopin::printVersion() {
	std::cout << applicationName().toStdString() << " " << applicationVersion().toStdString() << std::endl;
	std::cout << "QT Version: " << qVersion() << std::endl;
}
} // namespace AutopinPlus
