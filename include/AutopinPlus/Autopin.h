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

#pragma once

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/StandardConfiguration.h>
#include <AutopinPlus/Watchdog.h>
#include <QCoreApplication>
#include <list>
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Basic class of autopin+
 *
 * On starting the application an instance of this class is created in the ::main-function.
 * All components required for running autopin+ are initialized and managed within this
 * class.
 */
class Autopin : public QCoreApplication {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor of the Autopin-class
	 *
	 * On creation of a new instance of Autopin a timer is started which will emit sig_eventLoopStarted()
	 * as soon as the Qt event loop is ready. Due to the setup in the ::main-function this signal will
	 * cause the setup procedure slot_autopinSetup() to be executed.
	 *
	 * \param[in] argc	Reference to the number of command line arguments of the application
	 * \param[in] argv	String array with command line arguments
	 *
	 */
	Autopin(int &argc, char *const *argv);

  public slots:
	/*!
	 * \brief Procedure for initializing the environment
	 *
	 * In this slot the user configuration is read and all objects necessary for running autopin+ are
	 * created and initialized.
	 *
	 * The end of the initialization process is signaled via sig_autopinReady().
	 */
	void slot_autopinSetup();

	/*!
	 * \brief Destroyes the watchdog.
	 *
	 * In this slot the watchdog gets deleted and erased from the watchdogs list.
	 */
	void slot_watchdogStop();

	/*!
	 * \brief Setup new Watchdog and run the process
	 */
	void slot_runProcess(const QString configText);

signals:
	/*!
	 * \brief Used for signaling that the Qt event loop is running
	 */
	void sig_eventLoopStarted();

	/*!
	 * \brief Signals that autopin+ is ready
	 *
	 * This signal is emitted at the end of the initialization process in slot_autopinSetup().
	 */
	void sig_autopinReady();

  private:
	/*!
	 * \brief Prints a help message to stdout
	 */
	void printHelp();

	/*!
	 * \brief Prints the version to stdout
	 */
	void printVersion();

	/*!
	 * Stores a pointer to an instance of the class AutopinContext.
	 */
	std::unique_ptr<AutopinContext> context;

	/*!
	 * Stores each Watchdog in a List.
	 */
	std::list<Watchdog *> watchdogs;

	/*!
	 * \brief True, if the application runs in daemon mode, otherwise false.
	 */
	bool isDaemon = false;

	/*!
	 * Command line's argc
	 */
	const int argc;

	/*!
	 * Command line's argv
	 */
	char *const *argv;
};

} // namespace AutopinPlus
