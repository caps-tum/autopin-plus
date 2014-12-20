/*
 * Autopin+ - Automatic thread-to-core-pinning tool
 * Copyright (C) 2014 LRR
 *
 * Author:
 * Lukas Fürmetz
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

#include <QSocketNotifier>
#include <signal.h>

namespace AutopinPlus {
namespace OS {

/*!
 * \brief Singleton, which dispatches unix signals
 *
 * This class handles unix signals and converts unix signals to a qt signal
 * See also: http://qt-project.org/doc/qt-4.8/unix-signals.html
 */

class SignalDispatcher : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Get the instance of the SignalDispatcher
	 */
	static SignalDispatcher &getInstance();

	/*!
	 * Must be called before using the SignalDispatcher.
	 * Should be called usally once at the beginning of
	 * the lifetime of the application.
	 */
	static int setupSignalHandler();

	/*!
	 * Delete functions, so we don't accidently end up with another
	 * instance of SignalDispatcher
	 */
	SignalDispatcher(SignalDispatcher const &) = delete;
	void operator=(SignalDispatcher const &) = delete;

  public slots:
	/*!
	 * Slot for handling the sigchld signal.
	 */
	void slot_handleSigChld();

signals:
	/*!
	 * Emitted, when the dispatcher receives a SIGCHLD
	 *
	 * \param[in] pid        Process id of the child
	 * \param[in] exit_code  Exit code of the child
	 */
	void sig_ProcTerminated(int pid, int exit_code);

  private:
	/*!
	 * \brief Constructor
	 */
	SignalDispatcher();

	/*
	 * \brief fieldescriptor for the socketpair
	 */
	static int sigchldFd[2];

	/*
	 * \brief SocketNotifier for sigchldFd
	 */
	QSocketNotifier *snChld;

	/*!
	 * \brief Initalizes the SignalDispatcher
	 */
	int init();

	/*!
	 * \brief Handles sigchld
	 */
	static void chldSignalHandler(int param, siginfo_t *info, void *paramv);
};

} // namespace OS
} // namespace AutopinPlus
