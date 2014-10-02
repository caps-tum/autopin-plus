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
#include <AutopinPlus/ControlStrategy.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ObservedProcess.h>
#include <AutopinPlus/OutputChannel.h>
#include <AutopinPlus/StandardConfiguration.h>
#include <QCoreApplication>
#include <QTimer>

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
	 * \param[in]	argc	Reference to the number of command line arguments of the application
	 * \param[in] argv	String array with command line arguments
	 *
	 */
	Autopin(int &argc, char **argv);

	/*!
	 * \brief Destructor
	 */
	virtual ~Autopin();

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
	 * \brief Executes necessary operations before the application exits
	 */
	void slot_autopinCleanup();

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
	 * \brief Factory function for the os services
	 *
	 * Creates the os services.
	 *
	 */
	void createOSServices();

	/*!
	 * \brief Factory function for performance monitors
	 *
	 * Reads the configuration and creates all requested
	 * performance monitors.
	 *
	 */
	void createPerformanceMonitors();

	/*!
	 * \brief Factory function for the pinning history
	 *
	 * Reads the configuration and creates the requested
	 * pinning history. The pinning history which is created
	 * depends on the suffix of the history file:
	 *
	 * - .xml: XMLPinningHistory
	 *
	 */
	void createPinningHistory();

	/*!
	 * \brief Provide environment options for the pinning history
	 */
	void setPinningHistoryEnv();

	/*!
	 * \brief Factory function for the control strategy
	 *
	 * Reads the configuration and creates the requested
	 * control strategy.
	 */
	void createControlStrategy();

	/*!
	 * \brief Creates all global connections between Qt signals and slots
	 */
	void createComponentConnections();

	/*!
	 * Stores a pointer to an instance of the class OutputChannel.
	 */
	OutputChannel *outchan;

	/*!
	 * Stores a pointer to an instance of the class Output.
	 */
	AutopinContext context;

	/*!
	 * Stores a pointer to an instance of the class Error.
	 */
	Error *err;

	/*!
	 * Stores a pointer to an instance of a subclass of Configuration.
	 */
	Configuration *config;

	/*!
	 * Stores a pointer to an instance of a subclass of OSServices.
	 */
	OSServices *service;

	/*!
	 * Stores a pointer to an instance of the class ObservedProcess.
	 */
	ObservedProcess *proc;

	/*!
	 * Stores a list of pointers to performance monitors.
	 */
	PerformanceMonitor::monitor_list monitors;

	/*!
	 * Stores a pointer to an instance of a subclass of ControlStrategy.
	 */
	ControlStrategy *strategy;

	/*!
	 * Stores a pointer to an instance of a subclass of PinningHistory
	 */
	PinningHistory *history;
};

} // namespace AutopinPlus
