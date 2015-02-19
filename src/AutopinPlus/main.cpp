/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
