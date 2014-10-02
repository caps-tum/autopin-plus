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

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ObservedProcess.h>
#include <AutopinPlus/ProcessTree.h>
#include <QMutex>
#include <QString>
#include <QThread>

namespace AutopinPlus {
namespace OS {
namespace Linux {

/*!
 * \brief Realizes the process tracing for OSServicesLinux
 *
 * This class implements a thread which traces processes using ptrace.
 *
 * \sa OSServicesLinux
 */
class TraceThread : public QThread {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * Creates but does not start a new TraceThread.
	 *
	 * \param[in]	context	Refernce to the context of the object calling the constructor
	 */
	explicit TraceThread(const AutopinContext &context);

	/*!
	 * \brief Initializes and starts the thread
	 *
	 * \param[in] observed_process A pointer to the process the thread is
	 * 	going to trace
	 * \return A pointer to a locked mutex. This mutex will be unlocked when
	 * 	the thread has attached to all tasks and is used by
	 * 	OSServicesLinux::attachToProcess() to wait until the thread has
	 * 	finished attaching to all tasks.
	 */
	QMutex *init(ObservedProcess *observed_process);

	/*!
	 * \brief Stop process tracing and exit thread
	 *
	 * This function block until the thread has exited.
	 */
	void deinit();

	/*!
	 * \brief Signal handler for SIGALRM
	 */
	static void alrmSignalHandler(int param);

  protected:
	/*!
	 * \brief The method which will be executed by the thread
	 *
	 * In this method ptrace is attached to the observed process. As long
	 * as the process is running the thread will call waitpid and trace
	 * the threads and child processes of the observed process.
	 *
	 * \attention Attaching to a process which has not been started by autopin+
	 * 	might result in lots of warnings as ptrace cannot attach to tasks
	 * 	which are about to quit.
	 *
	 * \sa attach()
	 */
	void run() override;

signals:
	/*!
	 * \brief Signals that a new task has been created
	 *
	 * \param[in] tid tid of the new task
	 */
	void sig_TaskCreated(int tid);

	/*!
	 * \brief Signals that task has terminated
	 *
	 * \param[in] tid tid of the task
	 */
	void sig_TaskTerminated(int tid);

  private:
	/*!
	 * The runtime context
	 */
	AutopinContext context;

	/*!
	 * pid of the traced process
	 */
	pid_t pid;

	/*!
	 * pid of the traced process as a string
	 */
	QString pid_str;

	/*!
	 * Tasks that are currently traced
	 */
	ProcessTree::autopin_tid_list tasks;

	/*!
	 * Intermediate storage for new tasks
	 */
	ProcessTree::autopin_tid_list new_tasks;

	/*!
	 * Pointer to the internal representation of the process observed by the thread
	 */
	ObservedProcess *observed_process;

	/*!
	 * Options for ptrace
	 */
	const unsigned long ptrace_opt;

	/*!
	 * \brief Setup SIGALRM handler
	 *
	 * Unmasks SIGALRM and sets the handler for SIGALRM
	 * to alrmSignalHandler(). To ensure that SIGALRM is
	 * always catched by the thread SIGALRM is masked before
	 * starting the thread in OSServicesLinux::attachToProcess().
	 */
	void setupSignalHandler();

	/*!
	 * \brief Continues tasks stopped by ptrace
	 *
	 * \param[in] pid	The pid of the task which will be continued
	 * \param[in] sig	The signal which will be sent to the continued task
	 */
	void ptraceContinue(pid_t pid, unsigned long sig = 0);

	/*!
	 * \brief Handles the creation of a new task
	 *
	 * \param[in] pid The pid of the new task
	 */
	void newTask(int pid);

	/*!
	 * \brief Attaches to already existing tasks of the process
	 *
	 * This method is called when the tracing of the process is started.
	 */
	void attach();

	/*!
	 * Mutex for signaling that the thread has attached to all tasks
	 */
	QMutex trace_mutex;

	/*!
	 * Variable indicating that the thread shall exit
	 */
	bool exreq;

	/*!
	 * Variable indicating alarm
	 */
	static bool alarm_occured;
};

} // namespace Linux
} // namespace OS
} // namespace AutopinPlus
