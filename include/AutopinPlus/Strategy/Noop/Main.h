/*
 * This file is part of Autopin+.
 *
 * Copyright (C) 2014 Alexander Kurtz <alexander@kurtz.be>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/ControlStrategy.h>	// for ControlStrategy
#include <AutopinPlus/ObservedProcess.h>	// for ObservedProcess
#include <AutopinPlus/OSServices.h>			// for OSServices
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
#include <AutopinPlus/PinningHistory.h>		// fo PinningHistory
#include <qmutex.h>							// for QMutex
#include <qobjectdefs.h>					// for slots, Q_OBJECT
#include <qtimer.h>							// for QTimer

namespace AutopinPlus {
namespace Strategy {
namespace Noop {

/*!
 * \brief A control strategy which does nothing besides starting the configured performance monitors.
 */
class Main : public ControlStrategy {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config    Reference to the instance of the Configuration class to use
	 * \param[in] proc      Reference to the instance of the ObservedProcess class to use
	 * \param[in] service   Reference to the instance of the OSServices class to use
	 * \param[in] monitors  Reference to the list of performance monitors to use
	 * \param[in] context   Reference to the instance of the AutopinContext class to use
	 */
	Main(const Configuration &config, const ObservedProcess &proc, OSServices &service,
		 const PerformanceMonitor::monitor_list &monitors, const AutopinContext &context);

	// Overridden from base class
	void init() override;

	// Overridden from base class
	Configuration::configopts getConfigOpts() override;

  public slots:
	/*!
	 * \brief Slot which will be called if autopin+ has finished initializing.
	 */
	void slot_autopinReady() override;

	/*!
	 * \brief Slot which will be called if a new thread has been created.
	 *
	 * \param[in] tid The tid of the thread.
	 */
	void slot_TaskCreated(int tid) override;

	/*!
	 * \brief Slot which will be called if a thread has terminated.
	 *
	 * \param[in] tid The tid of the thread.
	 */
	void slot_TaskTerminated(int tid) override;

  private:
	/*!
	 * \brief Start all configured performance monitors for all new threads and stop them for all threads which have
	 * terminated.
	 */
	void updateMonitors();

	/*!
	 * \brief Stores the (configurable) interval between the regular queries to the OS for the current list of threads.
	 */
	int interval = 100;

	/*!
	 * \brief Mutex which prevents two threads from updating the monitors at the same time.
	 */
	QMutex mutex;

	/*!
	 * \brief If process tracing is disabled, this timer will regularly query the OS for the current list of threads.
	 */
	QTimer timer;
  private slots:
	/*!
	 * \brief Handles the regular update of the list of monitored threads if process tracing is disabled.
	 */
	void slot_timer();
};

} // namespace Log
} // namespace Strategy
} // namespace AutopinPlus
