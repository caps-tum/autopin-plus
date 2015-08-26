/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/PerformanceMonitor.h>
#include <map>

namespace AutopinPlus {
namespace Monitor {
namespace PageMigrate {

/*!
 * \brief Random performance counter
 *
 * This class simulates a real performance counter by generating random
 * performance values.
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
	 * \brief Returns a random value between rand_min and rand_max
	 */
	double getRandomValue();

	/*!
	 * Minimum random value
	 */
	double rand_min;

	/*!
	 * Maximum random value
	 */
	double rand_max;

	/*!
	 * Performance values of "monitored" tasks
	 */
	rand_map rands;
	
	bool started;
};

} // namespace Random
} // namespace Monitor
} // namespace AutopinPlus
