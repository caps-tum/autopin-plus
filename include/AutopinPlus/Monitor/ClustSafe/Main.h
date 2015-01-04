/*
 * This file is part of Autopin+.
 *
 * Copyright (C) 2014 Alexander Kurtz <alexander@kurtz.be>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext, etc
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor
#include <AutopinPlus/ProcessTree.h>		// for ProcessTree, etc
#include <qbytearray.h>						// for QByteArray
#include <qelapsedtimer.h>					// for QElapsedTimer
#include <qglobal.h>						// for qFree
#include <qlist.h>							// for QList
#include <qset.h>							// for QSet
#include <qstring.h>						// for QString

namespace AutopinPlus {
namespace Monitor {
namespace ClustSafe {

/*!
 * A performance monitor which supports querying the ClustSafe device from MEGWARE.
 *
 * This performance monitor can read the current energy levels from the ClustSafe devices made by MEGWARE.
 *
 * See also: http://www.megware.com/en/produkte_leistungen/eigenentwicklungen/clustsafe-71-8.aspx
 */
class Main : public PerformanceMonitor {
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] name    Name of this monitor
	 * \param[in] config  Reference to the configuration
	 * \param[in] context Reference to the context
	 */
	Main(QString name, const Configuration &config, AutopinContext &context);

	// Overridden from the base class
	void init() override;

	// Overridden from the base class
	Configuration::configopts getConfigOpts() override;

	// Overridden from the base class
	void start(int tid) override;

	// Overridden from the base class
	double value(int tid) override;

	// Overridden from the base class
	double stop(int tid) override;

	// Overridden from the base class
	void clear(int tid) override;

	// Overridden from the base class
	ProcessTree::autopin_tid_list getMonitoredTasks() override;

	// Overridden from the base class
	QString getUnit() override;

	/*!
	 * \brief Static initalizer for the ClustSafe-device
	 *
	 * \param[in] config  Reference to the global configuration
	 * \param[in] context Reference to the global AutopinContext
	 */
	static void init_static(const Configuration &config, const AutopinContext &context);

  private:
	/*!
	 * \brief Checks if an array has a specific prefix and drops it.
	 *
	 * This function checks if an array has a specific prefix and drops it. If the prefix is not present, an exception
	 * will be thrown.
	 *
	 * \param[in] array  The array to be checked
	 * \param[in] prefix The prefix which should be present
	 * \param[in] field  A description what the prefix is supposed to be.
	 *
	 * \exception Exception This exception will be thrown if the specified prefix is not present.
	 */
	static void checkAndDrop(QByteArray &array, const QByteArray &prefix, const QString &field);

	/*!
	 * \brief Sends a command to a ClustSafe device and returns the answer.
	 *
	 * This function sends a command to a ClustSafe device and returns the answer. If no answer was received in time
	 * or the answer was malformed, an exception will be thrown.
	 *
	 * \param[in] command The command to send.
	 * \paramp[in] data   An optional array of binary data which usually contains arguments to the command.
	 */
	static QByteArray sendCommand(uint16_t command, QByteArray data = QByteArray());

	/*!
	 * Resets the ClustSafe device on the first run of the monitor. Is
	 * called by start(int tid).
	 */
	static void start_static();

	/*!
	 * Mutex for threadsafe access to start_static().
	 */
	static QMutex mutexStart;

	/*!
	 * Gets the value from the ClustSafe device. Is called by
	 * value(int tid).
	 *
	 * \param[in] tid The id of the task
	 */
	static double value_static(int tid);

	/*!
	 * Mutex for threadsafe access to value_static(int tid).
	 */
	static QMutex mutexValue;

	/*!
	 * \brief The host name or IP address of the ClustSafe device.
	 */
	static QString host;

	/*!
	 * \brief The port on which the ClustSafe device listens.
	 */
	static uint16_t port;

	/*!
	 * \brief The signature string used to address a specific kind of CLustSafe device.
	 */
	static const QString signature;

	/*!
	 * \brief The password used when accessing the ClustSafe device.
	 */
	static QString password;

	/*!
	 * \brief The list of outlets whose values will be added to form the resulting value.
	 */
	static QList<int> outlets;

	/*!
	 * \brief The amount of milliseconds before a connection attempt or data read will time out.
	 */
	static const uint64_t timeout;

	/*!
	 * \brief The amount of milliseconds after which the cached value will be refreshed.
	 */
	static const uint64_t ttl;

	/*!
	 * \brief Stores if this monitor was already started once.
	 */
	static bool started;

	/*!
	 * \brief The cached value of the internal counter of the ClustSafe device.
	 */
	uint64_t cached;

	/*!
	 * \brief A set of threads which are currently monitored.
	 */
	QSet<int> threads;

	/*!
	 * \brief The timer keeping track of the age of the cached value.
	 */
	static QElapsedTimer timer;

}; // class Main

} // namespace ClustSafe
} // namespace Monitor
} // namespace AutopinPlus
