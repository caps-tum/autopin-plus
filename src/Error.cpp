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

#include "Error.h"

#include <QCoreApplication>
#include <QMutexLocker>

Error::Error() : global_estate(AUTOPIN_NOERROR) {}

autopin_estate Error::report(Error::autopin_errors error, QString opt) {
	QMutexLocker locker(&mutex);

	error_pair new_err;
	new_err.first = error;
	new_err.second = opt;
	errors.push_back(new_err);

	switch (error) {
	case FILE_NOT_FOUND:
		if (opt == "config") setError();

		break;
	case BAD_CONFIG:
		if (opt == "option_format")
			setError();
		else if (opt == "option_missing")
			setError();
		else if (opt == "inconsistent")
			setError();

		break;
	case PROCESS:
		if (opt == "already_running")
			break;
		else if (opt == "not_found")
			setError();
		else if (opt == "found_many")
			setError();
		else if (opt == "create")
			setError();
		else if (opt == "terminated")
			setError();

		break;
	case SYSTEM:
		if (opt == "already_initialized")
			break;
		else if (opt == "create_socket")
			setError();
		else if (opt == "sigset")
			setError();
		else if (opt == "get_threads")
			break;
		else if (opt == "access_proc")
			setError();
		else if (opt == "get_children")
			break;
		else if (opt == "set_affinity")
			break;
		else if (opt == "file_open")
			setError();

		break;
	case COMM:
		if (opt == "default_addr") setError();
		if (opt == "already_initialized") break;
		if (opt == "not_initialized") setError();
		if (opt == "socket") setError();
		if (opt == "comm_target") setError();
		if (opt == "connect") setError();
		if (opt == "send") setError();

		break;
	case PROC_TRACE:
		if (opt == "in_use")
			setError();
		else if (opt == "cannot_trace")
			break;
		else if (opt == "observed_process")
			break;
		else if (opt == "waitpid")
			break;
		else if (opt == "ptrace_eventmsg")
			setError();
		else if (opt == "cont_failed")
			setError();
		else if (opt == "set_options")
			break;

		break;
	case MONITOR:
		if (opt == "init")
			setError();
		else if (opt == "reset")
			break;
		else if (opt == "start")
			setError();
		else if (opt == "value")
			setError();
		else if (opt == "stop")
			setError();
		else if (opt == "create")
			setError();
		else if (opt == "destroy")
			setError();

		break;
	case STRATEGY:
		if (opt == "no_task") setError();

		break;
	case HISTORY:
		if (opt == "bad_syntax") setError();
		if (opt == "bad_file") setError();

		break;
	case UNSUPPORTED:
		if (opt == "critical")
			setError();
		else if (opt == "uncritical")
			break;

		break;
	case UNKNOWN:
		setError();
		break;
	case NONE:
		break;
	}

	return global_estate;
}

autopin_estate Error::autopinErrorState() {
	QMutexLocker locker(&mutex);

	return global_estate;
}

void Error::setError() {
	global_estate = AUTOPIN_ERROR;
	QCoreApplication::exit(-1);
}
