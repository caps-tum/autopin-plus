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
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>
#include <QFileInfo>
#include <QString>
#include <memory>

/*
 * Every implementation of OSServices must provide a static method
 * for reading the hostname of the system. The mapping to the right
 * OSServices class is done with a marco (see os_linux).
 */
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>
#include <AutopinPlus/OS/Linux/SignalDispatcher.h>

using AutopinPlus::OS::Linux::SignalDispatcher;
using AutopinPlus::OS::Linux::OSServicesLinux;

namespace AutopinPlus {

Autopin::Autopin(int &argc, char **argv) : QCoreApplication(argc, argv), context(std::string("global")) {}

void Autopin::slot_autopinSetup() {
	// Start message
	QString startup_msg, qt_msg, host;

	host = OSServicesLinux::getHostname_static();

	startup_msg = applicationName() + " " + applicationVersion() + " started with pid " +
				  QString::number(applicationPid()) + " on " + host + "!";
	context.info(startup_msg);

	qt_msg = QString("Running with Qt") + qVersion();
	context.info(qt_msg);

	// Setting up signal Handlers
	context.info("Setting up signal handlers");
	if (SignalDispatcher::setupSignalHandler() != 0) {
		context.report(Error::SYSTEM, "sigset", "Cannot setup signal handling");
		exit(-1);
	}

	// Read configuration
	context.info("Reading configurations ...");

	std::list<StandardConfiguration *> configs;

	bool isConfig = false;
	for (int i = 1; i < this->argc(); i++) {
		QString arg = this->argv()[i];
		if (isConfig) {
			StandardConfiguration *config = new StandardConfiguration(arg, context);
			config->init();
			configs.push_back(config);
			isConfig = false;
			continue;
		}
		if (arg == "-c") {
			isConfig = true;
			continue;
		}
		if (!isConfig && arg == "-d") {
			isDaemon = true;
			continue;
		}
	}

	for (const auto config : configs) {
		Watchdog *watchdog = new Watchdog(std::unique_ptr<const Configuration>(config));
		connect(this, SIGNAL(sig_autopinReady()), watchdog, SLOT(slot_watchdogRun()));
		connect(watchdog, SIGNAL(sig_watchdogStop()), this, SLOT(slot_watchdogStop()));

		watchdogs.push_back(watchdog);
	}

	emit sig_autopinReady();
}

void Autopin::slot_watchdogStop() {
	QObject *ptr = sender();
	Watchdog *watchdog = static_cast<Watchdog *>(ptr);

	watchdogs.remove(watchdog);
	watchdog->deleteLater();

	if (!isDaemon && watchdogs.empty()) QCoreApplication::exit(0);
}
} // namespace AutopinPlus
