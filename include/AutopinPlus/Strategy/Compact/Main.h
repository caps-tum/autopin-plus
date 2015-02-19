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

namespace AutopinPlus {
namespace Strategy {
namespace Compact {

/*!
 * \brief A control strategy, which pins the task as closely to each
 * other as possible.
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
	void slot_TaskCreated(int tid) override;

  private:
	Pinning getPinning(const Pinning &current_pinning) override;

	/*!
	 * \brief Stores the currently new Task for getPinning;
	 */
	int new_task_tid = 0;
};

} // namespace Compact
} // namespace Strategy
} // namespace AutopinPlus
