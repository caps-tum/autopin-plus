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

#include <AutopinPlus/Monitor/ClustSafe/Main.h>

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/Error.h>				// for Error, Error::::BAD_CONFIG, etc
#include <AutopinPlus/Exception.h>			// for Exception
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor
#include <AutopinPlus/ProcessTree.h>
#include <AutopinPlus/Tools.h> // for Tools
#include <qbytearray.h>		   // for QByteArray
#include <qelapsedtimer.h>	 // for QElapsedTimer
#include <qstring.h>		   // for operator+, QString
#include <qstringlist.h>	   // for QStringList
#include <QtEndian>			   // for qFromBigEndian
#include <QUdpSocket>		   // for QUdpSocket, etc.
#include <stdint.h>			   // for uint16_t, uint8_t

namespace AutopinPlus {
namespace Monitor {
namespace ClustSafe {

// Inialization of static variables
const QString Main::signature = "MEGware";

const uint64_t Main::timeout = 1000;

const uint64_t Main::ttl = 10;

QString Main::host = "localhost";

uint16_t Main::port = 2010;

QString Main::password = "";

QList<int> Main::outlets;

bool Main::timerStarted = false;

QElapsedTimer Main::timer;

QMutex Main::mutex;

std::map<ClustSafe::Main*, uint64_t> Main::instanceValueMap;

/*!
 * \brief Convert a uint16_t to a QByteArray.
 *
 * This function convert a unsigned 16-bit integer to a two-element byte array.
 *
 * \param[in] value The value to be converted.
 *
 * \return The resulting array which contains the specified value in network byte order.
 */
static inline QByteArray toArray(uint16_t value) {
	QByteArray result;

	result.append((uint8_t)((value >> 8)));
	result.append((uint8_t)((value >> 0)));

	return result;
}

/*!
 * \brief Calculates a (very simple) checksum over an array.
 *
 * \param[in] array The array to be checksummed.
 *
 * \return The calculated checksum.
 */
static inline uint8_t calculateChecksum(QByteArray const &array) {
	uint8_t result = 0;

	// Simply add up all bytes in the input array.
	for (auto value : array) {
		result += value;
	}

	return result;
}

Main::Main(QString name, const Configuration &config, AutopinContext &context)
	: PerformanceMonitor(name, config, context) {
	// Set the "type" field of the base class to the name of our monitor.
	type = "clustsafe";

	// Set the "valtype" field of the base class to minimal, as almost always smaller values are "better".
	valtype = PerformanceMonitor::montype::MIN;
}

void Main::init() { context.info("Initializing " + name + " (" + type + ")"); }

void Main::init_static(const Configuration &config, const AutopinContext &context) {
	// Read and parse the "host" option
	if (config.configOptionExists("clust.host") > 0) {
		Main::host = config.getConfigOption("clust.host");
	}
	// Read and parse the "port" option
	if (config.configOptionExists("clust.port") > 0) {
		try {
			port = Tools::readULong(config.getConfigOption("clust.port"));
		} catch (const Exception &e) {
			context.error("ClustSafe::Main::init_static() failed: Could not parse the 'port' option (" +
						  QString(e.what()) + ").");
		}
	}
	// Read and parse the "password" option
	if (config.configOptionExists("clust.password") > 0) {
		Main::password = config.getConfigOption("clust.password");
	}
	// Read and parse the "outlets" option
	if (config.configOptionExists("clust.outlets") > 0) {
		try {
			Main::outlets = Tools::readInts(config.getConfigOptionList("clust.outlets"));
		} catch (const Exception &e) {
			context.error("ClustSafe::Main::init_static() failed: Could not parse the 'outlets' option (" +
						  QString(e.what()) + ").");
		}
	} else {
		context.error("ClustSafe::Main::init_static() failed: Could not find the 'outlets' option.");
	}
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	return result;
}

void Main::start(int thread) {
	try {
		start_static(this);
	} catch (const Exception &e) {
		context.report(Error::MONITOR, "start", name + ".start(" + QString::number(thread) +
													") failed: Could not reset the ClustSafe device (" +
													QString(e.what()) + ")");
		return;
	}

	// Insert the thread into our thread set.
	threads.insert(thread);
}
void Main::start_static(ClustSafe::Main* instance) {
	QMutexLocker ml(&mutex);

	readValueFromDevice(true);
	instanceValueMap[instance] = 0;

	if(!timerStarted) {
		timer.start();
		timerStarted = true;
	}
}

double Main::value(int thread) {
	// Check if we are actually monitoring that thread. If not, error out.
	if (!threads.contains(thread)) {
		context.report(Error::MONITOR, "value",
					   name + ".value(" + QString::number(thread) + ") failed: Thread is not being monitored.");
		return 0;
	}

	try {
		return value_static(this);
	} catch (const Exception &e) {
		context.report(Error::MONITOR, "value", name + QString(e.what()));
		return 0;
	}
}

double Main::value_static(ClustSafe::Main* instance) {
	QMutexLocker ml(&mutex);

	readValueFromDevice();
	return instanceValueMap[instance];
}

double Main::stop(int thread) {
	// Before stopping the counter, get its value one last time...
	double result = 0;
	try {
		result = stop_static(this);
	} catch (const Exception &e) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: value() failed: " + QString(e.what()));
		return 0;
	}

	// ... and then clear it.
	clear(thread);
	if (context.isError()) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: clear() failed.");
		return 0;
	}

	return result;
}

double Main::stop_static(ClustSafe::Main* instance) {
	QMutexLocker ml(&mutex);

	readValueFromDevice();
	double result = instanceValueMap[instance];
	instanceValueMap.erase(instance);

	return result;
	
}

void Main::clear(int thread) { threads.remove(thread); }

ProcessTree::autopin_tid_list Main::getMonitoredTasks() {
	ProcessTree::autopin_tid_list result;

	// Iterate over our thread set o get a list of all threads which are currently being monitored.
	for (auto thread : threads) {
		result.insert(thread);
	}

	return result;
}

QString Main::getUnit() {
	// The ClustSafe device returns the energy in Joules
	return "Joules";
}

void Main::checkAndDrop(QByteArray &array, const QByteArray &prefix, const QString &field) {
	// Check if array starts with the expected prefix.
	if (array.startsWith(prefix)) {
		array.remove(0, prefix.size());
	} else {
		throw Exception(".checkAndDrop(" + array.toHex() + ", " + prefix.toHex() + ", " + field +
						") failed: Second argument is not a prefix of the first argument.");
	}
}

void Main::readValueFromDevice(bool reset) {
	int timeElapsed = timer.elapsed();
	// Only send a new query if the cached value is too old.
	if (reset || (timeElapsed > 0 && static_cast<uint>(timeElapsed) > ttl)) {
		// Try to get the current energy consumption.
		QByteArray payload;
		uint64_t value= 0;
		try {
			// Set the command to 0x010F which means "get the current energy consumption on all outlets".
			// Set the data to "0x01" which means "reset all counters after the response is sent".
			payload = sendCommand(0x010F, QByteArray(1, 1));
		} catch (const Exception &e) {
			throw Exception("Could not read from the ClustSafe device (" + QString(e.what()) + ")");
		}

		// Re-add the value of all outlets in which we are interested.
		for (auto outlet : outlets) {
			if (payload.size() >= 0 &&
				static_cast<uint>(payload.size()) >= outlet * sizeof(uint32_t) + sizeof(uint32_t)) {
				value += qFromBigEndian<qint32>(((uint32_t *)payload.data())[outlet]);
			} else {
				throw Exception("No data received for outlet #" +
								QString::number(outlet) + ".");
			}
		}

		// Restart the timer.
		timer.restart();

		// Update the values of all instances.
		for (auto iterator = instanceValueMap.begin(); iterator != instanceValueMap.end(); iterator++) {
			iterator->second += value;
		}
	}
}

QByteArray Main::sendCommand(uint16_t command, QByteArray data) {
	// Create the socket.
	QUdpSocket socket;

	// Create the request.
	QByteArray request;
	request.append(signature.toUtf8());
	request.append((char)0);
	request.append(password.toUtf8().leftJustified(16, 0, true));
	request.append(toArray(command));
	request.append(toArray(data.size()));
	request.append(data.left(0xFFFF));
	request.append((char)calculateChecksum(toArray(command) + toArray(data.size()) + data.left(0xFFFF)));

	// Connect to the specified host.
	socket.connectToHost(host, port);

	// Wait until the specified timeout for the connection to become ready.
	if (!socket.waitForConnected(timeout)) {
		throw Exception(".sendCommand(" + QString::number(command) + ", " + data.toHex() +
						") failed: Could not establish connection within " + QString::number(timeout) + " ms");
	}

	// Send the request.
	if (socket.write(request) != request.size()) {
		throw Exception(".sendCommand(" + QString::number(command) + ", " + data.toHex() +
						") failed: Could not send request.");
	}

	// Wait for the specified timeout for a response/
	if (!socket.waitForReadyRead(timeout)) {
		throw Exception(".sendCommand(" + QString::number(command) + ", " + data.toHex() +
						") failed: Did not receive any data within " + QString::number(timeout) + " ms");
	}

	// Read the response.
	QByteArray response = socket.readAll();
	checkAndDrop(response, signature.toUtf8(), "signature");
	checkAndDrop(response, QByteArray(1, 1), "device");
	checkAndDrop(response, QByteArray(1, 1), "status");
	checkAndDrop(response, QByteArray(15, 0), "padding");
	checkAndDrop(response, toArray(command), "command");

	QByteArray payload =
		response.mid(2 /* length field */, response.size() - 2 /* length field */ - 1 /* checksum field */);

	checkAndDrop(response, toArray(payload.size()), "length");
	checkAndDrop(response, payload, "payload");
	checkAndDrop(response, QByteArray(1, calculateChecksum(toArray(command) + toArray(payload.size()) + payload)),
				 "checksum");

	// Return the payload
	return payload;
}

} // namespace ClustSafe
} // namespace Monitor
} // namespace AutopinPlus
