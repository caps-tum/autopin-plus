/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/ControlStrategy.h>	// for ControlStrategy
#include <AutopinPlus/ObservedProcess.h>	// for ObservedProcess
#include <AutopinPlus/OS/OSServices.h>		// for OSServices
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
#include <qmutex.h>							// for QMutex
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
	Main(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
		 const PerformanceMonitor::monitor_list &monitors, AutopinContext &context);

	// Overridden from base class
	void init() override;

	// Overridden from base class
	Configuration::configopts getConfigOpts() override;

  public slots:
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
};

} // namespace Log
} // namespace Strategy
} // namespace AutopinPlus
