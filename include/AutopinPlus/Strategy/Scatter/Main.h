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
namespace Scatter {

/*!
 * \brief A control strategy which tries to put tasks as near as
 * possible to each other.
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
	void slot_TaskTerminated(int tid) override;

  private:
	Pinning getPinning(const Pinning &current_pinning) override;

	/*!
	 * \brief Stores the currently new Task for getPinning.
	 */
	int new_task_tid = 0;

	/*!
	 * \brief Count of task pinned on the n-th numa node
	 */
	std::vector<int> pinCount;

	/*!
	 * \brief Sorts an std::vector and returns an vector of the indexes
	 */
	template <typename T> std::vector<size_t> sort_indexes(const std::vector<T> &v) const {

		// initialize original index locations
		std::vector<size_t> idx(v.size());
		for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

		// sort indexes based on comparing values in v
		std::sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });

		return idx;
	}
};

} // namespace Scatter
} // namespace Strategy
} // namespace AutopinPlus
