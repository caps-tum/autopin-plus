/*
 * This file is part of Autopin+.
 *
 * Copyright (C) 2014 Alexander Kurtz <alexander@kurtz.be>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <linux/perf_event.h> // for perf_event_attr
#include <qlist.h>			  // for QList
#include <qstring.h>		  // for QString

namespace AutopinPlus {
namespace Monitor {
namespace GPerf {

/*!
 * \brief A struct describing a perf sensor.
 */
struct Sensor {
	/*!
	 * A "perf_event_attr" which can be passed to the "perf_event_open()" syscall to open the sensor.
	 */
	perf_event_attr attr;

	/*!
	 * \brief The name of the sensor.
	 */
	QString name;

	/*!
	 * \brief The list of processors this sensor should monitor.
	 */
	QList<int> processors;

	/*!
	 * \brief The scaling factor which has to be applied to the raw values read from the file descriptor returned by the
	 * "perf_event_open()" syscall.
	 */
	double scale;

	/*!
	 * \brief The unit of the returned values.
	 */
	QString unit;
}; // struct Sensor

} // namespace GPerf
} // namespace Monitor
} // namespace AutopinPlus
