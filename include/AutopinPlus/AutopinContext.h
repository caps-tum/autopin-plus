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

#include <AutopinPlus/Error.h>
#include <AutopinPlus/OutputChannel.h>
#include <QString>
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Class encapsulating all runtime information for each class
 *
 * This class is a wrapper for accessing the current runtime instances of
 * the classes OutputChannel and Error. Every autopin class
 * relying on the services provided by these classes gets an own instance of
 * AutopinContext.
 *
 * \sa OutputChannel, Error, Configuration
 */
class AutopinContext {
  public:
	/*!
	 * \brief Constructor
	 */
	AutopinContext();

	/*!
	 * \brief Constructor
	 *
	 * This constructor should only be used to create the first instance of the class. All other
	 * instances should be created from an exising AutopinContext object.
	 *
	 * \param[in] outchan	The OutputChannel providing access to the terminal
	 * \param[in]	err 	Shared pointer to the runtime instance of the Error class
	 * \param[in] layer	The initial amount of leading whitespace
	 */
	AutopinContext(std::shared_ptr<OutputChannel> outchan, std::shared_ptr<Error> err, int layer);

	/*!
	 * \brief Constructor
	 *
	 * Creates a new instance based on an exisiting object of the AutopinContext class.
	 * The leading whitespace for the new object ist extended.
	 *
	 * \param[in] context	Refernce to an instance of AutopinContext
	 */
	explicit AutopinContext(const AutopinContext &context);

	/*!
	 * \brief Prints a message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void info(QString msg);

	/*!
	 * \brief Prints a message with bold letters
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void biginfo(QString msg);

	/*!
	 * \brief Prints a debug message
	 *
	 * The message is only printed if the debug option of the OutputChannel is enabled.
	 *
	 * \param[in] msg	The message which will be printed
	 *
	 * \sa OutputChannel::enableDebug()
	 */
	void debug(QString msg);

	/*!
	 * \brief Enables the indentation of all output
	 *
	 * \sa disableIndentation()
	 */
	void enableIndentation();

	/*!
	 * \brief Disables the indentation of all output
	 *
	 * \sa enableIndentation()
	 */
	void disableIndentation();

	/*!
	 * \brief Reports an error
	 *
	 * The error is reported via the Error class. Depending on the resulting
	 * error state a warining or an error message will be displayed.
	 *
	 * \param[in] error	Type of the error
	 * \param[in] opt	Additional information about the error
	 * \param[in] msg	Message which will be printed
	 */
	autopin_estate report(Error::autopin_errors error, QString opt, QString msg) const;

	/*!
	 * \brief Get the global error state
	 *
	 * \return The global error state of autopin
	 */
	autopin_estate autopinErrorState() const;

  private:
	/*!
	 * \brief Prints an error message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void warning(QString msg);

	/*!
	 * \brief Prints a warning message
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void error(QString msg);

	/*!
	 * Stores a shared pointer to the current runtime instance of OutputChannel
	 */
	std::shared_ptr<OutputChannel> outchan;

	/*!
	 * Stores a shared pointer to the current runtime instance of Error
	 */
	std::shared_ptr<Error> err;

	/*!
	 * Stores the leading whitespace used for indentation
	 */
	QString whitespace;

	/*!
	 * Stores if indentation of output is currently enabled
	 */
	bool indent;
};

} // namespace AutopinPlus
