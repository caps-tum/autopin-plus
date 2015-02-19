/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <QSocketNotifier>
#include <AutopinPlus/AutopinContext.h>
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

	/*!
	 * \brief AutopinContext used by this class to log error messages
	 */
	const AutopinContext context;

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
