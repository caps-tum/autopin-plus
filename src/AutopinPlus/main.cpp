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

/*!
 * \mainpage
 * This is the code documentation for autopin+. For information on how to compile
 * and use autopin+ refer to the files README and CONFIGURATION.
 */

/*!
 * \brief main-function of autopin+
 *
 * Creates the application and starts the event loop.
 *
 * \param[in] argc 	The number of command line arguments
 * \param[in] argv	String array with command line arguments
 *
 * \return		Return value of the application
 */
int main(int argc, char **argv) {
	AutopinPlus::Autopin app(argc, argv);

	app.setApplicationName("autopin+");
	app.setApplicationVersion("1.0.0");

	// Connect the start-/quit-signals with the slots of the Autopin-object
	QObject::connect(&app, SIGNAL(sig_eventLoopStarted()), &app, SLOT(slot_autopinSetup()));

	QTimer::singleShot(0, &app, SIGNAL(sig_eventLoopStarted()));

	return app.exec();
}
