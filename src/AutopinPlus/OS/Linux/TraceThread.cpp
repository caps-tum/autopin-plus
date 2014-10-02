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

#include <AutopinPlus/OS/Linux/TraceThread.h>

#include <AutopinPlus/OS/Linux/OSServicesLinux.h>
#include <errno.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

namespace AutopinPlus {
namespace OS {
namespace Linux {

TraceThread::TraceThread(const AutopinContext &context)
	: context(context), pid(-1), ptrace_opt(PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE),
	  exreq(false) {}

QMutex *TraceThread::init(ObservedProcess *observed_process) {
	CHECK_ERROR(pid = observed_process->getPid(), &trace_mutex);
	pid_str.setNum(pid);
	this->observed_process = observed_process;
	trace_mutex.lock();
	start();
	return &trace_mutex;
}

void TraceThread::deinit() {
	exreq = true;
	wait();
}

void TraceThread::attach() {
	bool tasks_changed = true;
	ProcessTree::autopin_tid_list no_trace;
	int errsave, status;

	context.debug("TraceThread is attaching");

	// attach to all tasks and child processes of the observed_process

	while (tasks_changed) {
		ProcessTree::autopin_tid_list attach_tasks = observed_process->getProcessTree().getAllTasks();
		tasks_changed = false;

		for (const auto &attach_task : attach_tasks) {
			if (tasks.find(attach_task) != tasks.end())
				continue;
			else if (no_trace.find(attach_task) != no_trace.end())
				continue;
			else
				tasks_changed = true;

			long ret;
			pid_t pid = attach_task;

			ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);

			if (ret == -1) {
				errsave = errno;

				if (errsave == ESRCH || errsave == EPERM) {
					no_trace.insert(pid);
					REPORTV(Error::PROC_TRACE, "cannot_trace", "Not going to trace " + QString::number(pid));
				} else {
					REPORTV(Error::UNKNOWN, "PTRACE_ATTACH",
							"Could not attach to process with pid " + QString::number(pid));
				}
				continue;
			}

			// wait for the process to stop
			waitpid(pid, &status, __WALL);
			if (WIFEXITED(status)) {
				REPORTV(Error::PROC_TRACE, "cannot_trace", "Task " + QString::number(pid) + " has already exited");
				continue;
			}

			// set ptrace options
			ret = ptrace(PTRACE_SETOPTIONS, pid, NULL, ptrace_opt);
			if (ret == -1) {
				errsave = errno;

				if (errsave == ESRCH || errsave == EPERM) {
					no_trace.insert(pid);
					REPORTV(Error::PROC_TRACE, "cannot_trace", "Not going to trace " + QString::number(pid));
				} else {
					REPORTV(Error::UNKNOWN, "PTRACE_SETOPTIONS",
							"Could set ptrace options for process " + QString::number(pid));
				}

				ptrace(PTRACE_DETACH, pid, NULL, NULL);

				continue;
			} else {
				context.info("  :: Attached to task " + QString::number(pid));
				tasks.insert(pid);
			}
		}
	}

	// continue all tasks
	for (const auto &elem : tasks) ptraceContinue(elem);

	// continue the process if it has been started by autopin+
	if (observed_process->getExec()) kill(pid, SIGUSR1);
}

void TraceThread::run() {
	int status;
	long ret;
	pid_t trace_pid;
	unsigned long event_msg;
	context.enableIndentation();
	setupSignalHandler();

	// attach to processes
	CHECK_ERRORVA(attach(), trace_mutex.unlock());

	// Check if the observed process is traced
	if (tasks.find(pid) == tasks.end())
		REPORTVA(Error::PROC_TRACE, "observed_process", "Cannot trace the observed_process", trace_mutex.unlock());

	context.info("> Attached to all tasks of process " + QString::number(pid));
	trace_mutex.unlock();
	context.disableIndentation();

	// start the loop for tracking process events
	while (true) {
		context.debug("Trace Thread is waiting");
		// set alarm
		alarm(3);
		trace_pid = waitpid(-1, &status, __WALL);
		alarm(0);

		if (alarm_occured) {
			context.debug("ALARM!");
			alarm_occured = false;

			if (exreq)
				break;
			else if (trace_pid == -1)
				continue;
		}

		if (trace_pid == -1) {
			REPORTV(Error::PROC_TRACE, "waitpid", "waitpid failed while tracing the observed process");
			continue;
		}

		context.debug("Got signal from: " + QString::number(trace_pid));

		// check return status of waitpid

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			tasks.erase(trace_pid);

			emit sig_TaskTerminated(trace_pid);

			if (pid == trace_pid) return;
		} else if (WIFSTOPPED(status)) {

			if (WSTOPSIG(status) == SIGTRAP) {
				ret = ptrace(PTRACE_GETEVENTMSG, trace_pid, NULL, (void *)&event_msg);

				if (ret != 0) REPORTV(Error::PROC_TRACE, "ptrace_eventmsg", "Could not get ptrace event information");

				switch (status >> 16) {
				case(PTRACE_EVENT_CLONE) :

				case(PTRACE_EVENT_FORK) :

				case(PTRACE_EVENT_VFORK) :
					newTask(event_msg);
					break;

				default:
					context.debug("Received unhandled ptrace event!");
					break;
				}

				CHECK_ERRORV(ptraceContinue(trace_pid));

			} else if (WSTOPSIG(status) == SIGSTOP) {
				context.debug("Received stop signal!");

				if (tasks.find(trace_pid) != tasks.end()) {
					CHECK_ERRORV(ptraceContinue(trace_pid));
				} else
					newTask(trace_pid);

			} else {
				context.debug("Received unhandled signal!");
				CHECK_ERRORV(ptraceContinue(trace_pid, WSTOPSIG(status)));
			}

		} else {
			context.debug("Unknown ptrace signal");
		}
	}
}

void TraceThread::ptraceContinue(pid_t pid, unsigned long sig) {
	long ret;
	ret = ptrace(PTRACE_CONT, pid, NULL, (void *)sig);

	if (ret == -1) REPORTV(Error::PROC_TRACE, "cont_failed", "Could not continue task " + QString::number(pid));
}

void TraceThread::newTask(int pid) {
	unsigned long ret;
	if (new_tasks.find(pid) != new_tasks.end()) {
		emit sig_TaskCreated(pid);

		ret = ptrace(PTRACE_SETOPTIONS, pid, NULL, ptrace_opt);

		if (ret != 0)
			context.report(Error::PROC_TRACE, "set_options",
						   "Could not set ptrace options for process " + QString::number(pid));

		new_tasks.erase(pid);
        tasks.insert(pid);

		CHECK_ERRORV(ptraceContinue(pid));

	} else
		new_tasks.insert(pid);
}

void TraceThread::setupSignalHandler() {
	int ret = 0;
	sigset_t sigset;

	// Unmask SIGALRM
	ret |= sigemptyset(&sigset);
	ret |= sigaddset(&sigset, SIGALRM);
	ret |= pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);

	if (ret != 0) REPORTV(Error::SYSTEM, "sigset", "Could not setup signal mask for SIGALRM");

	// Set signal handler

	struct sigaction sigact;
	memset(&sigact, 0, sizeof(struct sigaction));

	sigact.sa_handler = TraceThread::alrmSignalHandler;

	sigemptyset(&sigact.sa_mask);

	sigact.sa_flags = 0;

	ret = sigaction(SIGALRM, &sigact, nullptr);

	if (ret != 0) REPORTV(Error::SYSTEM, "sigset", "Could not setup signal handler for SIGALRM");
}

bool TraceThread::alarm_occured = false;

void TraceThread::alrmSignalHandler(int param) { alarm_occured = true; }

} // namespace Linux
} // namespace OS
} // namespace AutopinPlus
