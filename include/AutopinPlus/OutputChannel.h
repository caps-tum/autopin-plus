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

#include <QFile>
#include <QMutex>
#include <QString>
#include <QTextStream>

namespace AutopinPlus {

/*!
 * \brief Class for printing output on the terminal
 *
 * All methods of this class are thread safe.
 *
 */
class OutputChannel {
  public:
	/*!
	 * \brief Constructor
	 */
	OutputChannel();

	/*!
	 * \brief Prints a message on the terminal
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void info(QString msg);

	/*!
	 * \brief Prints a message with bold letters on the terminal
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void biginfo(QString msg);

	/*!
	 * \brief Prints a message with yellow letters on the terminal
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void warning(QString msg);

	/*!
	 * \brief Prints a message with red letters on the terminal
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void error(QString msg);

	/*!
	 * \brief Prints a message on the terminal
	 *
	 * Debug-messages are only printed if debug output has been enabled by
	 * calling enableDebug().
	 *
	 * \param[in] msg	The message which will be printed
	 */
	void debug(QString msg);

	/*!
	 * \brief Enables the printing of debug messages
	 */
	void enableDebug();

	/*!
	 * \brief Disables the printing of debug messages
	 */
	void disableDebug();

	/*!
	 * \brief Checks if debug messages are enabled
	 *
	 * \return	True if debug messages are enabled and false in all other
	 * 	cases
	 */
	bool getDebug();

	/*!
	 * \brief Redirects the output to a file
	 *
	 * If the specified file cannot be opened the call of the method will have
	 * no effect. If there already exists a file with the specified name it will
	 * be overwritten. Error messages will be written to both the file and the
	 * console.
	 *
	 * \param[in] path	Path to the output file
	 * \return	True if file could be opened successfully and false in all other
	 * 	cases
	 */
	bool writeToFile(QString path);

	/*!
	 * \brief Redirects the output to the console
	 *
	 * If the output is currently written to a file this file
	 * will be closed.
	 */
	void writeToConsole();

  private:
	/*!
	 * Output stream for console
	 */
	QTextStream outstream;

	/*!
	 * Output stream for files
	 */
	QTextStream outfilestream;

	/*!
	 * Stores if debug messages are enabled or disabled.
	 */
	bool debug_msg;

	/*!
	 * Mutex for synchroninzation.
	 */
	QMutex mutex;

	/*!
	 * File for saving the output if requested
	 */
	QFile outfile;
};

} // namespace AutopinPlus
