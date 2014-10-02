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
#include <deque>
#include <map>
#include <QObject>
#include <QString>
#include <string>

extern "C" {
#include <AutopinPlus/libautopin+_msg.h>
}

namespace AutopinPlus {

/*!
 * \brief Abstracts all resources of the underlying operating system needed by autopin+
 *
 * This is an abstract base class defining the view of autopin+ on the services of the
 * operating system. An implementation of this class is needed for every operating system
 * autopin+ is intended to run on.
 *
 */
class OSServices : public QObject {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in]	context	Refernce to the context of the object calling the constructor
	 */
	explicit OSServices(const AutopinContext &context);

	virtual ~OSServices() {}

	/*!
	 * \brief Executes initialization procedures if necessary
	 *
	 * This method is called in Autopin::slot_autopinSetup() after creation of
	 * an object of this class. If there is already another instance of OSServices
	 * running this method is expected to return false.
	 *
	 * The initialization will fail if another OSServices instance is currently in use.
	 */
	virtual void init() = 0;

	/*!
	 * \brief Returns a pointer to the current instance of a subclass
	 *
	 * \return A pointer to an instance of a subclass of OSServices which is currently
	 * 	initialized. If there is no such an instance the method will return 0.
	 */
	const OSServices *getCurrentService();

	/*!
	 * \brief Returns the hostname of the host running autopin+
	 *
	 * \return The Hostname as a QString
	 */
	virtual QString getHostname() = 0;

	/*!
	 * \brief Returns a default address for the communication channel
	 *
	 * \return A string representation of the default address of the
	 * 	communication channel
	 */
	virtual QString getCommDefaultAddr() = 0;

	/*!
	 * \brief Creates a new process
	 *
	 * \param[in] cmd	The command which shall be executed
	 * \param[in] wait	If this argument is set to true the new process will
	 * 	be started only when autopin+ has attached via attachToProcess()
	 *
	 * \return The pid of the new process. If no process could be created the return value
	 * 	will be -1
	 */
	virtual int createProcess(QString cmd, bool wait) = 0;

	/*!
	 * \brief Assigns a task to a certain core
	 *
	 * \param[in] tid	The id of the task
	 * \param[in] cpu	The number of the core the task will be assigned to
	 *
	 */
	virtual void setAffinity(int tid, int cpu) = 0;

	/*!
	 * \brief Attaches autopin+ to a process
	 *
	 * After successfully calling this method the process will be watched and the
	 * creation and termination of tasks will be signalled via sig_TaskCreated()
	 * and sig_TaskTerminated().
	 *
	 * \param[in] observed_process The process which shall be watched
	 */
	virtual void attachToProcess(ObservedProcess *observed_process) = 0;

	/*!
	 * \brief Detaches autopin+ from a process
	 *
	 * If autopin+ is currently not attached to a process this method
	 * won't do anything.
	 */
	virtual void detachFromProcess() = 0;

	/*!
	 * \brief Initializes the communication channel
	 *
	 * This method has to be called before connection requests from an
	 * observed process can be accepted.
	 *
	 * \param[in] proc The observed process to which autopin+ will connect
	 *
	 */
	virtual void initCommChannel(ObservedProcess *proc) = 0;

	/*!
	 * \brief Accepts connection requests
	 *
	 * This waits for the observed process to connect and blocks
	 * until the connection is established or timeout seconds have
	 * passed.
	 *
	 * \param[in] timeout Timeout for the connection in seconds
	 *
	 */
	virtual void connectCommChannel(int timeout) = 0;

	/*!
	 * \brief Deletes all data for the communication channel
	 *
	 */
	virtual void deinitCommChannel() = 0;

	/*!
	 * \brief Sends a message to the observed process
	 *
	 * Messages can only be sent if the communication channel
	 * is connected to the observed process.
	 *
	 * \param[in] event_id The event_id of the message
	 * \param[in] arg The argument of the message
	 * \param[in] val The value of the message
	 *
	 */
	virtual void sendMsg(int event_id, int arg, double val) = 0;

	/*!
	 * \brief Returns the pid of a process with a certain name
	 *
	 * \param[in] proc	The name of the process
	 *
	 * \return A list of pids of all processes with the name proc. If no such process
	 * 	exits or an error has occured the list will be empty.
	 */
	virtual ProcessTree::autopin_tid_list getPid(QString proc) = 0;

	/*!
	 * \brief Returns the command a process has been started with
	 *
	 * \param[in] pid	The command of the process
	 *
	 * \return The command the process with the specified pid has been started with. If
	 * 	no process with the given pid exists or an error has occured the result will
	 * 	be empty.
	 *
	 */
	virtual QString getCmd(int pid) = 0;

	/*!
	 * \brief Determines all threads of a process
	 *
	 * \param[in] pid	The pid of the process
	 *
	 * \return A list containing the pids of all threads of the process. If no process
	 * 	with the given pid exists or an error has occured an empty list will be returned.
	 */
	virtual ProcessTree::autopin_tid_list getProcessThreads(int pid) = 0;

	/*!
	 * \brief Returns all direct child processes of a process
	 *
	 * \param[in] pid	The pid of the process
	 *
	 * \return A list with the pids of all child processes of the process. If no process
	 * 	with the given pid exists or an error has occured the list will be empty.
	 */
	virtual ProcessTree::autopin_tid_list getChildProcesses(int pid) = 0;

	/*!
	 * \brief Returns an id for a process which can be used for consistent sorting
	 *
	 * The function returns an id for each process/task which can be used for
	 * consitent sorting of tasks (e. g. by the time when the task was created). This
	 * is important for pinning strategies which allow the user to select certain threads
	 * in the configuration.
	 *
	 * The standard implementation returns the tid provided in the argument of the method.
	 *
	 * \param[in] tid	tid of the task for which an sorting id is requested
	 *
	 * \return The input pid
	 */
	virtual int getTaskSortId(int tid);

signals:
	/*!
	 * \brief Signals that a task has terminated
	 *
	 * New tasks can only be signalled if autopin+ has been attached to a process
	 * via attachToProcess().
	 *
	 * \param[in] tid	The tid of the terminated task
	 */
	void sig_TaskTerminated(int tid);

	/*!
	 * \brief Signals that a new task has been created
	 *
	 * New tasks can only be signalled if autopin+ has been attached to a process
	 * via attachToProcess().
	 *
	 * \param[in] tid	The tid of the new task
	 */
	void sig_TaskCreated(int tid);

	/*!
	 * \brief Signals new messages from the communication channel
	 *
	 * \param[in] msg	The new message
	*/
	void sig_CommChannel(struct autopin_msg msg);

  protected:
	/*!
	 * The runtime context
	 */
	AutopinContext context;

	/*!
	 * Stores a pointer to the acitve instance.
	 */
	static OSServices *current_service;
};

} // namespace AutopinPlus
