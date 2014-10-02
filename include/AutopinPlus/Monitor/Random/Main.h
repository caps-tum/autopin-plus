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

#include <AutopinPlus/PerformanceMonitor.h>
#include <map>

namespace AutopinPlus {
namespace Monitor {
namespace Random {

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
	 * \param[in] config		Pointer to the current Configuration instance
	 * \param[in] context	Refernce to the context of the object calling the constructor
	 */
	Main(QString name, Configuration *config, const AutopinContext &context);

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
};

} // namespace Random
} // namespace Monitor
} // namespace AutopinPlus
