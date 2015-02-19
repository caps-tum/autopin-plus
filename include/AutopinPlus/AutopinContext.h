/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/Error.h>
#include <spdlog.h>
#include <QString>
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Class encapsulating logging and error handling
 *
 * This class is used by various components in autopinplus for logging
 * and error handling. Every seperated component should have exactly
 * one AutopinContext. There is exactly one AutopinContext per
 * Watchdog. There is also one global context used by Autopin.
 *
 * \sa Error, Configuration
 */
class AutopinContext : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] name Name for the AutopinContext, which is logged
	 */
	explicit AutopinContext(std::string name);

	/*!
	 * \brief Prints a message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void info(QString msg) const;

	/*!
	 * \brief Prints a debug message
	 *
	 * The message is only printed if the debug option of the OutputChannel is enabled.
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void debug(QString msg) const;

	/*!
	 * \brief Prints an warning Message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void warn(QString msg) const;

	/*!
	 * \brief Prints a warning message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void error(QString msg) const;

	/*!
	 * \brief Reports an error
	 *
	 * The error is reported via the Error class. Depending on the resulting
	 * error state a warning or an error message will be displayed.
	 *
	 * \param[in] error	Type of the error
	 * \param[in] opt	Additional information about the error
	 * \param[in] msg	Message which will be printed
	 */
	autopin_estate report(Error::autopin_errors error, QString opt, QString msg);

	/*!
	 * \brief Get the global error state
	 *
	 * \return true, if the there was an error, otherwise false
	 */
	bool isError() const;

	/*!
	 * \brief Sets the pid, so it can be logged
	 */
	void setPid(int pid);

	/*!
	 * \brief Logging types
	 */
	enum class logging_t { STDOUT, LOGFILE, SYSLOG };

	/*!
	 * \brief Sets up the logging infrastructur
	 *
	 * \param[in] type	The logging type
	 * \param[in] path	Optional paramter, specifing the file, if type == LOGFILE
	 */
	static void setupLogging(const logging_t type, const QString &path = "log.txt");
signals:
	/*
	 * Gets emmitted, when context encounters a critical error
	 */
	void sig_error();

  private:
	/*!
	 * \brief Stores a shared pointer to the logger, shared by all
	 * instances of AutopinContex
	 */
	static std::shared_ptr<spdlog::logger> logger;

	/*!
	 * \brief Stores an instance of the class Error
	 */
	Error err;

	/*!
	 * \brief Stores the name of the current context
	 */
	std::string name;
};

} // namespace AutopinPlus
