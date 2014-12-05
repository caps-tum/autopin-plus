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

#include <AutopinPlus/AutopinContext.h> // for AutopinContext
#include <AutopinPlus/Configuration.h>  // for Configuration, etc
#include <AutopinPlus/ProcessTree.h>	// for ProcessTree, etc
#include <deque>						// for deque
#include <map>							// for map
#include <qstring.h>					// for QString
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Base class for performance monitors
 *
 * A performance monitor measures the performance of running tasks.
 */
class PerformanceMonitor {
  public:
	/*!
	 * \brief Data structure for storing unique pointers to performance monitors
	 */
	typedef std::list<std::unique_ptr<PerformanceMonitor>> monitor_list;

	/*!
	 * \brief Data structure for assigning performance values to tasks
	 */
	typedef std::map<int, double> autopin_measurements;

  public:
	/*!
	 * \brief Different types of performance monitors
	 */
	typedef enum { MAX, MIN, UNKNOWN } montype;

	/*!
	 * \brief Constructor
	 *
	 * \param[in] name		Name of the monitor
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] context	Reference to the context of the object calling the constructor
	 */
	PerformanceMonitor(QString name, const Configuration &config, AutopinContext &context);

	virtual ~PerformanceMonitor();

	/*!
	 * \brief Initializes the performance monitor
	 */
	virtual void init() = 0;

	/*!
	 * \brief Returns the type the value measured by the monitor
	 *
	 * This method can be used to check whether greater values (e. g. instructions retired)
	 * or smaller values (e. g. energy consumption) are better.
	 *
	 * \return Value type of the performance monitor
	 */
	montype getValType();

	/*!
	 * \brief Returns the type of the performance monitor
	 *
	 * \return The type of the performance monitor as a string
	 */
	QString getType();

	/*!
	 * \brief Returns the name of the performance monitor
	 *
	 * \return Name of the performance monitor
	 */
	QString getName();

	/*!
	 * \brief Get the unit of the performance monitor.
	 *
	 * \return The unit of the performance monitor or the empty string if the monitor doesn't have a unit.
	 */
	virtual QString getUnit();

	/*!
	 * \brief Returns the configuration options of the monitor
	 *
	 * \return A list with the configuration options
	 */
	virtual Configuration::configopts getConfigOpts() = 0;

	/*!
	 * \brief Starts performance measuring for a task
	 *
	 * If the performance of the task is already being measured all
	 * previous values will be cleared.
	 *
	 * \param[in] tid	The tid of the task
	 */
	virtual void start(int tid) = 0;

	/*!
	 * \brief Reads the current performance value of a task
	 *
	 * \param[in] tid The tid of the task
	 *
	 * \return Performance of the task or 0 if there is any error.
	 */
	virtual double value(int tid) = 0;

	/*!
	 * \brief Stops performance measuring for a task
	 *
	 * \param[in] tid	The tid of the task
	 *
	 * \return Performance of the task or 0 if there is any error.
	 */
	virtual double stop(int tid) = 0;

	/*!
	 * \brief Stops performance measuring for a task without producing errors
	 *
	 * \param[in] tid 	The tid of the task
	 *
	 */
	virtual void clear(int tid) = 0;

	/*!
	 * \brief Starts performance measuring for a set of tasks
	 *
	 * If the performance of one of the tasks is already being measured all
	 * previous performance values of this task will be cleared.
	 *
	 * \param[in] tasks	List with tids of the tasks
	 */
	virtual void start(ProcessTree::autopin_tid_list tasks);

	/*!
	 * \brief Stops performance measuring for set of tasks
	 *
	 * \param[in] tasks	List with tids of the tasks
	 *
	 * \return Performance of the tasks
	 */
	virtual autopin_measurements stop(ProcessTree::autopin_tid_list tasks);

	/*!
	 * \brief Stops performance a set of tasks without producing errors
	 *
	 * \param[in] tasks	List with tids of the tasks
	 *
	 */
	virtual void clear(ProcessTree::autopin_tid_list tasks);

	/*!
	 * \brief Reads the current performance value of a set of tasks
	 *
	 * \param[in] tasks	List with tids of the tasks
	 *
	 * \return Performance of the tasks
	 */
	virtual autopin_measurements value(ProcessTree::autopin_tid_list tasks);

	/*!
	 * \brief Stops all running measurements
	 *
	 * \return Performance of the tasks
	 */
	virtual autopin_measurements stop();

	/*!
	 * \brief Stops performance for all tasks without producing errors
	 *
	 */
	virtual void clear();

	/*!
	 * \brief Get the tasks which are currently monitored by the performance monitor
	 *
	 * \return A list with the tids of the currently monitored tasks. If no task is
	 * 	monitored the list is empty.
	 */
	virtual ProcessTree::autopin_tid_list getMonitoredTasks() = 0;

	/*!
	 * \brief Parses a string into a montype.
	 *
	 * This function parses the supplied string into a montype. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string to be parsed.
	 *
	 * \exception Exception This exception will be thrown if the string could not be parsed.
	 *
	 * \return The parsed montype.
	 */
	static montype readMontype(const QString &string);

	/*!
	 * \brief Converts a montype into a string.
	 *
	 * This function converts the supplied montype into a string. If this fails, an exception is thrown.
	 *
	 * \param[in] type The montype to be converted.
	 *
	 * \exception Exception This exception will be thrown if the supplied montype was invalid.
	 *
	 * \return A string representing the supplied montype.
	 */
	static QString showMontype(const montype &type);

  protected:
	/*!
	 * Reference of the current Configuration object
	 */
	const Configuration &config;

	/*!
	 * The runtime context
	 */
	AutopinContext &context;

	/*!
	 * Value type of the performance monitor
	 */
	montype valtype;

	/*!
	 * Type of the monitor
	 */
	QString type;

	/*!
	 * Name of the performance monitor
	 */
	QString name;
};

} // namespace AutopinPlus
