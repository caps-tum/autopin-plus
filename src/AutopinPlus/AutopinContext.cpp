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

AutopinContext::AutopinContext(std::string name) : err(), name(name) {
	try {
		logger = spdlog::stdout_logger_mt(name);
	} catch (const spdlog::spdlog_ex &ex) {
		std::cerr << "Could not initalizes logger. Exiting!";
		QCoreApplication::exit(-2);
	}
}

void AutopinContext::info(QString msg) const { logger->info(msg.toStdString()); }

void AutopinContext::warn(QString msg) const { logger->warn(msg.toStdString()); }

void AutopinContext::error(QString msg) const { logger->error(msg.toStdString()); }

void AutopinContext::debug(QString msg) const { logger->debug(msg.toStdString()); }

autopin_estate AutopinContext::report(Error::autopin_errors error, QString opt, QString msg) {
	autopin_estate result;
	result = err.report(error, opt);
	if (isError()) {
		logger->error(msg.toStdString());
		// Emit signal to Watchdog
		emit sig_error();
	} else {
		logger->warn(msg.toStdString());
	}

	return result;
}

void AutopinContext::setPid(int pid) {
	name = name + " (pid: " + std::to_string(pid) + ")";
	try {
		logger = spdlog::stdout_logger_mt(name);
	} catch (const spdlog::spdlog_ex &ex) {
		std::cerr << "Could not initalizes logger. Exiting!";
		emit sig_error();
	}
}

bool AutopinContext::isError() const { return err.autopinErrorState() == AUTOPIN_ERROR; }
} // namespace AutopinPlus
