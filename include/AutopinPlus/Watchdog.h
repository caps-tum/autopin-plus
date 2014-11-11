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

#pragma once

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/ControlStrategy.h>
#include <AutopinPlus/DataLogger.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ObservedProcess.h>
#include <AutopinPlus/OutputChannel.h>
#include <AutopinPlus/StandardConfiguration.h>
#include <QCoreApplication>
#include <QTimer>
#include <QList>

namespace AutopinPlus {

/*!
 * \brief Basic class of autopin+
 *
 * On starting the application an instance of this class is created in the ::main-function.
 * All components required for running autopin+ are initialized and managed within this
 * class.
 */
class Watchdog : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor of the Watchdog-class
	 *
	 * This class manages all resources for running exactly one process in the context of autopin+
	 * It is responsible for allocating all the resources for running a process and also actually
	 * starts the process, as soon as it receives a signal on the slot slot_watchdogRun.
	 *
	 * \param[in] config  Config of the process, which is run by this class
	 * \param[in] service OSService, used for pinning the threads
	 * \param[in] context Reference to AutopinContext
	 *
	 */
	Watchdog(Configuration *config, OSServices *service, const AutopinContext &context);

	/*!
	 * \brief Destructor
	 */
	virtual ~Watchdog();

  public slots:
	/*!
	 * Runs the watchdog
	 */
	void slot_watchdogRun();

  private:
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
	 * \brief Factory function for data loggers
	 *
	 * Reads the configuration and creates all requested data loggers.
	 */
	void createDataLoggers();

	/*!
	 * \brief Creates all global connections between Qt signals and slots
	 */
	void createComponentConnections();

	/*!
	 * Stores a pointer to an instance of the class AutopinContext.
	 */
	AutopinContext context;

	/*!
	 * Stores a pointer to an instance of a subclass of Configuration.
	 */
	Configuration *config;

	/*!
	 * Stores a pointer to an instance of a subclass of OSServices.
	 */
	OSServices *service;

	/*!
	 * Stores a pinter to an instance of ObservedProcess.
	 */
	ObservedProcess *process;

	/*!
	 * Stores a list of pointers to performance monitors.
	 */
	PerformanceMonitor::monitor_list monitors;

	/*!
	 * Stores a pointer to an instance of a subclass of ControlStrategy.
	 */
	ControlStrategy *strategy;

	/*!
	 * Stores a list of pointers to data loggers.
	 */
	QList<DataLogger *> loggers;

	/*!
	 * Stores a pointer to an instance of a subclass of PinningHistory
	 */
	PinningHistory *history;
};

} // namespace AutopinPlus
