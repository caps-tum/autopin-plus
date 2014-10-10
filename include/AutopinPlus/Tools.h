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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <qlist.h>		 // for QList
#include <qstring.h>	 // for QString
#include <qstringlist.h> // for QStringList
#include <utility>		 // for pair

namespace AutopinPlus {
/*!
 * \brief A class containing several static utility functions.
 *
 * This class contains several static utility functions. The class itself should never be instantiated.
 * It's constructor has therefore been made private.
 */
class Tools {
  public:
	/*!
	 * \brief Constructor.
	 *
	 * This constructor is deleted to prevent this class from ever being instantiated.
	 */
	Tools() = delete;

	/*!
	 * \brief Parses a string to a double.
	 *
	 * This function parses the specified string to a double. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string to be parsed.
	 *
	 * \exception Exception This exception will be thrown if the string could not be parsed.
	 *
	 * \return The parsed double.
	 */
	static double readDouble(const QString &string);

	/*!
	 * \brief Parses a string to a int.
	 *
	 * This function parses the specified string to a int. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string to be parsed.
	 *
	 * \exception Exception This exception will be thrown if the string could not be parsed.
	 *
	 * \return The parsed int.
	 */
	static int readInt(const QString &string);

	/*!
	 * \brief Parses a string list to a list of ints.
	 *
	 * This function parses the specified string list to a list of ints. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string list to be parsed.
	 *
	 * \exception Exception This exception will be thrown if the string list could not be parsed.
	 *
	 * \return The parsed list of ints.
	 */
	static QList<int> readInts(const QStringList &list);

	/*!
	 * \brief Reads a line from a file.
	 *
	 * This function reads a line from the specified line. If this fails, an exception is thrown.
	 *
	 * \param[in] path The path to the file from which the line should read.
	 *
	 * \exception Exception This exception will be thrown if writing to the specified file fails.
	 *
	 * \return The first line of the specified file, without the line break.
	 */
	static QString readLine(const QString &string);

	/*!
	 * \brief Splits a string in two parts.
	 *
	 * This function splits the specified string in two parts. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string to be split.
	 * \param[in] separator The string which separates the two parts in the input string.
	 *
	 * \exception Exception This exception will be thrown if there weren't exactly two parts in the input string.
	 *
	 * \return A pair containg the first and second part of the input string.
	 */
	static std::pair<QString, QString> readPair(const QString &string, const QString &separator = "=");

	/*!
	 * \brief Parses a string to a unsigned long.
	 *
	 * This function parses the specified string to a unsigned long. If this fails, an exception is thrown.
	 *
	 * \param[in] string The string to be parsed.
	 *
	 * \exception Exception This exception will be thrown if the string could not be parsed.
	 *
	 * \return The parsed unsigned long.
	 */
	static unsigned long readULong(const QString &string);

	/*!
	 * \brief Converts a list of ints to a string list.
	 *
	 * This function converts the specified list of ints to a string list. If this fails, an exception is thrown.
	 *
	 * \param[in] list The list of ints to be converted.
	 *
	 * \return A string list representing the supplied list of ints.
	 */
	static QStringList showInts(const QList<int> &list);
}; // class Tools
} // namespace AutopinPlus
