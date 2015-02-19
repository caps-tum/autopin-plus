/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/OS/TraceThread.h>
#include <deque>
#include <QMutex>
#include <QRegExp>
#include <QSocketNotifier>
#include <QStringList>
#include <signal.h>
#include <string>

namespace AutopinPlus {
namespace OS {

/*!
 * \brief Implementation of the OSServices for Linux
 *
 * The tracing of a process is implemented separately in TraceThread.
 */
class OSServices : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in]	context	Reference to the context of the object calling the constructor
	 */
	explicit OSServices(AutopinContext &context);

	/*!
	 * \brief Destructor
	 */
	virtual ~OSServices();

	/*!
	 * \brief Executes initialization procedures if necessary
	 *
	 * This method is called in Autopin::slot_autopinSetup() after creation of
	 * an object of this class. If there is already another instance of OSServices
	 * running this method is expected to return false.
	 *
	 * The initialization will fail if another OSServices instance is currently in use.
	 */
	void init();

	/*!
	 * \brief Returns the hostname of the host running autopin+
	 *
	 * \return The Hostname as a QString
	 */
	QString getHostname();

	/*!
	 * \brief Returns a default address for the communication channel
	 *
	 * \return A string representation of the default address of the
	 * 	communication channel
	 */
	QString getCommDefaultAddr();

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
	int createProcess(QString cmd, bool wait);

	/*!
	 * \brief Assigns a task to a certain core
	 *
	 * \param[in] tid	The id of the task
	 * \param[in] cpu	The number of the core the task will be assigned to
	 *
	 * \return The result of sched_setaffinity
	 */
	int setAffinity(int tid, int cpu);

	/*!
	 * \brief Attaches autopin+ to a process
	 *
	 * After successfully calling this method the process will be watched and the
	 * creation and termination of tasks will be signalled via sig_TaskCreated()
	 * and sig_TaskTerminated().
	 *
	 * \param[in] observed_process The process which shall be watched
	 */
	void attachToProcess(ObservedProcess *observed_process);

	/*!
	 * \brief Detaches autopin+ from a process
	 *
	 * If autopin+ is currently not attached to a process this method
	 * won't do anything.
	 */
	void detachFromProcess();

	/*!
	 * \brief Detaches autopin+ from a process
	 *
	 * If autopin+ is currently not attached to a process this method
	 * won't do anything.
	 */
	void initCommChannel(ObservedProcess *proc);

	/*!
	 * \brief Accepts connection requests
	 *
	 * This waits for the observed process to connect and blocks
	 * until the connection is established or timeout seconds have
	 * passed.
	 *
	 * \param[in] timeout Timeout for the connection in seconds
	 */
	void connectCommChannel(int timeout);

	/*!
	 * \brief Deletes all data for the communication channel
	 */
	void deinitCommChannel();

	/*!
	 * \brief Sends a message to the observed process
	 *
	 * Messages can only be sent if the communication channel
	 * is connected to the observed process.
	 *
	 * \param[in] event_id The event_id of the message
	 * \param[in] arg The argument of the message
	 * \param[in] val The value of the message
	 */
	void sendMsg(int event_id, int arg, double val);

	/*!
	 * \brief Returns the pid of a process with a certain name
	 *
	 * \param[in] proc	The name of the process
	 *
	 * \return A list of pids of all processes with the name proc. If no such process
	 * 	exits or an error has occured the list will be empty.
	 */
	ProcessTree::autopin_tid_list getPid(QString proc);

	/*!
	 * \brief Returns the command a process has been started with
	 *
	 * \param[in] pid	The command of the process
	 *
	 * \return The command the process with the specified pid has been started with. If
	 * 	no process with the given pid exists or an error has occured the result will
	 * 	be empty.
	 */
	QString getCmd(int pid);

	/*!
	 * \brief Determines all threads of a process
	 *
	 * \param[in] pid	The pid of the process
	 *
	 * \return A list containing the pids of all threads of the process. If no process
	 * 	with the given pid exists or an error has occured an empty list will be returned.
	 */
	ProcessTree::autopin_tid_list getProcessThreads(int pid);

	/*!
	 * \brief Returns all direct child processes of a process
	 *
	 * \param[in] pid	The pid of the process
	 *
	 * \return A list with the pids of all child processes of the process. If no process
	 * 	with the given pid exists or an error has occured the list will be empty.
	 */
	ProcessTree::autopin_tid_list getChildProcesses(int pid);

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
	int getTaskSortId(int tid);

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
	 * \brief Handles SIGUSR1
	 *
	 * If the observed process is started by autopin+ an process tracing is active
	 * the new process is blocked until autopin+ has attached. This is signaled
	 * via SIGUSR1.
	 */
	static void usrSignalHandler(int param);

	/*!
	 * \brief Gets the number of availaible cpus.
	 *
	 * \return Returns the number of availaible cpus.
	 */
	static int getCpuCount();

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

  public slots:
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
	using thread_list = std::list<int>;

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

	/*!
	 * The runtime context
	 */
	AutopinContext &context;
};

} // namespace OS
} // namespace AutopinPlus
