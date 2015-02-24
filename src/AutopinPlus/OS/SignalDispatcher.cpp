/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/OS/SignalDispatcher.h>
#include <QCoreApplication>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>

namespace AutopinPlus {
namespace OS {

int SignalDispatcher::sigchldFd[2];

SignalDispatcher &SignalDispatcher::getInstance() {
	static SignalDispatcher instance;
	return instance;
}

SignalDispatcher::SignalDispatcher() : context(std::string("SignalDispatcher")){};

int SignalDispatcher::setupSignalHandler() {
	// Setup signal handler
	int ret = 0;
	struct sigaction chld;
	memset(&chld, 0, sizeof(struct sigaction));
	chld.sa_sigaction = chldSignalHandler;
	ret |= sigemptyset(&chld.sa_mask);
	chld.sa_flags = 0;
	chld.sa_flags |= SA_RESTART | SA_SIGINFO | SA_NOCLDSTOP;
	ret |= sigaction(SIGCHLD, &chld, nullptr);

	return getInstance().init();
}

int SignalDispatcher::init() {
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigchldFd)) return -1;

	snChld = new QSocketNotifier(sigchldFd[1], QSocketNotifier::Read, &getInstance());
	connect(snChld, SIGNAL(activated(int)), &getInstance(), SLOT(slot_handleSigChld()));

	return 0;
}

void SignalDispatcher::slot_handleSigChld() {
	snChld->setEnabled(false);

	siginfo_t info;
	if (read(sigchldFd[1], &info, sizeof(siginfo_t)) == -1) {
		context.error("Could not read from socketpair. Bailing out!");
		QCoreApplication::exit(1);
	}

	emit sig_ProcTerminated(info.si_pid, info.si_status);

	snChld->setEnabled(true);
}

void SignalDispatcher::chldSignalHandler(int, siginfo_t *, void *) {
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		siginfo_t info;
		info.si_pid = pid;
		info.si_status = status;
		if (write(sigchldFd[0], &info, sizeof(siginfo_t)) == -1) {
			SignalDispatcher::getInstance().context.error("Could not write to socketpair. Bailing out!");
			QCoreApplication::exit(1);
		}
	}
}

} // namespace OS
} // namespace AutopinPlus
