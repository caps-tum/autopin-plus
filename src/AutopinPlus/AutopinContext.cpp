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

#include <AutopinPlus/AutopinContext.h>
#include <iostream>

namespace AutopinPlus {

std::shared_ptr<spdlog::logger> AutopinContext::logger;

AutopinContext::AutopinContext(std::string name) : err(), name(name) {}

void AutopinContext::info(QString msg) const {
	std::string log = "[" + name + "] " + msg.toStdString();
	logger->info(log.c_str());
}

void AutopinContext::warn(QString msg) const {
	std::string log = "[" + name + "] " + msg.toStdString();
	logger->warn(log.c_str());
}

void AutopinContext::error(QString msg) const {
	std::string log = "[" + name + "] " + msg.toStdString();
	logger->error(log.c_str());
}

void AutopinContext::debug(QString msg) const {
	std::string log = "[" + name + "] " + msg.toStdString();
	logger->debug(log.c_str());
}

autopin_estate AutopinContext::report(Error::autopin_errors error, QString opt, QString msg) {
	autopin_estate result;
	result = err.report(error, opt);
	if (isError()) {
		logger->error(msg.toLocal8Bit());
		// Emit signal to Watchdog
		emit sig_error();
	} else {
		logger->warn(msg.toLocal8Bit());
	}

	return result;
}

void AutopinContext::setPid(int pid) { name = name + " (pid: " + std::to_string(pid) + ")"; }

bool AutopinContext::isError() const { return err.autopinErrorState() == AUTOPIN_ERROR; }

void AutopinContext::setupLogging(const AutopinContext::logging_t type, const QString &path) {
	try {
		// std::cout << logtype << std::endl;
		if (type == SYSLOG) logger = spdlog::syslog_logger("autopin+");
		if (type == LOGFILE) logger = spdlog::daily_logger_mt("autopin+", path.toStdString());
		if (type == STDOUT) logger = spdlog::stdout_logger_mt("autopin+");
	} catch (const spdlog::spdlog_ex &ex) {
		std::cerr << "Could not initalizes logger. Exiting!" << std::endl;
		QCoreApplication::exit(-2);
	}
}

} // namespace AutopinPlus
