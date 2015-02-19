/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <list>
#include <QCoreApplication>
#include <QMutex>
#include <QString>

namespace AutopinPlus {

/*!
 * \brief Return codes when reporting an error (autopin error state)
 */
typedef enum { AUTOPIN_ERROR, AUTOPIN_NOERROR } autopin_estate;

/*!
 * \brief Class for error handling
 *
 * Any error occuring in autopin+ is reported to this class. Based on the rules defined here the application
 * will continue running or autopin+ will be terminated.
 */
class Error {
  public:
	/*!
	 * \brief Error types that can be reported
	 */
	typedef enum {
		FILE_NOT_FOUND,
		BAD_CONFIG,
		PROCESS,
		SYSTEM,
		PROC_TRACE,
		COMM,
		MONITOR,
		STRATEGY,
		HISTORY,
		UNSUPPORTED,
		UNKNOWN,
		NONE
	} autopin_errors;

	/*!
	 * \brief Constructor
	 */
	Error();

	/*!
	 * \brief Method for reporting errors
	 *
	 * All errors occuring while autopin+ is running are reported via this method.
	 *
	 * \param[in] error	Type of the error
	 * \param[in] opt	Additional information about the error
	 */
	autopin_estate report(autopin_errors error, QString opt);

	/*!
	 * \brief Get the global error state
	 *
	 * \return The global error state of autopin
	 */
	autopin_estate autopinErrorState() const;

  private:
	/*!
	 * \brief Data structure for storing an error together with additional information
	 */
	typedef std::pair<autopin_errors, QString> error_pair;

	/*!
	 * \brief Data structure for keeping errors that have occured
	 */
	typedef std::list<error_pair> error_history;

	/*!
	 * \brief Set the error state of autopin+
	 *
	 * Calling this function will set the error state of
	 * autopin+ to AUTOPIN_ERROR and stop the event loop.
	 */
	void setError();

	/*!
	 * \brief Errors that have already occured
	 */
	error_history errors;

	/*!
	 * \brief Mutex for safe access to the class
	 *
	 * The mutex is mutable, because locking it doesn't change the
	 * internal state of the object. It must be locked in some const
	 * methods, such as autopinErrorState().
	 */
	mutable QMutex mutex;

	/*!
	 * Global error state of autopin
	 */
	autopin_estate global_estate;
};

} // namespace AutopinPlus
