/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Logger/External/Process.h>

namespace AutopinPlus {
namespace Logger {
namespace External {

Process::~Process() {
	// Close all communication channels (stdin, stdout, stderr)...
	closeReadChannel(StandardOutput);
	closeReadChannel(StandardError);
	closeWriteChannel();

	// ... and then set the internal state to "NotRunning" without actually waiting for the process to terminate.
	setProcessState(ProcessState::NotRunning);
}

} // namespace External
} // namespace Logger
} // namespace AutopinPlus
