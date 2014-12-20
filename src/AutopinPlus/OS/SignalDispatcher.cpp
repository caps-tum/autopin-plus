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

#include <AutopinPlus/OS/SignalDispatcher.h>
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

SignalDispatcher::SignalDispatcher() {};

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
	read(sigchldFd[1], &info, sizeof(siginfo_t));
	waitpid(info.si_pid, nullptr, WNOHANG);

	emit sig_ProcTerminated(info.si_pid, info.si_status);

	snChld->setEnabled(true);
}

void SignalDispatcher::chldSignalHandler(int param, siginfo_t *info, void *paramv) {
	write(sigchldFd[0], info, sizeof(siginfo_t));
}

} // namespace OS
} // namespace AutopinPlus
