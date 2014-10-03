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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <exception>	// for exception
#include <qbytearray.h> // for QByteArray
#include <qstring.h>	// for QString

namespace AutopinPlus {
/*!
 * \brief Subclass of std::exception for use in Autopin+
 *
 * This class is a subclass of std::exception. It should be preferred over that class, since catching it
 * guarantees that you only catch exceptions originating from with Autopin+ and not those from other code.
 */
class Exception : public std::exception {
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] message A message describing the problem
	 */
	Exception(QString message) noexcept;

	/*!
	 * \brief Get a description of the problem
	 *
	 * Returns a pointer to a \0-terminated, UTF8-encoded string describing the problem.
	 */
	virtual const char *what() const noexcept;

  private:
	/*!
	 * \brief Store the message describing the problem
	 *
	 * This variable stores the message describing the problem. Using a QByteArray has the
	 * advantage that conversions between a "QString" and a traditional C-String are trivial.
	 */
	QByteArray message;
}; // class Exception
} // namespace AutopinPlus
