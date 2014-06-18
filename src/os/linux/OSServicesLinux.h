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

#include "OSServices.h"

#include <QRegExp>
#include <QStringList>
#include <QMutex>
#include <QSocketNotifier>
#include <string>
#include <deque>
#include <signal.h>

#include "TraceThread.h"

/*!
 * \brief Implementation of the OSServices for Linux
 *
 * The tracing of a process is implemented separately in TraceThread.
 */
class OSServicesLinux : public OSServices {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in]	context	Refernce to the context of the object calling the constructor
	 */
	explicit OSServicesLinux(const AutopinContext &context);

	/*!
	 * \brief Destructor
	 */
	virtual ~OSServicesLinux();

	void init() override;
	QString getHostname() override;
	QString getCommDefaultAddr() override;
	int createProcess(QString cmd, bool wait) override;
	void setAffinity(int tid, int cpu) override;
	void attachToProcess(ObservedProcess *observed_process) override;
	void detachFromProcess() override;
	void initCommChannel(ObservedProcess *proc) override;
	void deinitCommChannel() override;
	void connectCommChannel(int timeout) override;
	void sendMsg(int event_id, int arg, double val) override;
	ProcessTree::autopin_tid_list getPid(QString proc) override;
	QString getCmd(int pid) override;

	ProcessTree::autopin_tid_list getProcessThreads(int pid) override;
	ProcessTree::autopin_tid_list getChildProcesses(int pid) override;

	/*!
	 * \brief Returns an id for a process which can be used for consistent sorting
	 *
	 * The function returns an id for each process/task which can be used for
	 * consitent sorting of tasks (e. g. by the time when the task was created). This
	 * is important for pinning strategies which allow the user to select certain threads
	 * in the configuration.
	 *
	 * This implementation returns the creation time of the task with the given pid.
	 *
	 * \param[in] tid	tid of the task for which an sorting id is requested
	 *
	 * \return On success the creation time of the task with the given input pid is returned.
	 * 	Otherwise, the method returns -1.
	 */
	int getTaskSortId(int tid) override;

	/*!
	 * \brief Returns the hostname of the host running autopin+
	 *
	 * This is the static version of the getHostname method which
	 * is used by the class Autopin before the OSServices are
	 * initialized.
	 *
	 * \return The Hostname as a QString
	 */
	static QString getHostname_static();

	/*!
	 * \brief Handles SIGCHL for created processes
	 */
	static void chldSignalHandler(int param, siginfo_t *info, void *paramv);

	/*!
	 * \brief Handles SIGUSR1
	 *
	 * If the observed process is started by autopin+ an process tracing is active
	 * the new process is blocked until autopin+ has attached. This is signaled
	 * via SIGUSR1.
	 */
	static void usrSignalHandler(int param);

  public slots:
	/*!
	 * \brief Qt Signal handler for SIGHLD
	 */
	void slot_handleSigChld();

	/*!
	 * \brief Receives new messages from the communication channel
	 *
	 * \param[in] socket The descriptor of the socket
	 *
	 */
	void slot_msgReceived(int socket);

  private:
	/*!
	 * \brief Data type for storing a list of tids
	 */
	typedef std::list<int> thread_list;

	/*!
	 * Regular expression for integer values
	 */
	QRegExp integer;

	/*!
	 * Thread uses for tracing the process when calling attachToProcess()
	 *
	 * \sa TraceThread
	 */
	TraceThread tracer;

	/*!
	 * \brief Returns an entry from /proc/pid/stat
	 *
	 * This method is not thread-safe!
	 *
	 * \param[in] tid The tid of the process/thread
	 * \param[in] index The index of the desired entry. The counting starts at 0.
	 * \param[in] error Can be used to block errors
	 *
	 * \return The desires value as a string. If an error has occured the string
	 * 	is empty
	 */
	QString getProcEntry(int tid, int index, bool error = true);

	/*!
	 * \brief Extracts all arguments from a command line expression
	 *
	 * This method is used to convert an expression of the form <I>cmd -arg</I>
	 * to a list of strings containing the elements <I>cmd</I> and <I>-arg</I>
	 *
	 * \param[in] cmd The command which will be converted into a list of strings
	 *
	 * \return A list of strings containing all parts of cmd in the same order as
	 * 	they appear in the command (from left to right)
	 */
	static QStringList getArguments(QString cmd);

	/*!
	 * \brief Converts a list of QStrings to a list of integers
	 *
	 * \param[in] qlist A list of integers represented as QStrings
	 *
	 * \return A list of integers
	 */
	static ProcessTree::autopin_tid_list convertQStringList(QStringList &qlist);

	/*!
	 * Mutex for thread-safe access
	 */
	QMutex mutex;

	/*!
	 * Mutex for protecting the attachToProcess() method
	 */
	QMutex attach;

	/*!
	 * File descriptor for the QSocketNotifier. Needed
	 * for signal handling.
	 */
	static int sigchldFd[2];

	/*!
	 * Needed signal handling of SIGCHLD
	 */
	QSocketNotifier *snChld;

	/*!
	 * Stores old values for SIGCHLD
	 */
	struct sigaction old_chld;

	/*!
	 * Monitors the connection to the observed process
	 */
	QSocketNotifier *comm_notifier;

	/*!
	 * The path of the UNIX domain socket
	 */
	QString socket_path;

	/*!
	 * Server socket
	 */
	int server_socket;

	/*!
	 * Socket for the communication with the observed process
	 */
	int client_socket;

	/*!
	 * Stores if autopin+ has already attached
	 */
	static bool autopin_attached;
};
