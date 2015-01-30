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
#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ObservedProcess.h>
#include <AutopinPlus/OS/OSServices.h>
#include <AutopinPlus/PerformanceMonitor.h>
#include <QTimer>
#include <memory>
#include <deque>
#include <map>

namespace AutopinPlus {

/*!
 * \brief Base class for control strategies
 *
 * A control strategy determines how autopin+ assigns tasks to cpu cores.
 */
class ControlStrategy : public QObject {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] proc		Reference to the observed process
	 * \param[in] service	Reference to the current OSServices instance
	 * \param[in] monitors	Reference to a list of available instances of PerformanceMonitor
	 * \param[in] context	Reference to the context of the object calling the constructor
	 */
	ControlStrategy(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
					const PerformanceMonitor::monitor_list &monitors, AutopinContext &context);

	/*!
	 * \brief Initializes the control strategy
	 */
	virtual void init();

	/*!
	 * \brief Get the name of the control strategy
	 *
	 * \return The name of the control strategy
	 */
	QString getName();

	/*!
	 * \brief Returns the configuration options of the control strategy
	 *
	 * \return A list with the configuration options
	 */
	virtual Configuration::configopts getConfigOpts() = 0;

  public slots:
	/*!
	 * \brief Starts the strategy when autopin+ has finished initialization
	 */
	virtual void slot_watchdogReady();

	/*!
	 * \brief Handles the creation of a new tasks
	 *
	 * \param[in] tid The tid of the task
	 */
	virtual void slot_TaskCreated(int tid);

	/*!
	 * \brief Handles the termination of a task
	 *
	 * \param[in] tid The tid of the task
	 */
	virtual void slot_TaskTerminated(int tid);

	/*!
	 * \brief Handles transitions between execution phases
	 *
	 * \param[in] newphase 	The new phase
	 */
	virtual void slot_PhaseChanged(int newphase);

	/*!
	 * \brief Handles user-defined messages from the communication channel
	 *
	 * \param[in] arg The argument value of the messages
	 * \param[in] val The value stored in the messages
	 */
	virtual void slot_UserMessage(int arg, double val);

  protected:
	struct Task {
		int pid;
		int tid;
		bool operator==(const Task &rhs) const;
		bool operator!=(const Task &rhs) const;
		bool isCpuFree() const;
	};

	const static Task emptyTask;

	/*!
	 * \brief Data structure that maps tasks to cores
	 */
	using Pinning = std::vector<Task>;

	/*!
	 * \brief Data structure for storing a list of pinnings
	 */
	using Tasklist = std::vector<int>;

	/*!
	 * Current tasks of the observed process
	 */
	Tasklist tasks;

	/*!
	 * Should be called, when the Strategy wishes to change the
	 * pinning. This funtion calls getPinning and changes the pinning
	 * accordingly.
	 */
	void changePinning();

	/*!
	 * \brief Returns the nth-cpu for the nth-unpinned task
	 *
	 * The implementation of this function has to make sure that the
	 * cpus are free to pin, which means it is occupied by a task from
	 * its own process or it is unused which equals 'pinning[cpu] ==
	 * {0,0}'.
	 *
	 * \param[in] current_pinning The actual pinning for all supervised processes
	 *
	 * \return The new pinning. The tasks will be pinned accordingly
	 * by changePinning.
	 */
	virtual Pinning getPinning(const Pinning &current_pinning) const;

	//@{
	/*!
	 * Variables for storing runtime information in the constructor
	 */
	const Configuration &config;
	const ObservedProcess &proc;
	OS::OSServices &service;
	const PerformanceMonitor::monitor_list &monitors;
	//@}

	/*!
	 * The runtime context
	 */
	AutopinContext &context;

	/*!
	 * \brief Get running tasks
	 *
	 * Refreshes the current tasks of the observed process and stores
	 * them in tasks sorted by tid.
	 */
	void refreshTasks();

	/*!
	 * Name of the control strategy
	 */
	QString name;

	/*!
	 * \brief Stores the (configurable) interval in milliseconds
	 * between the regular queries to the OS for the current list of
	 * threads. If intervall is 0 or negative, polling is disabled.
	 */
	int interval = 100;

  private:
	/*!
	 * \brief Stores the current pinning.
	 */
	static Pinning pinning;

	/*!
	 * \brief Mutex for access to pinning, to avoid a race condition,
	 * between reading the current pinning and writing a new one.
	 */
	static QMutex mutex;

	/*!
	 * \brief If process tracing is disabled, this timer will
	 * regularly query the OS for the current list of threads.
	 */
	QTimer timer;
  private slots:
	/*!
	 * \brief Handles the regular update of the list of monitored
	 * threads if process tracing is disabled.
	 */
	void slot_timer();
};

} // namespace AutopinPlus
