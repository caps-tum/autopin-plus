/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
	explicit Exception(QString message) noexcept;

	/*!
	 * \brief Destructor
	 */
	virtual ~Exception() noexcept;

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
