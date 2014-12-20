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
#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ProcessTree.h>
//#include <AutopinPlus/OS/Linux/OSServicesLinux.h>
#include <list>
#include <QObject>
#include <QRegExp>
#include <string>

namespace AutopinPlus {

extern "C" {
#include "libautopin+_msg.h"
}

namespace OS {
namespace Linux {
class OSServicesLinux;
}
}

/*!
 * \brief This class implements the internal representation of the process observed by autopin+
 *
 * This class is responsible for attaching to or staring a process specified by the user in the
 * configuration. Moreover it emits signals when a task has been created or terminated. The
 * communication with the observed process is also handled in this class.
 */
class ObservedProcess : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the Configuration object containing the current configuration
	 * \param[in] service	Reference to the current instance of a subclass of OSServices
	 * \param[in] context	Reference to the context of the object calling the constructor
	 */
	ObservedProcess(const Configuration &config, OS::Linux::OSServicesLinux &service, AutopinContext &context);

	/*!
	 * \brief Initializes the observed process
	 *
	 * Reads the configuration options for the observed process.
	 *
	 */
	void init();

	/*!
	 * \brief Starts or attaches to the process specified in the configuration
	 *
	 * Depending on the configuration options this method attaches to a process with the pid
	 * specified by the user or creates a new process based on a command in the configuration.
	 * This method can only be called once. Further calls will have no effect. If both pid
	  * and a command have been specified in the configuration autopin+ will try to attach to
	 * the process with the specified id first.
	 */
	void start();

	/*!
	 * \brief Returns the pid of the observed process
	 *
	 * \return Returns the pid of the process. If there is any problem with the process or the process
	 * 	has not been started yet -1 is retured
	 */
	int getPid() const;

	/*!
	 * \brief Returns the current execution phase of the process
	 *
	 * \return The current execution phase stored in phase
	 */
	int getExecutionPhase() const;

	/*!
	 * \brief Returns the command the observed process has been started with
	 *
	 * \return Returns the command teh process has been started with. If the command
	 * 	is not available or if there is any error the result will be empty.
	 */
	QString getCmd() const;

	/*!
	 * \brief Determines if the observed process has been started by autopin+
	 *
	 * \return true if the observed process has been started by autopin+ and
	 * 	false otherwise
	 *
	 */
	bool getExec() const;

	/*!
	 * \brief Determine if process tracing is active
	 *
	 * \return true if process tracing is active and false in all other cases
	 */
	bool getTrace() const;

	/*!
	 * \brief Reports if autopin+ has already attached to or started a process
	 *
	 * \return True after start() has returned successfully, false in all other cases
	 */
	bool isRunning() const;

	/*!
	 * \brief Creates and returns the ProcessTree of the observed process
	 *
	 * The root of a process tree is the observed process. All children are direct and indirect children
	 * processes of the observed process.
	 *
	 * \return A new process tree for the observed process. If autopin+ has not attached to or started a
	 * 	process yet the tree will be empty.
	 *
	 * \sa ProcessTree
	 */
	ProcessTree getProcessTree() const;

	/*!
	 * \brief Returns the desired address for the communication channel
	 *
	 * The return value depends on the operating system. The value is requested by the corresponding
	 * subclass of OSServices. On Linux this method returns a file path for a UNIX domain socket
	 *
	 * \return String representation for the address of the communication channel. If the user has enabled
	 * 	the communication channel without specifying an address the method returns a default address which
	 * 	is provided by the OSServices.
	 *
	 */
	QString getCommChanAddr() const;

	/*!
	 * \brief Sets the minimum between two phase change notifications from the observed process
	 *
	 * \param[in] interval The desired interval in seconds
	 *
	 */
	void setPhaseNotificationInterval(int interval) const;

	/*!
	 * \brief Returns the timout for the communication channel
	 *
	 * \return Timeout for the communication channel
	 *
	 */
	int getCommTimeout() const;

signals:
	/*!
	 * \brief Signals the termination of a task of the observed process
	 *
	 * \param[in] tid	id of the task (may depend on the operating system)
	 */
	void sig_TaskTerminated(int tid);

	/*!
	 * \brief Signals the creation of a new task of the observed process
	 *
	 * \param[in] tid	id of the task (may depend on the operating system)
	 */
	void sig_TaskCreated(int tid);

	/*!
	 * \brief Signals that the observed process has changed its execution phase
	 *
	 * \param[in] newphase integer representing the new execution phase of the
	 * 	observed process
	 */
	void sig_PhaseChanged(int newphase);

	/*!
	 * \brief Signals new user-defined messages from the communication channel
	 *
	 * \param[in] arg The argument value of the messages
	 * \param[in] val The value stored in the messages
	 *
	 */
	void sig_UserMessage(int arg, double val);

	/*!
	 * \brief
	 */
	void sig_ProcTerminated();

  public slots:
	/*!
	 * \brief handles the termination of the process
	 *
	 * \param[in] pid        the process id of the process
	 * \param[in] exit_code  the exit code of the process
	 */
	void slot_ProcTerminated(int pid, int exit_code);

	/*!
	 * \brief Handles the termination of a task of the observed process
	 *
	 * \param[in] tid	id of the task (may depend on the operating system)
	 */
	void slot_TaskTerminated(int tid);
	/*!
	 * \brief Handles the creation of a new task of the observed process
	 *
	 * \param[in] tid	id of the task (may depend on the operating system)
	 */
	void slot_TaskCreated(int tid);

	/*!
	 * \brief Handles new messages from the communication channel
	 *
	 * \param[in] msg	The new messages
	 *
	 */
	void slot_CommChannel(struct autopin_msg msg);

  private:
	//@{
	/*!
	 * Member variable referencing the current instance of the (sub-)class. This variable is set in the constructor.
	 */
	const Configuration &config;
	OS::Linux::OSServicesLinux &service;
	//@}

	/*!
	 * The runtime context
	 */
	AutopinContext &context;

	/*!
	 * \brief Regular expression for recognizing integer values in strings
	 */
	QRegExp integer;

	/*!
	 * Stores the pid of the observed process. If there autopin+ has not attached to or started a
	 * process yet the value of this varialbe is -1.
	 */
	int pid;

	/*!
	 * Stores if the observed process has been started by autopin+
	 *
	 */
	bool exec;

	/*!
	 * Stores the command the observed process has been started with
	 */
	QString cmd;

	/*!
	 * Stores if autopin+ process tracing is enabled
	 */
	bool trace;

	/*!
	 * Stores if the process is currently running
	 */
	bool running;

	/*!
	 * Stores the execution phase of the process. The default value
	 * is 0.
	 */
	int phase;

	/*!
	 * Stores the address of the communication channel
	 */
	QString comm_addr;

	/*!
	 * Timeout for the communication channel
	 */
	int comm_timeout;
};

} // namespace AutopinPlus
