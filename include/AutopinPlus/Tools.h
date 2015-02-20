/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <qlist.h>		 // for QList
#include <qstring.h>	 // for QString
#include <qstringlist.h> // for QStringList
#include <utility>		 // for pair

namespace AutopinPlus {
namespace Tools {

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
double readDouble(const QString &string);

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
int readInt(const QString &string);

/*!
 * \brief Parses a string list to a list of ints.
 *
 * This function parses the specified string list to a list of ints. If this fails, an exception is thrown.
 *
 * \param[in] list The string list to be parsed.
 *
 * \exception Exception This exception will be thrown if the string list could not be parsed.
 *
 * \return The parsed list of ints.
 */
QList<int> readInts(const QStringList &list);

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
QString readLine(const QString &path);

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
std::pair<QString, QString> readPair(const QString &string, const QString &separator = "=");

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
unsigned long readULong(const QString &string);

/*!
 * \brief Converts a list of ints to a string list.
 *
 * This function converts the specified list of ints to a string list. If this fails, an exception is thrown.
 *
 * \param[in] list The list of ints to be converted.
 *
 * \return A string list representing the supplied list of ints.
 */
QStringList showInts(const QList<int> &list);

} // namespace Tools
} // namespace AutopinPlus
