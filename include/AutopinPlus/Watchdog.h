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
#include <list>
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Basic class of autopin+
 *
 * This class manages all resources for running exactly one process in the context of autopin+
 * It is responsible for allocating all the resources for running a process and also actually
 * starts the process, as soon as it receives a signal on the slot slot_watchdogRun.
 *
 */
class Watchdog : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor of the Watchdog-class
	 *
	 * \param[in] config  Config of the process, which is run by this class
	 * \param[in] service OSService, used for pinning the threads
	 * \param[in] context Reference to AutopinContext
	 *
	 */
	Watchdog(std::unique_ptr<const Configuration> config, OSServices &service, const AutopinContext &context);

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
	 * Stores a unique pointer to an instance of a subclass of Configuration.
	 */
	std::unique_ptr<const Configuration> config;

	/*!
	 * Reference to an instance of a subclass of OSServices.
	 */
	OSServices &service;

	/*!
	 * Stores a unique pointer to an instance of ObservedProcess.
	 */
	std::unique_ptr<ObservedProcess> process;

	/*!
	 * Stores a list of shared pointers to performance monitors.
	 */
	PerformanceMonitor::monitor_list monitors;

	/*!
	 * Stores a unique pointer to an instance of a subclass of ControlStrategy.
	 */
	std::unique_ptr<ControlStrategy> strategy;

	/*!
	 * Stores a list of shared pointers to data loggers.
	 */
	std::list<std::shared_ptr<DataLogger>> loggers;
};

} // namespace AutopinPlus
