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
#include <qatomic_x86_64.h>					// for QBasicAtomicInt::deref
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
	 * \param[in] config  Pointer to the configuration
	 * \param[in] context Pointer to the context
	 */
	Main(QString name, Configuration *config, const AutopinContext &context);

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

  private:
	/*!
	 * \brief Calculates a (very simple) checksum over an array.
	 *
	 * \param[in] array The array to be checksummed.
	 *
	 * \return The calculated checksum.
	 */
	static uint8_t calculateChecksum(QByteArray array);

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
	void checkAndDrop(QByteArray &array, const QByteArray &prefix, const QString &field);

	/*!
	 * \brief Sends a command to a ClustSafe device and returns the answer.
	 *
	 * This function sends a command to a ClustSafe device and returns the answer. If no answer was received in time
	 * or the answer was malformed, an exception will be thrown.
	 *
	 * \param[in] command The command to send.
	 * \paramp[in] data   An optional array of binary data which usually contains arguments to the command.
	 */
	QByteArray sendCommand(uint16_t command, QByteArray data = QByteArray());

	/*!
	 * \brief Convert a uint16_t to a QByteArray.
	 *
	 * This function convert a unsigned 16-bit integer to a two-element byte array.
	 *
	 * \param[in] value The value to be converted.
	 *
	 * \return The resulting array which contains the specified value in network byte order.
	 */
	QByteArray toArray(uint16_t value);

	/*!
	 * \brief The host name or IP address of the ClustSafe device.
	 */
	QString host;

	/*!
	 * \brief The port on which the ClustSafe device listens.
	 */
	uint16_t port;

	/*!
	 * \brief The signature string used to address a specific kind of CLustSafe device.
	 */
	QString signature;

	/*!
	 * \brief The password used when accessing the ClustSafe device.
	 */
	QString password;

	/*!
	 * \brief The list of outlets whose values will be added to form the resulting value.
	 */
	QList<int> outlets;

	/*!
	 * \brief The amount of milliseconds before a connection attempt or data read will time out.
	 */
	uint64_t timeout;

	/*!
	 * \brief The amount of milliseconds after which the cached value will be refreshed.
	 */
	uint64_t ttl;

	/*!
	 * \brief Stores if this monitor was already started once.
	 */
	bool started;

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
	QElapsedTimer timer;

}; // class Main

} // namespace ClustSafe
} // namespace Monitor
} // namespace AutopinPlus
