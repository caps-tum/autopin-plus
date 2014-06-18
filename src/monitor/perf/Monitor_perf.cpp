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

#include "Monitor_perf.h"

#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

Monitor_perf::Monitor_perf(QString name, Configuration *config, const AutopinContext &context)
	: PerformanceMonitor(name, config, context) {
	valtype = PerformanceMonitor::MAX;
	type = "perf";
}

void Monitor_perf::init() {
	context.enableIndentation();
	context.info("  :: Initializing \"" + name + "\" (perf)");

	// Get the type of instructions to count
	if (config->configOptionExists(name + ".event_type") == 1) {
		QString event_name = config->getConfigOption(name + ".event_type");
		if (event_name == "PERF_COUNT_HW_CPU_CYCLES")
			event_type = PERF_COUNT_HW_CPU_CYCLES;
		else if (event_name == "PERF_COUNT_HW_INSTRUCTIONS")
			event_type = PERF_COUNT_HW_INSTRUCTIONS;
		else if (event_name == "PERF_COUNT_HW_CACHE_REFERENCES")
			event_type = PERF_COUNT_HW_CACHE_REFERENCES;
		else if (event_name == "PERF_COUNT_HW_CACHE_MISSES")
			event_type = PERF_COUNT_HW_CACHE_MISSES;
		else if (event_name == "PERF_COUNT_HW_BRANCH_INSTRUCTIONS")
			event_type = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
		else if (event_name == "PERF_COUNT_HW_BRANCH_MISSES")
			event_type = PERF_COUNT_HW_BRANCH_MISSES;
		else if (event_name == "PERF_COUNT_HW_BUS_CYCLES")
			event_type = PERF_COUNT_HW_BUS_CYCLES;
		else if (event_name == "PERF_COUNT_HW_STALLED_CYCLES_FRONTEND")
			event_type = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
		else if (event_name == "PERF_COUNT_HW_STALLED_CYCLES_BACKEND")
			event_type = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
		else
			REPORTV(Error::BAD_CONFIG, "option_format", "Event " + event_name + " is not supported by perf");

		context.info("     - Using event type " + event_name);
	} else if (config->configOptionExists(name + ".event_type") > 1) {
		REPORTV(Error::BAD_CONFIG, "inconsistent", "More than type specified for performance monitor " + name);
	} else
		REPORTV(Error::BAD_CONFIG, "option_missing", "No event type specified for performance monitor " + name);

	context.disableIndentation();
}

Configuration::configopts Monitor_perf::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(
		Configuration::configopt("event_type", QStringList(config->getConfigOption(name + ".event_type"))));

	return result;
}

void Monitor_perf::start(int tid) {
	if (perfds.find(tid) != perfds.end()) {
		int ret;
		int fd = perfds[tid];
		ret = ioctl(fd, PERF_EVENT_IOC_RESET);
		if (ret == -1) {
			context.debug("perf: Could not reset counter!");
			REPORTV(Error::MONITOR, "reset", "Could not reset perf for " + QString::number(tid));
		}

		ret = startPerfCounter(fd);
		if (ret == -1) {
			context.debug("perf: Could not start counter!");
			REPORTV(Error::MONITOR, "start", "Could not start perf for " + QString::number(tid));
		}
	} else {
		int newfd = createPerfCounter(tid);

		if (newfd == -1) {
			context.debug("perf: Could not create counter!");
			REPORTV(Error::MONITOR, "create", "Could not create perf counter for " + QString::number(tid));
		} else
			context.debug("perf: Created new counter!");

		perfds[tid] = newfd;

		int ret = startPerfCounter(newfd);
		if (ret == -1) {
			context.debug("perf: Could not start counter!");
			REPORTV(Error::MONITOR, "start", "Could not start perf for " + QString::number(tid));
		} else
			context.debug("perf: Started counter!");
	}
}

double Monitor_perf::value(int tid) {
	if (perfds.find(tid) == perfds.end()) return 0;

	__u64 val;
	int fd = perfds[tid];

	if (read(fd, &val, sizeof(__u64)) == -1)
		REPORT(Error::MONITOR, "value", "Could not read perf result for " + QString::number(tid), 0);

	double result = val;

	return result;
}

double Monitor_perf::stop(int tid) {
	if (perfds.find(tid) == perfds.end()) return 0;

	int fd = perfds[tid];

	double result;
	CHECK_ERROR(result = value(tid), 0);

	if (close(fd) == -1) REPORT(Error::MONITOR, "stop", "Could not stop perf for task " + QString::number(tid), 0);

	perfds.erase(tid);

	return result;
}

void Monitor_perf::clear(int tid) {
	if (perfds.find(tid) == perfds.end()) return;

	int fd = perfds[tid];
	close(fd);
	perfds.erase(fd);
}

ProcessTree::autopin_tid_list Monitor_perf::getMonitoredTasks() {
	ProcessTree::autopin_tid_list result;

	for (auto &elem : perfds) result.insert(elem.first);

	return result;
}

int Monitor_perf::createPerfCounter(int tid) {
	// Structure storing the preferences for perf
	struct perf_event_attr attr;
	memset(&attr, 0, sizeof(attr));

	pid_t pid = tid;
	// The counter is going to measure the performance of the
	// specified thread on all cores
	int cpu = -1;
	// only one counter per group will be used
	int group_fd = -1;
	// flags must be zero according to the perf documentation
	unsigned long flags = 0;

	// Setup the attr struct
	attr.type = PERF_TYPE_HARDWARE;
	attr.config = event_type;
	attr.disabled = 1;

	int fd = sys_perf_event_open(&attr, pid, cpu, group_fd, flags);

	if (fd == -EINVAL)
		return -1;
	else
		return fd;
}

int Monitor_perf::startPerfCounter(int fd) { return ioctl(fd, PERF_EVENT_IOC_ENABLE); }

int Monitor_perf::sys_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd,
									  unsigned long flags) {
	attr->size = sizeof(*attr);
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}
