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

#pragma once

#include <AutopinPlus/OutputChannel.h>
#include <list>
#include <QCoreApplication>
#include <QMutex>
#include <QString>

namespace AutopinPlus {

#define CHECK_ERROR(x, y)                                             \
	do {                                                              \
		x;                                                            \
		if (context.autopinErrorState() != AUTOPIN_NOERROR) return y; \
	} while (0)
#define CHECK_ERRORV(x)                                                                 \
	do {                                                                                \
		x;                                                                              \
		if (context.autopinErrorState() != AUTOPIN_NOERROR) QCoreApplication::exit(-1); \
	} while (0)
#define CHECK_ERRORVA(x, a)                                   \
	do {                                                      \
		x;                                                    \
		if (context.autopinErrorState() != AUTOPIN_NOERROR) { \
			a;                                                \
			QCoreApplication::exit(-1);                       \
		}                                                     \
	} while (0)
#define REPORT(x, y, z, r)                                        \
	do {                                                          \
		if (context.report(x, y, z) != AUTOPIN_NOERROR) return r; \
	} while (0)
#define REPORTV(x, y, z)                                                            \
	do {                                                                            \
		if (context.report(x, y, z) != AUTOPIN_NOERROR) QCoreApplication::exit(-1); \
	} while (0)
#define REPORTVA(x, y, z, a)                              \
	do {                                                  \
		if (context.report(x, y, z) != AUTOPIN_NOERROR) { \
			a;                                            \
			QCoreApplication::exit(-1);                   \
		}                                                 \
	} while (0)

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
	autopin_estate autopinErrorState();

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
	 * Mutex for safe access to the class
	 */
	QMutex mutex;

	/*!
	 * Global error state of autopin
	 */
	autopin_estate global_estate;
};

} // namespace AutopinPlus
