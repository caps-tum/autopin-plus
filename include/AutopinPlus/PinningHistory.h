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

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/Error.h>
#include <deque>
#include <list>
#include <map>
#include <QObject>
#include <QString>
#include <QStringList>

namespace AutopinPlus {

/*!
 * \brief Manages pinnings and corresponding performance values
 *
 * The class can write results for different pinnings to a file and load previous results
 * from a file. Additional information (e. g. the strategy which was used for pinning)
 * ensures that the values will be interpreted correctly when they are read again later.
 */
class PinningHistory {
  public:
	/*!
	 * \brief Stores configuration options for a PerformanceMonitor
	 */
	typedef struct {
		QString name;
		QString type;
		Configuration::configopts opts;
	} monitor_config;

  public:
	/*!
	 * \brief Data structure that maps tasks to cores
	 */
	typedef std::deque<int> autopin_pinning;

	/*!
	 * \brief Data structure for storing a list of pinnings
	 */
	typedef std::deque<PinningHistory::autopin_pinning> pinning_list;

	/*!
	 * \brief Data type for storing a pinning with its result
	 */
	typedef std::pair<autopin_pinning, double> pinning_result;

	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] context	Reference to the context of the object calling the constructor
	 *
	 */
	PinningHistory(const Configuration &config, AutopinContext &context);

	virtual ~PinningHistory();

	/*!
	 * \brief Loads a pinning history from a file specified in the configuration
	 */
	virtual void init() = 0;

	/*!
	 * \brief Saves the current pinning history to a file specified in the configuration
	 */
	virtual void deinit() = 0;

	/*!
	 * \brief Adds a new pinning to the pinning history
	 *
	 * The pinning will be stored with respect to the provided phase. If this phase
	 * already contains the specified pinning the value will be updated. Thus, there is
	 * always at most one entry for every possible pinning.
	 *
	 * \param[in] phase	The current phase of the observed process
	 * \param[in] pinning	The pinning which will be added to the history
	 * \param[in] value	Performance result for the pinning
	 */
	virtual void addPinning(int phase, autopin_pinning pinning, double value);

	/*!
	 * \brief Reads a pinning from the history
	 *
	 * The best pinning for every phase is updated on every call of addPinning().
	 *
	 * \param[in] phase The desired process phase
	 *
	 * \return The best pinning of the specified phase. If there is no
	 * 	such pinning the result will be empty.
	 */
	virtual pinning_result getBestPinning(int phase) const;

	/*!
	 * \brief Reads all pinnings of a phase
	 *
	 * \param[in] phase The desired process phase
	 *
	 * \return All pinnings of the given process phase which
	 * 	are stored in the history
	 */
	std::list<pinning_result> getPinnings(int phase) const;

	const QString &getStrategy() const;

	const Configuration::configopts &getStrategyOptions() const;

	Configuration::configopts::const_iterator getStrategyOption(QString s) const;

	// Methods for setting the runtime information
	/*!
	 * \brief Sets the hostname which will be used by the pinning history
	 *
	 * \param[in]	hostname	The hostname
	 *
	 */
	void setHostname(QString hostname);

	/*!
	 * \brief Sets the information about the configuration
	 *
	 * \param[in] type   Type of the configuration
	 * \param[in] opts   Options of the configuration
	 */
	void setConfiguration(QString type, Configuration::configopts opts);

	/*!
	 * \brief Sets the information about the observed process
	 *
	 * \param[in] cmd	The command the process has been started with
	 * \param[in] trace	The type of tracing used
	 * \param[in] comm	Setting for the communication channel
	 * \param[in] comm_timeout Timeout for the communication channel
	 */
	void setObservedProcess(QString cmd, QString trace, QString comm, QString comm_timeout);

	/*!
	 * \brief Sets the information about the control strategy
	 *
	 * \param[in] type	Type of the control strategy
	 * \param[in] opts	Options of the control strategy
	 */
	void setControlStrategy(QString type, Configuration::configopts opts);

	/*!
	 * \brief Adds information about a performance monitor
	 *
	 * \param[in] mon	Information about the monitor
	 */
	void addPerformanceMonitor(monitor_config mon);

  protected:
	/*!
	 * \brief Data type for storing pinnings with regard to the process phase and the monitor measurements
	 */
	typedef std::map<int, std::list<pinning_result>> pinning_map;

	/*!
	 * \brief Data type for storing the best pinnings of each phase
	 */
	typedef std::map<int, pinning_result> best_pinning_map;

	/*!
	 * Reference to the current configuration instance
	 */
	const Configuration &config;

	/*!
   * The runtime context
   */
	AutopinContext &context;

	/*!
	 * Internal representation of the pinning history
	 */
	pinning_map pinmap;

	/*!
	 * Best pinning for each phase
	 */
	best_pinning_map bestpinmap;

	/*!
	 * Saved if the pinning history has been modified since loading
	 */
	bool history_modified;

	// Data structures for storing environment information
  protected:
	/*!
	 * Variable storing the hostname
	 */
	QString hostname;

	/*!
	 * Variable storing the command of the observed process
	 */
	QString cmd;

	/*!
	 * Stores if process tracing is enabled
	 */
	QString trace;

	/*!
	 * Stores setting for the communication channel
	 */
	QString comm;

	/*!
	 * Stores the timeout for the communication channel
	 */
	QString comm_timeout;

	/*!
	 * Type of the configuration used
	 */
	QString configuration;

	/*!
	 * Configuration options
	 */
	Configuration::configopts configuration_options;

	/*!
	 * Name of the control strategy
	 */
	QString strategy;

	/*!
	 * Configuration options of the control strategy
	 */
	Configuration::configopts strategy_options;

	/*!
	 * Configuration options of the performance monitors
	 */
	std::list<monitor_config> monitors;
};

} // namespace AutopinPlus
