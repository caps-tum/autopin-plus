/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
