/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/PerformanceMonitor.h>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif
#include "spm.h"
#ifdef __cplusplus
}
#endif

namespace AutopinPlus {
namespace Monitor {
namespace PageMigrate {

/*!
 * \brief PageMigrate counter
 *
 * This class integrates autopin with the page migration logic
 */
class Main : public PerformanceMonitor {
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] name		Name of the monitor
	 * \param[in] config	Pointer to the current Configuration instance
	 * \param[in] context	Reference to the context of the object calling the constructor
	 */
	Main(QString name, const Configuration &config, AutopinContext &context);

	void init() override;
	void start(int tid) override;
	double value(int tid) override;
	double stop(int tid) override;
	void clear(int tid) override;
	ProcessTree::autopin_tid_list getMonitoredTasks() override;
	Configuration::configopts getConfigOpts() override;

  private:
	/*!
	 * Data structure for storing random performance values
	 * for threads
	 */
	typedef std::map<int, double> rand_map;

	/*!
	 * The period used to make the memory loads sampling
	 */
	int period;
	
	/*!
	 * The minimum weight for a memory access to be sampled
	 */
	int min_weight;

	/*!
	 * The time in seconds to look for remote accesses
	 */
	int sensing_time;
	/*!
	 * Performance values of "monitored" tasks
	 */
	rand_map rands;
	
	bool started;
	/*!
	 * The configuration fot the page migration
	 */
	struct sampling_settings st;
};

} 
} // namespace Monitor
} // namespace AutopinPlus

