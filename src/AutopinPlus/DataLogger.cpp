/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/DataLogger.h>

namespace AutopinPlus {

DataLogger::DataLogger(const Configuration &config, const PerformanceMonitor::monitor_list &monitors,
					   AutopinContext &context)
	: config(config), monitors(monitors), context(context) {}

QString DataLogger::getName() { return name; }

} // namespace AutopinPlus
