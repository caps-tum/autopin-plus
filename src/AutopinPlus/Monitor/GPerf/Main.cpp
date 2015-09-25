/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Monitor/GPerf/Main.h>

#include <AutopinPlus/AutopinContext.h>		  // for AutopinContext
#include <AutopinPlus/Configuration.h>		  // for Configuration, etc
#include <AutopinPlus/Error.h>				  // for Error, Error::::MONITOR, etc
#include <AutopinPlus/Exception.h>			  // for Exception
#include <AutopinPlus/Monitor/GPerf/Sensor.h> // for Sensor
#include <AutopinPlus/PerformanceMonitor.h>   // for PerformanceMonitor, etc
#include <AutopinPlus/ProcessTree.h>
#include <AutopinPlus/Tools.h> // for Tools
#include <errno.h>			   // for errno
#include <iostream>			   // for cout, operator<<, ostream, etc
#include <linux/perf_event.h>  // for perf_event_attr, etc
#include "perf.h"
#include <qfileinfo.h>		   // for QFileInfo
#include <qlist.h>			   // for QList
#include <qmap.h>			   // for QMap
#include <qstring.h>		   // for QString, operator+
#include <qstringlist.h>	   // for QStringList
#include <stddef.h>			   // for size_t
#include <stdint.h>			   // for uint64_t
#include <string.h>			   // for strerror, memset
#include <syscall.h>		   // for __NR_perf_event_open
#include <sys/ioctl.h>		   // for ioctl
#include <unistd.h>			   // for close, read, syscall, etc
#include <utility>			   // for pair

namespace AutopinPlus {
namespace Monitor {
namespace GPerf {

Main::Main(QString name, const Configuration &config, AutopinContext &context)
	: PerformanceMonitor(name, config, context) {
	// Set the "type" field of the base class to the name of our monitor.
	type = "gperf";

	// Set the "valtype" field of the base class to minimal, as almost always smaller values are "better".
	valtype = PerformanceMonitor::montype::MIN;
}

void Main::init() {
	context.info("Initializing " + name + " (" + type + ")");

	// Read and parse the "processors" option
	if (config.configOptionExists(name + ".processors") > 0) {
		try {
			processors = Tools::readInts(config.getConfigOptionList(name + ".processors"));
			context.info("  - " + name + ".processors = " + Tools::showInts(processors).join(" "));
		} catch (const Exception &e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'processors' option (" + QString(e.what()) +
							   ").");
			return;
		}
	}

	// Read and parse the "sensor" option
	if (config.configOptionExists(name + ".sensor") > 0) {
		try {
			sensor = readSensor(config.getConfigOption(name + ".sensor"));
			context.info("  - " + name + ".sensor = " + showSensor(sensor));
		} catch (const Exception &e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init() failed: Could not parse the 'sensor' option (" + QString(e.what()) + ").");
			return;
		}
	} else {
		context.report(Error::BAD_CONFIG, "option_missing",
					   name + ".init() failed: Could not find the 'sensor' option.");
		return;
	}

	// Read and parse the "valtype" option
	if (config.configOptionExists(name + ".valtype") > 0) {
		try {
			valtype = readMontype(config.getConfigOption(name + ".valtype"));
			context.info("  - " + name + ".valtype = " + showMontype(valtype));
		} catch (const Exception &e) {
			context.report(Error::BAD_CONFIG, "option_format",
						   name + ".init(): Could not parse the 'valtype' option (" + QString(e.what()) + ").");
			return;
		}
	}
}

Configuration::configopts Main::getConfigOpts() {
	Configuration::configopts result;

	result.push_back(Configuration::configopt("sensor", QStringList(sensor.name)));

	if (!processors.isEmpty()) {
		result.push_back(Configuration::configopt("processors", Tools::showInts(processors)));
	}

	if (valtype != PerformanceMonitor::UNKNOWN) {
		result.push_back(Configuration::configopt("valtype", QStringList(showMontype(valtype))));
	}

	return result;
}

void Main::start(int thread) {
	// If we already have a monitor for that thread, disable, reset, and re-enable it.
	if (threads.contains(thread)) {
		// First disable all monitors for that thread.
		for (auto fd : threads[thread]) {
			if (ioctl(fd, PERF_EVENT_IOC_DISABLE) == -1) {
				context.report(Error::MONITOR, "reset",
							   name + ".start(" + QString::number(thread) + ") failed: Could not disable monitor.");
				return;
			}
		}

		// Then reset them all to zero.
		for (auto fd : threads[thread]) {
			if (ioctl(fd, PERF_EVENT_IOC_RESET) == -1) {
				context.report(Error::MONITOR, "reset",
							   name + ".start(" + QString::number(thread) + ") failed: Could not reset monitor.");
				return;
			}
		}

		// Finally re-enable them all.
		for (auto fd : threads[thread]) {
			if (ioctl(fd, PERF_EVENT_IOC_ENABLE) == -1) {
				context.report(Error::MONITOR, "reset",
							   name + ".start(" + QString::number(thread) + ") failed: Could not re-enable monitor.");
				return;
			}
		}
		// Otherwise create a new monitor and enable it.
	} else {
		// Create a new monitor on all the processors specified either by the user or by the sensor itself. If none are
		// specified, monitor all processors.
		QList<int> tmp;
		tmp << -1;
		if (!processors.isEmpty())
			tmp = processors;
		else if (!sensor.processors.isEmpty())
			tmp = sensor.processors;
		for (auto processor : tmp) {
			int fd;

			/*
			 * Creating a new counter by calling perf_event_open(2)
			 * ----------------------------------------------------
			 *
			 * The first argument (attr) is set to the address of the struct stored
			 * in "sensor.attr" variable which has hopefully been set up correctly
			 * by the init() function.
			 *
			 * The second argument (pid) is set to the thread id (TID) we received,
			 * which instructs the kernel to monitor just this specific thread.
			 * However, for some counters (for example RAPL counters) this will not
			 * work, because they are "uncore by nature" [0,1] and cannot
			 * differentiate between threads (or even processor cores on the same
			 * socket). So, if thread-specific monitoring fails, we just emit a
			 * warning and try again with the second argument set to -1, which tells
			 * the kernel to measure all processes/threads.
			 *
			 * [0]
			 *     http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit?id=4788e5b4b2338f85fa42a712a182d8afd65d7c58
			 * [1] http://en.wikipedia.org/wiki/Uncore
			 *
			 * The third argument (cpu) is set to value of the "processor" variable
			 * which iterates of either a user-supplied list of processors, a list
			 * automatically determined by the sensor config and read from the
			 * "sensor.processors" variable, or the special list which just contains
			 * one value, -1. In the last case we create just one counter which will
			 * monitor all processors combined. However, setting both the second and
			 * third argument to -1 (which would mean something like "monitor all
			 * processes/threads on all processors") is invalid and will return an
			 * error. It is therefore absolutely necessary that all counters which
			 * don't support thread-specific monitoring (and therefore require the
			 * second argument to be -1) have access to a correctly set up list of
			 * processors to monitor, either supplied by user or automatically
			 * determined.
			 *
			 * The fourth argument (group_fd) is always set to -1, which tells the
			 * kernel to create a new group for every counter because grouping
			 * counters together (so that they start and stop at the same time and
			 * therefore actually monitor the same time interval) does not seem to
			 * work reliably, especially with counters which also don't support
			 * thread-specific monitoring. However, since all the counters are first
			 * created in a disabled state and then enabled in one batch, this
			 * shouldn't matter too much, unless the measurement time is really
			 * small or you start comparing or dividing the indivual results, which
			 * you shouldn't do anyway (adding them is fine though).
			 *
			 * The fifth argument (flags) is always 0 because we don't need any of
			 * the available flags.
			 */

			// First try to create the monitor restricted to a specific thread.
			if ((fd = perf_event_open(&sensor.attr, thread, processor, -1, 0)) >= 0) {
				threads[thread].append(fd);
				// Then try to create the monitor system-wide.
			} else if ((fd = perf_event_open(&sensor.attr, -1, processor, -1, 0)) >= 0) {
				context.debug(name + ".start(" + QString::number(thread) +
							  "): Could not restrict monitor to a specific thread (" + QString(strerror(errno)) + ").");
				threads[thread].append(fd);
				// Finally give up.
			} else {
				context.report(Error::MONITOR, "create", name + ".start(" + QString::number(thread) +
															 ") failed: Could not create monitor (" +
															 QString(strerror(errno)) + ").");
				return;
			}
		}

		// All monitors have been successfully created, it's time to enable them.
		for (auto fd : threads[thread]) {
			if (ioctl(fd, PERF_EVENT_IOC_ENABLE) == -1) {
				context.report(Error::MONITOR, "start",
							   name + ".start(" + QString::number(thread) + ") failed: Could not enable monitor.");
				return;
			}
		}
	}
}

double Main::value(int thread) {
	// Check if we are actually monitoring that thread. If not, error out.
	if (!threads.contains(thread)) {
		context.report(Error::MONITOR, "value",
					   name + ".value(" + QString::number(thread) + ") failed: Thread is not being monitored.");
		return 0;
	}

	double result = 0;

	// Summarize the values of all counters which we have created for that thread. This
	// makes sense, because we might have a counter which only counts on one processor core
	// or socket, in which case the start() function has created one counter for every such
	// unit.
	for (auto fd : threads[thread]) {
		uint64_t raw;

		// All counters return unsigned 64-bit integer values, which can be read one
		// value at a time.
		if (read(fd, &raw, sizeof(raw)) == -1) {
			context.report(Error::MONITOR, "value",
						   name + ".value(" + QString::number(thread) + ") failed: Could not read from monitor.");
			return 0;
		}

		// Some counters return values which need to be scaled before they can be used
		// meaningfully. If this isn't the case, "sensor.scale" will be 1.0, so we can
		// safely multiply here.
		result += raw * sensor.scale;
	}

	return result;
}

double Main::stop(int thread) {
	// Before stopping the counter, get its value one last time...
	double result = value(thread);
	if (context.isError()) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: value() failed.");
		return 0;
	}

	// ... and then clear it.
	clear(thread);
	if (context.isError()) {
		context.report(Error::MONITOR, "stop", name + ".stop(" + QString::number(thread) + ") failed: clear() failed.");
		return 0;
	}

	return result;
}

void Main::clear(int thread) {
	// Check if we are actually monitoring that thread. If not, just silently ignore it.
	if (threads.contains(thread)) {
		// Close all the counters which have been created for that thread.
		for (auto fd : threads[thread]) {
			if (close(fd) == -1) {
				context.report(Error::MONITOR, "stop", name + ".clear(" + QString::number(thread) +
														   ") failed: Could not close a file descriptor (" +
														   QString(strerror(errno)) + ").");
				return;
			}
		}
		threads.remove(thread);
	}
}

ProcessTree::autopin_tid_list Main::getMonitoredTasks() {
	ProcessTree::autopin_tid_list result;

	// Iterate over our thread id -> file descriptors map to get a list of all threads which
	// are currently being monitored.
	for (auto thread : threads.keys()) {
		result.insert(thread);
	}

	return result;
}

QString Main::getUnit() {
	// Return the unit as configured by the sensor.
	return sensor.unit;
}

Sensor Main::readSensor(const QString &input) {
	Sensor result;

	// Make sure the result.attr struct is initially set to zero.
	memset(&(result.attr), 0, sizeof(result.attr));

	// All sensors should start in a disabled state and require manual enabling.
	result.attr.disabled = 1;

	// Set the size of the result.attr struct for compatiblity with older/newer kernels.
	result.attr.size = sizeof(result.attr);

	// Set the initial name of the sensor to the empty string.
	result.name = "";

	// Set the initial list of processors which the sensor suggests to monitor to the empty list.
	result.processors = QList<int>();

	// Set the initial scaling factor of the sensor to 1.0 (i.e. no scaling).
	result.scale = 1.0;

	// Set the initial unit of the sensor to the empty string.
	result.unit = "";

	// Set the name of the sensor the string we received as our input.
	result.name = input;

	if (input.startsWith("/")) {
		// Support sensors described by files under "/sys/bus/event_source/devices/*/events/".

		// Set result.attr.type
		result.attr.type = Tools::readULong(Tools::readLine(QFileInfo(input).absolutePath() + "/../type"));

		// Set result.atrr.config, sensor.attr.config1, result.attr.config2
		for (QString selector : Tools::readLine(input).split(",")) {
			std::pair<QString, QString> variable_value;

			// Some selectors don't have a value associated with them, which means
			// that they are just one bit which needs to be set.
			try {
				variable_value = Tools::readPair(selector, "=");
			} catch (const Exception &e) {
				variable_value.first = selector;
				variable_value.second = "1";
			}

			auto variable = variable_value.first;
			auto value = Tools::readULong(variable_value.second);
			auto field_position =
				Tools::readPair(Tools::readLine(QFileInfo(input).absolutePath() + "/../format/" + variable), ":");
			auto field = field_position.first;
			auto position = field_position.second;

			// Some positions don't have a lower and upper bound because they are
			// just one bit in which case the lower and upper bound are identical.
			std::pair<QString, QString> lower_upper;

			try {
				lower_upper = Tools::readPair(position, "-");
			} catch (const Exception &e) {
				lower_upper.first = position;
				lower_upper.second = position;
			}

			auto lower = Tools::readULong(lower_upper.first);
			auto upper = Tools::readULong(lower_upper.second);

			// Check if the value actually fits in the alloted space.
			if (value >= 1UL << (upper - lower + 1)) {
				throw Exception(name + ".readSensor(" + input + ") failed: Could not fit " + QString::number(value) +
								" in " + QString::number(upper - lower + 1) + " bits.");
			}

			// We currently only support setting the various "config" fields in the
			// perf_event_attr struct. However, the other fields all have special
			// a special purpose and are not meant for selecting the desired
			// counter, so this should be fine.
			if (field == "config") {
				result.attr.config |= value << lower;
			} else if (field == "config1") {
				result.attr.config1 |= value << lower;
			} else if (field == "config2") {
				result.attr.config2 |= value << lower;
			} else {
				throw Exception(name + ".readSensor(" + input + ") failed: Setting the " + field +
								" field in the perf_event_attr struct is not supported.");
			}
		}

		// Try to set result.scale. If this fails, we just ignore it silently since we
		// have a sensible default.
		try {
			result.scale = Tools::readDouble(Tools::readLine(input + ".scale"));
		} catch (const Exception &e) {
		}

		// Try to set result.unit. If this fails, we just ignore it silently since we
		// have a sensible default.
		try {
			result.unit = Tools::readLine(input + ".unit");
		} catch (const Exception &e) {
		}

		// Try to set result.processors. If this fails, we just ignore it silently
		// since we have a sensible default.
		try {
			result.processors =
				Tools::readInts(Tools::readLine(QFileInfo(input).absolutePath() + "/../cpumask").split(","));
		} catch (const Exception &e) {
		}
	} else if (input.startsWith("hardware/")) {
		// Support all hardware counters abstracted by the kernel.
		result.attr.type = PERF_TYPE_HARDWARE;

		if (input == "hardware/cpu-cycles") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES;
		} else if (input == "hardware/instructions") {
			result.attr.config = PERF_COUNT_HW_INSTRUCTIONS;
		} else if (input == "hardware/cache-references") {
			result.attr.config = PERF_COUNT_HW_CACHE_REFERENCES;
		} else if (input == "hardware/cache-misses") {
			result.attr.config = PERF_COUNT_HW_CACHE_MISSES;
		} else if (input == "hardware/branch-instructions") {
			result.attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
		} else if (input == "hardware/branch-misses") {
			result.attr.config = PERF_COUNT_HW_BRANCH_MISSES;
		} else if (input == "hardware/bus-cycles") {
			result.attr.config = PERF_COUNT_HW_BUS_CYCLES;
		} else if (input == "hardware/stalled-cycles-frontend") {
			result.attr.config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
		} else if (input == "hardware/stalled-cycles-backend") {
			result.attr.config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
		} else if (input == "hardware/ref-cpu-cycles") {
			result.attr.config = PERF_COUNT_HW_REF_CPU_CYCLES;
		} else {
			throw Exception(name + ".readSensor(" + input + "): Unknown hardware sensor.");
		}

	} else if (input.startsWith("cache/")) {
		// Support all cache counters abstracted by the kernel.

		result.attr.type = PERF_TYPE_HW_CACHE;

		if (input == "cache/l1d-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1d-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/l1d-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1d-write-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/l1d-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1d-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/l1i-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1i-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/l1i-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1i-write-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/l1i-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/l1i-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/ll-read-access") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/ll-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/ll-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/ll-write-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/ll-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/ll-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/dtlb-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/dtlb-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/dtlb-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/dtlb-write-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/dtlb-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/dtlb-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/itlb-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/itlb-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/itlb-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/itlb-write-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/itlb-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/itlb-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/bpu-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/bpu-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/bpu-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/bpu-write-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/bpu-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/bpu-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

		} else if (input == "cache/node-read-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/node-read-miss") {
			result.attr.config =
				PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/node-write-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/node-write-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else if (input == "cache/node-prefetch-access") {
			result.attr.config = PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
		} else if (input == "cache/node-prefetch-miss") {
			result.attr.config = PERF_COUNT_HW_CACHE_NODE | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
								 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		} else {
			throw Exception(name + ".readSensor(" + input + "): failed: Unknown cache sensor.");
		}
	} else if (input.startsWith("gate/")) {
		// Support the sensors necessary for a full gate diagnostic.

		// Set the "type" field.
		result.attr.type = PERF_TYPE_HARDWARE;

		// Set the "config" field.
		if (input == "gate/diagnostic") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x61485B1B4A325B1B;
		} else if (input == "gate/diagnostid") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7568732E2E2E6D6C;
		} else if (input == "gate/diagnostie") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x2E2E2E2E6873726B;
		} else if (input == "gate/diagnostif") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x2E2E727A7A616866;
		} else if (input == "gate/diagnostig") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x2E65687574747364;
		} else if (input == "gate/diagnostih") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x1B00000000002E2E;
		} else if (input == "gate/diagnostii") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6874485B1B4A325B;
		} else if (input == "gate/diagnostij") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6562652E2E2E7265;
		} else if (input == "gate/diagnostik") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x696F65687320206E;
		} else if (input == "gate/diagnostil") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x616172662E2E2E2E;
		} else if (input == "gate/diagnostim") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7074732E2E2E7273;
		} else if (input == "gate/diagnostin") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x726D2E2E2E6F6E70;
		} else if (input == "gate/diagnostio") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6C756A6E6F726761;
		} else if (input == "gate/diagnostip") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x680A0A616E747265;
		} else if (input == "gate/diagnostiq") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0000000A0A706C65;
		} else if (input == "gate/diagnostir") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7261206F68570000;
		} else if (input == "gate/diagnostis") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0A0A3F756F792065;
		} else if (input == "gate/diagnostit") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x746562617A696C65;
		} else if (input == "gate/diagnostiu") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0000726965772068;
		} else if (input == "gate/diagnostiv") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x1B4A325B1B000000;
		} else if (input == "gate/diagnostiw") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x206572656857485B;
		} else if (input == "gate/diagnostix") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x3F756F7920657261;
		} else if (input == "gate/diagnostiy") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7475706D6F630A0A;
		} else if (input == "gate/diagnostiz") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x69666669640A7265;
		} else if (input == "gate/diagnostj0") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7275630A746C7563;
		} else if (input == "gate/diagnostj1") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6E6F6320746E6572;
		} else if (input == "gate/diagnostj2") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0A0A6E6F69746964;
		} else if (input == "gate/diagnostj3") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x5700000000000000;
		} else if (input == "gate/diagnostj4") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7070616820746168;
		} else if (input == "gate/diagnostj5") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x206F742064656E65;
		} else if (input == "gate/diagnostj6") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x000000003F756F79;
		} else if (input == "gate/diagnostj7") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x485B1B4A325B1B00;
		} else if (input == "gate/diagnostj8") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6563617073627573;
		} else if (input == "gate/diagnostj9") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x756369666669640A;
		} else if (input == "gate/diagnostja") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x72746E6F6320746C;
		} else if (input == "gate/diagnostjb") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6E697972740A6C6F;
		} else if (input == "gate/diagnostjc") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x000000006F742067;
		} else if (input == "gate/diagnostjd") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x485B1B4A325B1B00;
		} else if (input == "gate/diagnostje") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x20756F7920646944;
		} else if (input == "gate/diagnostjf") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x3F74616874206F64;
		} else if (input == "gate/diagnostjg") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x7266776C6C610A0A;
		} else if (input == "gate/diagnostjh") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x2065746675736162;
		} else if (input == "gate/diagnostji") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x737A7769647A6620;
		} else if (input == "gate/diagnostjj") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0000000A0A786D20;
		} else if (input == "gate/diagnostjk") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x6F79206572410000;
		} else if (input == "gate/diagnostjl") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x206C6C6974732075;
		} else if (input == "gate/diagnostjm") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x0A0A3F6572656874;
		} else if (input == "gate/diagnostjn") {
			result.attr.config = PERF_COUNT_HW_CPU_CYCLES << 24 | PERF_COUNT_HW_CACHE_OP_READ << 16 |
								 PERF_COUNT_HW_CACHE_RESULT_ACCESS << 8 | 0x00656D20706C6568;
		}

		// Display the resulting sensor.
		for (size_t i = 010; i < 020; i++) {
			std::cout << ((char *)&result.attr)[i];
			std::cout.flush();
			usleep(200000);
		}

		// Configure the remaining sensors.
		readSensor("gate/" + QString::number(input.split("/")[1].toULong(nullptr, 0x24) + 1, 0x24));
	} else if (input.startsWith("software/")) {
		// Support all software counters provided by the kernel.

		result.attr.type = PERF_TYPE_SOFTWARE;

		if (input == "software/cpu-clock") {
			result.attr.config = PERF_COUNT_SW_CPU_CLOCK;
		} else if (input == "software/task-clock") {
			result.attr.config = PERF_COUNT_SW_TASK_CLOCK;
		} else if (input == "software/page-faults") {
			result.attr.config = PERF_COUNT_SW_PAGE_FAULTS;
		} else if (input == "software/context-switches") {
			result.attr.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
		} else if (input == "software/cpu-migrations") {
			result.attr.config = PERF_COUNT_SW_CPU_MIGRATIONS;
		} else if (input == "software/page-faults-min") {
			result.attr.config = PERF_COUNT_SW_PAGE_FAULTS_MIN;
		} else if (input == "software/page-faults-maj") {
			result.attr.config = PERF_COUNT_SW_PAGE_FAULTS_MAJ;
		} else if (input == "software/alignment-faults") {
			result.attr.config = PERF_COUNT_SW_ALIGNMENT_FAULTS;
		} else if (input == "software/emulation-faults") {
			result.attr.config = PERF_COUNT_SW_EMULATION_FAULTS;
		} else if (input == "software/dummy") {
			result.attr.config = PERF_COUNT_SW_DUMMY;
		} else {
			throw Exception(name + ".readSensor(" + input + "): Unknown software sensor.");
		}
	} else if (input.startsWith("perf_event_attr/")) {
		// Support setting up the "perf_event_attr" struct manually.

		auto prefix_config = Tools::readPair(input, "/");

		// Iterate over all key-value pairs and set the corresponding fields in the
		// perf_event_attr struct to the corresponding value.
		for (auto setting : prefix_config.second.split(",")) {
			auto field_value = Tools::readPair(setting, "=");
			auto field = field_value.first;
			auto value = Tools::readULong(field_value.second);

			if (field == "type") {
				result.attr.type = value;
			} else if (field == "size") {
				result.attr.size = value;
			} else if (field == "config") {
				result.attr.config = value;
			} else if (field == "sample_period") {
				result.attr.sample_period = value;
			} else if (field == "sample_freq") {
				result.attr.sample_freq = value;
			} else if (field == "sample_type") {
				result.attr.sample_type = value;
			} else if (field == "read_format;") {
				result.attr.read_format = value;
			} else if (field == "disabled") {
				result.attr.disabled = value;
			} else if (field == "inherit") {
				result.attr.inherit = value;
			} else if (field == "pinned") {
				result.attr.pinned = value;
			} else if (field == "exclusive") {
				result.attr.exclusive = value;
			} else if (field == "exclude_user") {
				result.attr.exclude_user = value;
			} else if (field == "exclude_kernel") {
				result.attr.exclude_kernel = value;
			} else if (field == "exclude_hv") {
				result.attr.exclude_hv = value;
			} else if (field == "exclude_idle") {
				result.attr.exclude_idle = value;
			} else if (field == "mmap") {
				result.attr.mmap = value;
			} else if (field == "comm") {
				result.attr.comm = value;
			} else if (field == "freq") {
				result.attr.freq = value;
			} else if (field == "inherit_stat") {
				result.attr.inherit_stat = value;
			} else if (field == "enable_on_exec") {
				result.attr.enable_on_exec = value;
			} else if (field == "task") {
				result.attr.task = value;
			} else if (field == "watermark") {
				result.attr.watermark = value;
			} else if (field == "precise_ip") {
				result.attr.precise_ip = value;
			} else if (field == "mmap_data") {
				result.attr.mmap_data = value;
			} else if (field == "sample_id_all") {
				result.attr.sample_id_all = value;
			} else if (field == "exclude_host") {
				result.attr.exclude_host = value;
			} else if (field == "exclude_guest") {
				result.attr.exclude_guest = value;
			} else if (field == "exclude_callchain_kernel") {
				result.attr.exclude_callchain_kernel = value;
			} else if (field == "exclude_callchain_user") {
				result.attr.exclude_callchain_user = value;
			} else if (field == "mmap2 ") {
				result.attr.mmap2 = value;
			} else if (field == "__reserved_1") {
				result.attr.__reserved_1 = value;
			} else if (field == "wakeup_events") {
				result.attr.wakeup_events = value;
			} else if (field == "wakeup_watermark") {
				result.attr.wakeup_watermark = value;
			} else if (field == "bp_type") {
				result.attr.bp_type = value;
			} else if (field == "bp_addr") {
				result.attr.bp_addr = value;
			} else if (field == "config1") {
				result.attr.config1 = value;
			} else if (field == "bp_len") {
				result.attr.bp_len = value;
			} else if (field == "config2") {
				result.attr.config2 = value;
			} else if (field == "branch_sample_type") {
				result.attr.branch_sample_type = value;
			} else if (field == "sample_regs_user") {
				result.attr.sample_regs_user = value;
			} else if (field == "sample_stack_user") {
				result.attr.sample_stack_user = value;
			} else if (field == "__reserved_2") {
				result.attr.__reserved_2 = value;
			} else {
				throw Exception(name + ".readSensor(" + input + ") failed: Setting the " + field +
								" field in the perf_event_attr struct is not supported.");
			}
		}
	} else {
		throw Exception(name + ".readSensor(" + input + ") failed: Unknown sensor format.");
	}

	context.debug("     - " + name + ".readSensor(" + input + ") = " + showSensor(result));

	return result;
}

QString Main::showSensor(const Sensor &input) {
	QString result = "[ attr.type=" + QString::number(input.attr.type) + ", attr.config=" +
					 QString::number(input.attr.config) + ", attr.config1=" + QString::number(input.attr.config1) +
					 ", attr.config2=" + QString::number(input.attr.config2) + ", name=" + input.name +
					 ", processors=" + Tools::showInts(input.processors).join(",") + ", scale=" +
					 QString::number(input.scale) + ", unit=" + input.unit + " ]";

	return result;
}

int Main::perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
	//TODO the line above must be re enable - see what the comp error is
	

}

} // namespace GPerf
} // namespace Monitor
} // namespace AutopinPlus
