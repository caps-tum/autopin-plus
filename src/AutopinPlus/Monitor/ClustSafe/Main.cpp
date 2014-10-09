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
#include <qatomic_x86_64.h>	// for QBasicAtomicInt::deref, etc
#include <qbytearray.h>		   // for QByteArray
#include <qelapsedtimer.h>	 // for QElapsedTimer
#include <qglobal.h>		   // for qFree
#include <qlist.h>			   // for QList
#include <qset.h>			   // for QSet
#include <qstring.h>		   // for operator+, QString
#include <qstringlist.h>	   // for QStringList
#include <QtEndian>			   // for qFromBigEndian
#include <QUdpSocket>		   // for QUdpSocket, etc.
#include <stdint.h>			   // for uint16_t, uint8_t

namespace AutopinPlus {
namespace Monitor {
namespace ClustSafe {

Main::Main(QString name, Configuration *config, const AutopinContext &context)
	: PerformanceMonitor(name, config, context) {
	// Set the "type" field of the base class to the name of our monitor.
	type = "clustsafe";

	// Set the "valtype" field of the base class to minimal, as almost always smaller values are "better".
	valtype = PerformanceMonitor::montype::MIN;
}

void Main::init() {
	context.enableIndentation();

	context.info("  :: Initializing " + name + " (" + type + ")");

	// Read and parse the "host" option
	if (config->configOptionExists(name + ".host") > 0) {
		host = config->getConfigOption(name + ".host");
		context.info("     - " + name + ".host = " + host);
	} else {
		context.report(Error::BAD_CONFIG, "option_missing", name + ".init() failed: Could not find the 'host' option.");
		return;
	}

	// Read and parse the "port" option
	if (config->configOptionExists(name + ".port") > 0) {
		try {
			port = Tools::readULong(config->getConfigOption(name + ".port"));
			context.info("     - " + name + ".port = " + QString::number(port));
		} catch (Exception e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'port' option (" + QString(e.what()) + ").");
			return;
		}
	}

	// Read and parse the "signature" option
	if (config->configOptionExists(name + ".signature") > 0) {
		signature = config->getConfigOption(name + ".signature");
		context.info("     - " + name + ".signature = " + signature);
	}

	// Read and parse the "password" option
	if (config->configOptionExists(name + ".password") > 0) {
		password = config->getConfigOption(name + ".password");
		context.info("     - " + name + ".password = " + password);
	}

	// Read and parse the "outlets" option
	if (config->configOptionExists(name + ".outlets") > 0) {
		try {
			outlets = Tools::readInts(config->getConfigOptionList(name + ".outlets"));
			context.info("     - " + name + ".outlets = " + Tools::showInts(outlets).join(" "));
		} catch (Exception e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'outlets' option (" + QString(e.what()) + ").");
			return;
		}
	} else {
		context.report(Error::BAD_CONFIG, "option_missing",
					   name + ".init() failed: Could not find the 'outlets' option.");
		return;
	}

	// Read and parse the "timeout" option
	if (config->configOptionExists(name + ".timeout") > 0) {
		try {
			timeout = Tools::readULong(config->getConfigOption(name + ".timeout"));
			context.info("     - " + name + ".timeout = " + QString::number(timeout));
		} catch (Exception e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'timeout' option (" + QString(e.what()) + ").");
			return;
		}
	}

	// Read and parse the "ttl" option
	if (config->configOptionExists(name + ".ttl") > 0) {
		try {
			ttl = Tools::readULong(config->getConfigOption(name + ".ttl"));
			context.info("     - " + name + ".ttl = " + QString::number(ttl));
		} catch (Exception e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'ttl' option (" + QString(e.what()) + ").");
			return;
		}
	}
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(Configuration::configopt("host", QStringList(host)));
	result.push_back(Configuration::configopt("port", QStringList(QString::number(port))));
	result.push_back(Configuration::configopt("signature", QStringList(signature)));
	result.push_back(Configuration::configopt("password", QStringList(password)));
	result.push_back(Configuration::configopt("outlets", Tools::showInts(outlets)));
	result.push_back(Configuration::configopt("timeout", QStringList(QString::number(timeout))));
	result.push_back(Configuration::configopt("ttl", QStringList(QString::number(ttl))));

	return result;
}

void Main::start(int thread) {
	// If this monitor was never started, we need to reset the device and start the timer.
	if (!started) {
		// Set started to true.
		started = true;

		// Reset the device.
		try {
			// Set the command to 0x010F which means "get the current energy consumption on all outlets".
			// Set the data to "0x01" which means "reset all counters after the response is sent".
			sendCommand(0x010F, QByteArray(1, 1));
		} catch (Exception e) {
			context.report(Error::MONITOR, "start", name + ".start(" + QString::number(thread) +
														") failed: Could not reset the ClustSafe device (" +
														QString(e.what()) + ")");
			return;
		}

		// Start the timer.
		timer.start();
	}

	// Insert the thread into our thread set.
	threads.insert(thread);
}

double Main::value(int thread) {
	// Check if we are actually monitoring that thread. If not, error out.
	if (!threads.contains(thread)) {
		context.report(Error::MONITOR, "value",
					   name + ".value(" + QString::number(thread) + ") failed: Thread is not being monitored.");
		return 0;
	}

	// Only send a new query if the cached value is too old.
	if (timer.elapsed() > ttl) {
		// Try to get the current energy consumption.
		QByteArray payload;
		try {
			// Set the command to 0x010F which means "get the current energy consumption on all outlets".
			payload = sendCommand(0x010F);
		} catch (Exception e) {
			context.report(Error::MONITOR, "value", name + ".value(" + QString::number(thread) +
														") failed: Could not read from the ClustSafe device (" +
														QString(e.what()) + ")");
			return 0;
		}

		// Reset the cached value to zero.
		cached = 0;

		// Re-add the value of all outlets in which we are interested to the cached value.
		for (auto outlet : outlets) {
			if (payload.size() >= outlet * sizeof(uint32_t) + sizeof(uint32_t)) {
				cached += qFromBigEndian<qint32>(((uint32_t *)payload.data())[outlet]);
			} else {
				context.report(Error::MONITOR, "value", name + ".value(" + QString::number(thread) +
															") failed: No data received for outlet #" +
															QString::number(outlet) + ".");
				return 0;
			}
		}

		// Restart the timer.
		timer.restart();
	}

	// Return the cached value of the counter minus the thread-specific offset
	return cached;
}

double Main::stop(int thread) {
	double result = value(thread);

	// Before stopping the counter, get its value one last time...
	if (context.autopinErrorState() != autopin_estate::AUTOPIN_NOERROR) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: value() failed.");
		return 0;
	}

	// ... and then clear it.
	clear(thread);

	if (context.autopinErrorState() != autopin_estate::AUTOPIN_NOERROR) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: clear() failed.");
		return 0;
	}

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

uint8_t Main::calculateChecksum(QByteArray array) {
	uint8_t result = 0;

	// Simply add up all bytes in the input array.
	for (auto value : array) {
		result += value;
	}

	return result;
}

void Main::checkAndDrop(QByteArray &array, const QByteArray &prefix, const QString &field) const {
	// Check if array starts with the expected prefix.
	if (array.startsWith(prefix)) {
		array.remove(0, prefix.size());
	} else {
		throw Exception(name + ".checkAndDrop(" + array.toHex() + ", " + prefix.toHex() + ", " + field +
						") failed: Second argument is not a prefix of the first argument.");
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
		throw Exception(name + ".sendCommand(" + QString::number(command) + ", " + data.toHex() +
						") failed: Could not establish connection within " + QString::number(timeout) + " ms");
	}

	// Send the request.
	if (socket.write(request) != request.size()) {
		throw Exception(name + ".sendCommand(" + QString::number(command) + ", " + data.toHex() +
						") failed: Could not send request.");
	}

	// Wait for the specified timeout for a response/
	if (!socket.waitForReadyRead(timeout)) {
		throw Exception(name + ".sendCommand(" + QString::number(command) + ", " + data.toHex() +
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

QByteArray Main::toArray(uint16_t value) {
	QByteArray result;

	result.append((uint8_t)((value >> 8)));
	result.append((uint8_t)((value >> 0)));

	return result;
}

} // namespace ClustSafe
} // namespace Monitor
} // namespace AutopinPlus
