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
#include <linux/perf_event.h>
#include <map>

namespace AutopinPlus {
namespace Monitor {
namespace Perf {

/*!
 * \brief Implements a performance monitor based on the Linux Performance Counter subsystem
 *
 * This monitor can only be used if the kernel supports the Linux Performance Counter subsystem.
 *
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
	 * \brief Data type for managing file descriptors for tasks
	 */
	typedef std::map<int, int> descriptor_map;

	/*!
	 * \brief Stores tids together with the corresponding file descriptors
	 */
	descriptor_map perfds;

	/*!
	 * \brief Creates a new performance counter
	 *
	 * The counter will measure the instructions executed by the specified task on all cores.
	 * It won't start measuring performance values before startPerfCounter() has been called.
	 *
	 * \param[in] tid The tid of the task for which the performance counter will be created
	 *
	 * \return The file descriptor for the new performance counter. If the counter can
	 * 	not be created -1 will be returned.
	 */
	int createPerfCounter(int tid);

	/*!
	 * \brief Starts a performance counter
	 *
	 * \param[in] fd File descriptor of the counter
	 *
	 * \return 0 the counter could be started successfully
	 */
	int startPerfCounter(int fd);

	/*!
	 * \brief System call for the perf performance counter
	 *
	 * The code for this method is taken from the perf utilities.
	 *
	 * \return Return code of the perf system call
	 */
	static inline int sys_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd,
										  unsigned long flags);

	/*!
	 * Stores the event to be measured by perf
	 */
	perf_hw_id event_type;
};

} // namespace Perf
} // namespace Monitor
} // namespace AutopinPlus
