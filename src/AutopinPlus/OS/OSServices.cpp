/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/OS/OSServices.h>

#include <AutopinPlus/ObservedProcess.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <sched.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

/*!
 * \brief Maximum length for paths of UNIX domain sockets
 *
 * Taken from sys/un.h
 */
#define UNIX_PATH_MAX 108

namespace AutopinPlus {
namespace OS {

OSServices::OSServices(AutopinContext &context)
	: tracer(context), comm_notifier(nullptr), server_socket(-1), client_socket(-1), context(context) {
	integer = QRegExp("\\d+");

	connect(&tracer, SIGNAL(sig_TaskCreated(int)), this, SIGNAL(sig_TaskCreated(int)));
	connect(&tracer, SIGNAL(sig_TaskTerminated(int)), this, SIGNAL(sig_TaskTerminated(int)));
}

OSServices::~OSServices() {
	detachFromProcess();
	deinitCommChannel();

	if (tracer.isRunning()) tracer.terminate();

	if (comm_notifier != nullptr) delete comm_notifier;
}

bool OSServices::autopin_attached = false;

void OSServices::init() { context.info("Initializing OS services"); }

QString OSServices::getHostname() { return getHostname_static(); }

QString OSServices::getCommDefaultAddr() {
	QString result;
	const char *homedir = getenv("HOME");

	if (homedir == nullptr) {
		context.report(Error::COMM, "default_addr", "Cannot determine the user's home directory");
		return result;
	}

	result = homedir;
	result += "/.autopin_socket";

	return result;
}

QString OSServices::getHostname_static() {
	QString qhostname;

	char hostname[30];
	gethostname(hostname, 30);
	qhostname = hostname;

	return qhostname;
}

int OSServices::createProcess(QString cmd, bool wait) {
	pid_t pid;

	struct sigaction old_usr;

	// install handler for signal SIGUSR1
	if (wait) {
		int ret = 0;
		struct sigaction usr;
		memset(&usr, 0, sizeof(struct sigaction));
		usr.sa_handler = usrSignalHandler;
		usr.sa_flags = SA_RESTART;
		ret |= sigemptyset(&usr.sa_mask);
		ret |= sigaction(SIGUSR1, &usr, &old_usr);
		if (ret != 0) {
			context.report(Error::SYSTEM, "sigset", "Cannot setup signal handling");
			return -1;
		}
	}

	pid = fork();

	if (pid == 0) {
		// in the child

		std::string s = cmd.toStdString();
		char *argc = new char[s.size() + 1];
		for (std::size_t i = 0; i < s.size(); ++i) argc[i] = s[i];
		argc[cmd.size()] = '\0';

		char **argv = new char *[cmd.size() / 2];
		{
			char **temp = argv;
			while (*argc != '\0') {
				// replace whitespace with \0
				// while is ok as long as argc is null terminated
				while (*argc == ' ' || *argc == '\t' || *argc == '\n') *argc++ = '\0';
				*temp = argc;
				++temp;
				// wait until whitespace or '\0'
				while (*argc != '\0' && *argc != ' ' && *argc != '\t' && *argc != '\n') ++argc;
			}
			*temp = nullptr;
		}

		// Get new pid
		pid = getpid();

		context.debug(QString("Binary to start: ") + *argv);

		if (wait) {
			context.debug("Waiting for autopin to attach");

			while (!autopin_attached) {

				context.debug("Wait for signal from autopin+");
				usleep(1000);
			}

			context.debug("Autopin has attached!");
		}

		execvp(*argv, argv);
		// execvp(bin.toStdString().c_str(), args);

		context.report(Error::PROCESS, "create", QString("Could not create new process from binary ") + *argv);
		exit(-1);

	} else {
		// Restore original signal handler
		if (wait) {
			int ret;
			ret = sigaction(SIGUSR1, &old_usr, nullptr);
			if (ret != 0) {
				context.report(Error::SYSTEM, "sigset", "Cannot setup signal handling");
				return -1;
			}
		}

		return pid;
	}

	return -1;
}

void OSServices::attachToProcess(ObservedProcess *observed_process) {
	QMutexLocker locker(&attach);
	int ret = 0;

	if (tracer.isRunning()) {
		context.report(Error::PROC_TRACE, "in_use", "Process tracing is already running");
	}

	// Block the alarm signal so that it will always be delivered to the TraceThread
	sigset_t sigset;
	ret |= sigemptyset(&sigset);
	ret |= sigaddset(&sigset, SIGALRM);
	ret |= pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
	if (ret != 0) context.report(Error::SYSTEM, "sigset", "Cannot setup signal handling");

	context.debug("Starting TraceThread");
	QMutex *traceattach;
	traceattach = tracer.init(observed_process);

	// Wait until the thread has attached to all tasks
	context.debug("OSServices is waiting until the thread has attached");
	traceattach->lock();
	traceattach->unlock();
}

void OSServices::detachFromProcess() {
	if (!tracer.isRunning()) return;

	context.info("Detaching from the observed process");
	tracer.deinit();
}

void OSServices::initCommChannel(ObservedProcess *proc) {
	if (server_socket != -1)
		context.report(Error::COMM, "already_initialized", "The communication channel is already initialized");

	// Create a new server socket and bind it to a path
	server_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (server_socket == -1) context.report(Error::COMM, "socket", "Could not create a new socket");

	// Get flags of the socket
	long socket_flags = fcntl(server_socket, F_GETFL);
	if (socket_flags == -1) {
		close(server_socket);
		server_socket = -1;
		context.report(Error::COMM, "socket", "Cannot access socket properties");
	}
	socket_flags |= O_NONBLOCK;
	int result = fcntl(server_socket, F_SETFL, socket_flags);
	if (result == -1) {
		close(server_socket);
		server_socket = -1;
		context.report(Error::COMM, "socket", "Cannot set socket properties");
	}

	// Create the socket path
	socket_path = proc->getCommChanAddr();
	if (socket_path.size() + 1 > UNIX_PATH_MAX)
		context.report(Error::COMM, "comm_target", "The path for the communication socket exceeds the maximum length");

	// Create the address structure for the socket
	struct sockaddr_un socket_addr;
	socket_addr.sun_family = AF_UNIX;
	strcpy(socket_addr.sun_path, socket_path.toStdString().c_str());

	// Bind the socket
	result = bind(server_socket, (struct sockaddr *)&socket_addr, sizeof(socket_addr));
	if (result == -1) {
		close(server_socket);
		server_socket = -1;
		context.report(Error::COMM, "socket", "Cannot bind the communication socket.");
	}

	// Make the socket a listening socket
	result = listen(server_socket, 1);
	if (result == -1) {
		close(server_socket);
		remove(socket_path.toStdString().c_str());
		server_socket = -1;
		context.report(Error::COMM, "socket", "Cannot setup communication socket");
	}
}

void OSServices::deinitCommChannel() {
	if (comm_notifier != nullptr) {
		delete comm_notifier;
		comm_notifier = nullptr;
	}
	if (client_socket != -1) close(client_socket);
	if (server_socket != -1) {
		close(server_socket);
		remove(socket_path.toStdString().c_str());
	}
}

void OSServices::connectCommChannel(int timeout) {
	int result;

	if (server_socket == -1) {
		context.report(Error::COMM, "not_initialized", "The communication channel is not initialized");
	} else if (client_socket != -1) {
		context.report(Error::COMM, "already_initialized", "The communication channel is already connected");
	}

	for (int i = 0; i < timeout; i++) {
		// Sleep for 1 second
		sleep(1);

		// Process new events (if available)
		QCoreApplication::processEvents();

		client_socket = accept(server_socket, nullptr, nullptr);
		if (client_socket == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			context.report(Error::COMM, "connect", "Error while connecting to the observed process");
		} else if (client_socket != -1) {
			// Set flags of the socket
			long socket_flags = fcntl(client_socket, F_GETFL);
			if (socket_flags == -1) {
				close(client_socket);
				client_socket = -1;
				context.report(Error::COMM, "socket", "Cannot access socket properties");
			}
			socket_flags |= O_NONBLOCK;
			result = fcntl(client_socket, F_SETFL, socket_flags);
			if (result == -1) {
				close(client_socket);
				client_socket = -1;
				context.report(Error::COMM, "socket", "Cannot set socket properties");
			}

			break;
		}
	}

	if (client_socket == -1) {
		context.report(Error::COMM, "connect", "Cannot connect to the observed process");
	} else {
		// Setup notifier
		comm_notifier = new QSocketNotifier(client_socket, QSocketNotifier::Read);

		// Connect the notifier to the handler
		connect(comm_notifier, SIGNAL(activated(int)), this, SLOT(slot_msgReceived(int)));

		// Enable notifier
		comm_notifier->setEnabled(true);

		// Send acknowlegement to the observed process
		struct autopin_msg ack;
		ack.event_id = APP_READY;

		result = send(client_socket, &ack, sizeof(ack), 0);
		if (result == -1) context.report(Error::COMM, "connect", "Cannot connect to the observed process");
	}
}

void OSServices::sendMsg(int event_id, int arg, double val) {

	if (client_socket == -1) context.report(Error::COMM, "not_initialized", "The communication channel is not active");

	int result;
	struct autopin_msg msg;
	msg.event_id = event_id;
	msg.arg = arg;
	msg.val = val;

	result = send(client_socket, &msg, sizeof(msg), 0);
	if (result == -1) context.report(Error::COMM, "send", "Cannot send data to the observed process");
}

void OSServices::slot_msgReceived(int socket) {
	// The parameter socket is currently not used, so the next line is
	// for silencing the warning
	(void)socket;

	comm_notifier->setEnabled(false);

	struct autopin_msg msg;

	int result = recv(client_socket, &msg, sizeof(msg), 0);

	while (result > 0) {
		emit sig_CommChannel(msg);
		result = recv(client_socket, &msg, sizeof(msg), 0);
	}

	comm_notifier->setEnabled(true);
}

ProcessTree::autopin_tid_list OSServices::getProcessThreads(int pid) {
	QMutexLocker locker(&mutex);

	ProcessTree::autopin_tid_list result;
	QString procpath;
	QStringList threads;
	QDir procdir;

	bool ret;

	// set path
	procpath = "/proc/" + QString::number(pid) + "/task";

	procdir.setSorting(QDir::Name);
	procdir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

	// try to enter directory
	ret = procdir.cd(procpath);
	if (ret == false) {
		context.report(Error::SYSTEM, "get_threads", "Could not get threads of process " + QString::number(pid));
		return result;
	}

	// get a list of the threads in the directory
	threads = procdir.entryList();

	// convert the list of thread into a autopin_tid_list
	result = convertQStringList(threads);

	return result;
}

ProcessTree::autopin_tid_list OSServices::getChildProcesses(int pid) {
	QMutexLocker locker(&mutex);

	ProcessTree::autopin_tid_list result;
	QString procpath;
	QStringList procs;
	QDir procdir;
	bool ret = false;
	bool success = true;

	// set path
	procpath = "/proc";

	procdir.setSorting(QDir::Name);
	procdir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

	// try to enter proc directory
	ret = procdir.cd(procpath);
	if (ret == false) {
		context.report(Error::SYSTEM, "access_proc", "Could not access the proc filesystem");
		return result;
	}

	procs = procdir.entryList();

	for (int i = 0; i < procs.size(); i++) {

		QString tmp_pid = procs.at(i);

		if (!integer.exactMatch(tmp_pid)) continue;

		QString ppid = getProcEntry(tmp_pid.toInt(), 3, false);
		if (ppid == "") {
			success = false;
			break;
		} else if (ppid.toInt() == pid)
			result.insert(tmp_pid.toInt());
	}

	if (!success)
		context.report(Error::SYSTEM, "get_children", "Could not get all children of process " + QString::number(pid));

	return result;
}

int OSServices::getTaskSortId(int tid) {
	QMutexLocker locker(&mutex);

	QString str_result = getProcEntry(tid, 21);
	if (context.isError()) {
		return 0;
	}

	return str_result.toInt();
}

int OSServices::setAffinity(int tid, int cpu) {
	cpu_set_t cores;
	pid_t linux_tid = tid;

	// Setup CPU mask
	CPU_ZERO(&cores);
	CPU_SET(cpu, &cores);

	// set affinity
	return sched_setaffinity(linux_tid, sizeof(cores), &cores);
}

ProcessTree::autopin_tid_list OSServices::getPid(QString proc) {
	QMutexLocker locker(&mutex);

	ProcessTree::autopin_tid_list result;
	QStringList procs;
	QDir procdir;
	bool ret;

	// set filters for proc directory
	procdir.setSorting(QDir::Name);
	procdir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

	// try to enter proc directory
	ret = procdir.cd("/proc");
	if (ret == false) {
		context.report(Error::SYSTEM, "access_proc", "Could not access the proc filesystem");
		return result;
	}

	procs = procdir.entryList();

	for (int i = 0; i < procs.size(); i++) {

		QString tmp_pid = procs[i];

		if (!integer.exactMatch(tmp_pid)) continue;

		QString procname = getProcEntry(tmp_pid.toInt(), 1, true);

		if (procname == "")
			continue;
		else if (procname == proc)
			result.insert(tmp_pid.toInt());
	}

	return result;
}

QString OSServices::getCmd(int pid) {
	QMutexLocker locker(&mutex);
	QString result;

	QFile procfile("/proc/" + QString::number(pid) + "/cmdline");
	if (!procfile.open(QIODevice::ReadOnly)) return result;

	// The last byte of the file is 0 and is ignored
	char c;
	procfile.read(&c, 1);
	while (!procfile.atEnd()) {
		if (c != '\0')
			result += c;
		else
			result += ' ';

		procfile.read(&c, 1);
	}

	procfile.close();

	return result;
}

QString OSServices::getProcEntry(int tid, int index, bool error) {
	QString result = "";

	if (index < 0 || index > 43) {
		if (error)
			context.report(Error::SYSTEM, "access_proc", "Invalid index for stat file of: " + QString::number(index));
		return result;
	}

	QFile stat_file("/proc/" + QString::number(tid) + "/stat");

	if (!stat_file.open(QIODevice::ReadOnly)) {
		if (error)
			context.report(Error::SYSTEM, "access_proc",
						   "Could not get status information for process " + QString::number(tid));
		return result;
	}

	QTextStream stat_stream(&stat_file);
	QString proc_stat = stat_stream.readAll();

	stat_file.close();

	int start_pos = 0, end_pos = 0, current_index = 0;
	bool proc_name = false;

	// Determine the desired entry. Keep in mind that the process name may contain spaces

	while (end_pos < proc_stat.size()) {
		switch (proc_stat.toStdString().at(end_pos)) {
		case ' ':
			if (proc_name)
				end_pos++;
			else if (current_index == index) {
				result = proc_stat.mid(start_pos, end_pos - start_pos);
				return result;
			} else {
				end_pos++;
				start_pos = end_pos;
				current_index++;
			}
			break;

		case '(':
			proc_name = true;
			end_pos++;
			start_pos = end_pos;
			break;

		case ')':
			proc_name = false;
			if (current_index == index) {
				result = proc_stat.mid(start_pos, end_pos - start_pos);
				return result;
			}
			end_pos += 2;
			start_pos = end_pos;
			current_index++;
			break;

		default:
			end_pos++;
		}
	}

	if (index == current_index) {
		result = proc_stat.mid(start_pos, end_pos - start_pos - 1);
		return result;
	}

	if (error)
		context.report(Error::SYSTEM, "access_proc",
					   "Could not get status information for process " + QString::number(tid));

	return result;
}

void OSServices::usrSignalHandler(int param) {
	// The parameter param is currently not used, so the next line is
	// for silencing the warning
	(void)param;
	autopin_attached = true;
}

ProcessTree::autopin_tid_list OSServices::convertQStringList(QStringList &qlist) {
	ProcessTree::autopin_tid_list result;

	for (int i = 0; i < qlist.size(); i++) {
		int val = qlist.at(i).toInt();
		result.insert(val);
	}

	return result;
}

} // namespace OS
} // namespace AutopinPlus
