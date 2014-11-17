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
#include <AutopinPlus/Logger/External/Main.h>
#include <AutopinPlus/Monitor/ClustSafe/Main.h>
#include <AutopinPlus/Monitor/GPerf/Main.h>
#include <AutopinPlus/Monitor/Perf/Main.h>
#include <AutopinPlus/Monitor/Random/Main.h>
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>
#include <AutopinPlus/Strategy/Autopin1/Main.h>
#include <AutopinPlus/Strategy/History/Main.h>
#include <AutopinPlus/Strategy/Noop/Main.h>
#include <AutopinPlus/XMLPinningHistory.h>
#include <QFileInfo>
#include <QString>
#include <QList>
#include <memory>

/*
 * Every implementation of OSServices must provide a static method
 * for reading the hostname of the system. The mapping to the right
 * OSServices class is done with a marco (see os_linux).
 */
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>

namespace AutopinPlus {

Autopin::Autopin(int &argc, char **argv) : QCoreApplication(argc, argv), service(nullptr) {}

void Autopin::slot_autopinSetup() {
	// Create output channel
	std::shared_ptr<OutputChannel> outchan(new OutputChannel());
	// outchan->enableDebug();

	// Create error handler
	std::shared_ptr<Error> err(new Error());

	// Create autopin context
	context = AutopinContext(outchan, err, 0);

	// Start message
	QString startup_msg, host;

	host = OS::Linux::OSServicesLinux::getHostname_static();

	startup_msg = applicationName() + " " + applicationVersion() + " started with pid " +
				  QString::number(applicationPid()) + " on " + host + "!\n" + "Running with Qt " + qVersion();
	context.biginfo("\n" + startup_msg);

	// Read configuration
	context.biginfo("\nReading configurations ...");

	std::list<StandardConfiguration *> configs;

	bool isConfig = false;
	for (int i = 1; i < this->argc(); i++) {
		QString arg = this->argv()[i];
		if (isConfig) {
			StandardConfiguration *config = new StandardConfiguration(arg, context);
			CHECK_ERRORV(config->init());
			configs.push_back(config);
			isConfig = false;
			continue;
		}
		if (arg == "-c") {
			isConfig = true;
			continue;
		}
	}

	context.biginfo("\nInitializing environment ...");

	// Setup and initialize os services
	CHECK_ERRORV(createOSServices());
	CHECK_ERRORV(service->init());

	for (const auto config : configs) {
		std::unique_ptr<Watchdog> ptr(new Watchdog(std::unique_ptr<const Configuration>(config), *service, context));
		connect(this, SIGNAL(sig_autopinReady()), ptr.get(), SLOT(slot_watchdogRun()));
		watchdogs.push_back(std::move(ptr));
	}

	emit sig_autopinReady();
}

void Autopin::createOSServices() { service = std::unique_ptr<OSServices>(new OS::Linux::OSServicesLinux(context)); }

} // namespace AutopinPlus
