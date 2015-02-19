/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/DataLogger.h> // for DataLogger

#include <AutopinPlus/AutopinContext.h>			 // for AutopinContext
#include <AutopinPlus/Configuration.h>			 // for Configuration, etc
#include <AutopinPlus/DataLogger.h>				 // for DataLogger
#include <AutopinPlus/Logger/External/Process.h> // for Process
#include <AutopinPlus/PerformanceMonitor.h>		 // for PerformanceMonitor, etc
#include <qelapsedtimer.h>						 // for QElapsedTimer
#include <qmutex.h>								 // for QMutex
#include <qstringlist.h>						 // for QStringList
#include <qtimer.h>								 // for QTimer

namespace AutopinPlus {
namespace Logger {
namespace External {

/*!
 * \brief A data logger which spawns a configurable program and periodically sends it the current performance data for
 *        further processing.
 */
class Main : public DataLogger {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor.
	 * \param[in] config Reference to the instance of the "Configuration" class to use.
	 * \param[in] monitors Reference to the list of performance monitors to use
	 * \param[in] context Reference to the instance of the "AutopinContext" class to use.
	 */
	Main(const Configuration &config, PerformanceMonitor::monitor_list const &monitors, AutopinContext &context);

	// Overridde from the base class.
	void init() override;

	// Overridde from the base class.
	Configuration::configopts getConfigOpts() const override;

  private slots:
	/*!
	 * \brief Slot which will be called when a new data point needs to be logged.
	 */
	void slot_logDataPoint();

	/*!
	 * \brief Slot which will be called when the external logger has written to stderr.
	 */
	void slot_readyReadStandardError();

	/*!
	 * \brief Slot which will be called when the external logger has written to stdout.
	 */
	void slot_readyReadStandardOutput();

  private:
	/*!
	 * \brief The program to which the performance data will be sent and its arguments.
	 */
	QStringList command = QStringList("cat");

	/*!
	 * \brief The amount in milliseconds between two data points.
	 */
	int interval = 100;

	/*!
	 * \brief If true, we will only send performance data for the first monitored thread since we assume that the values
	 *        are identical for all threads.
	 */
	bool systemwide = false;

	/*!
	 * \brief The running instance of the specified command.
	 */
	Process process;

	/*!
	 * \brief A mutex preventing two threads from send performance data at the same time.
	 */
	QMutex mutex;

	/*!
	 * \brief The timer responsible for periodically logging the data points.
	 */
	QTimer timer;

	/*!
	 * \brief The timer responsible for keeping track of the elapsed time since the start of the logger.
	 */
	QElapsedTimer running;
};

} // namespace External
} // namespace DataLogger
} // namespace AutopinPlus
