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

#include <AutopinPlus/ControlStrategy.h>
#include <deque>
#include <QElapsedTimer>
#include <QStringList>
#include <QTimer>
#include <set>

namespace AutopinPlus {
namespace Strategy {
namespace Autopin1 {

/*!
 * \brief Realizes the pinning strategy of autopin1
 */
class Main : public ControlStrategy {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] proc		Reference to the observed process
	 * \param[in] service	Reference to the current OSServices instance
	 * \param[in] monitors	Reference to a list of available instances of PerformanceMonitor
	 * \param[in] context	Refernce to the context of the object calling the constructor
	 */
	Main(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
		 const PerformanceMonitor::monitor_list &monitors, AutopinContext &context);

	void init() override;
	Configuration::configopts getConfigOpts() override;

  public slots:
	void slot_autopinReady() override;

	/*!
	 * \brief Assigns tasks to cores
	 */
	void slot_startPinning();

	/*!
	 * \brief Starts the performance monitor for a pinning
	 */
	void slot_startMonitor();

	/*!
	 * \brief Stops the performance monitor for a pinning
	 */
	void slot_stopPinning();

	void slot_TaskCreated(int tid) override;
	void slot_TaskTerminated(int tid) override;
	void slot_PhaseChanged(int newphase) override;

  private:
	/*!
	 * \brief Data structure for storing the parameters of the currently pinned tasks.
	 */
	typedef struct {
		int tid;
		qint64 start;
		qint64 stop;
		double result;
	} pinned_task;

	/*!
	 * \brief List of pinned tasks
	 */
	using autopin_pinned_tasklist = std::deque<pinned_task>;

	/*!
	 * \brief Applies a pinning
	 *
	 * The pinning provided in the argument is applied
	 * by setting the affinity for the corresponding tasks.
	 *
	 * \param [in] pinning The pinning which shall be applied.
	 *
	 */
	void applyPinning(autopin_pinning pinning);

	/*!
	 * \brief Determines if all pinned tasks are still running
	 *
	 * This method determines if all pinned tasks are still running and removes
	 * terminated tasks from the list of pinned tasks. This is necessary if process
	 * tracing is not active.
	 *
	 */
	void checkPinnedTasks();

	/*!
	 * Stores the pinnings
	 */
	pinning_list pinnings;

	/*!
	 * The pinning which is currently tested
	 */
	int current_pinning;

	/*!
	 * Index of the best pinning
	 */
	int best_pinning;

	/*!
	 * Performance value of the best pinning
	 */
	double best_performance;

	/*!
	 * Init time
	 */
	int init_time;

	/*!
	 * Timer for the init time
	 */
	QTimer init_timer;

	/*!
	 * Warmup time between pinnings
	 */
	int warmup_time;

	/*!
	 * Timer for the warmup time
	 */
	QTimer warmup_timer;

	/*!
	 * Measure time
	 */
	int measure_time;

	/*!
	 * Timer for the measure time
	 */
	QTimer measure_timer;

	/*!
	 * Stores if the second thread should not be pinned
	 * because OpenMP is used with icc.
	 */
	bool openmp_icc;

	/*!
	 * Tasks to skip when pinning
	 */
	std::set<int> skip;

	/*!
	 * Interval for phase notifications
	 */
	int notification_interval;

	/*!
	 * Skipped thread ids as strings
	 */
	QStringList skip_str;

	/*!
	 * Store the time when the last pinning started
	 */
	QElapsedTimer measure_start;

	/*!
	 * The performance monitor used by the strategy
	 *
	 * TODO: Replace raw pointer, with something more suitable
	 */
	PerformanceMonitor *monitor;

	/*!
	 * The type of the performance monitor
	 */
	PerformanceMonitor::montype monitor_type;

	/*!
	 * Tasks actually pinned to cores
	 */
	autopin_pinned_tasklist pinned_tasks;

	/*!
	 * Stores if notifications about execution phases and
	 * the creation/termination of tasks are currently enabled.
	 */
	bool notifications;
};

} // namespace Autopin1
} // namespace Strategy
} // namespace AutopinPlus
