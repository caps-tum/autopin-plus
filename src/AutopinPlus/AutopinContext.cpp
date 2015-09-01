/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
		logger->error(msg.toStdString());
		// Emit signal to Watchdog
		emit sig_error();
	} else {
		logger->warn(msg.toStdString());
	}

	return result;
}

void AutopinContext::setPid(int pid) { name = name + " (pid: " + std::to_string(pid) + ")"; }

bool AutopinContext::isError() const { return err.autopinErrorState() == AUTOPIN_ERROR; }

void AutopinContext::setupLogging(const AutopinContext::logging_t type, const QString &path) {
	try {
		// std::cout << logtype << std::endl;
		if (type == logging_t::SYSLOG) logger = spdlog::syslog_logger("autopin+");
		if (type == logging_t::LOGFILE) logger = spdlog::daily_logger_mt("autopin+", path.toStdString());
		if (type == logging_t::STDOUT) logger = spdlog::stdout_logger_mt("autopin+");
	} catch (const spdlog::spdlog_ex &ex) {
		std::cerr << "Could not initalizes logger. Exiting!" << std::endl;
		QCoreApplication::exit(-2);
	}
}

} // namespace AutopinPlus
