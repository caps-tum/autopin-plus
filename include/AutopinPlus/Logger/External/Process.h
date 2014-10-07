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

#include <qprocess.h> // for QProcess

namespace AutopinPlus {
namespace Logger {
namespace External {

/*!
 * \brief A wrapper around QProcess which allows spawning a process without waiting for them to terminate.
 *
 * This class behaves just like a QProcess, except that its destructor will just close all communication channels
 * (stdin, stdout, stderr) and then set the internal state to "NotRunning" without actually waiting for the process to
 * terminate.
 */
class Process : public QProcess {
  public:
	/*!
	 * \brief Destructor.
	 *
	 * This destructor closes all communication channels (stdin, stdout, stderr) and then sets the internal state to
	 * "NotRunning" without actually waiting for the process to terminate.
	 */
	~Process();
}; // class Process

} // namespace External
} // namespace Logger
} // namespace AutopinPlus
