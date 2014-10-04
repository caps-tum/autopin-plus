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

#ifndef LIBAUTOPIN_MSG_H
#define LIBAUTOPIN_MSG_H

// Messages from autopin+ to the application
#define APP_READY 0x0001
#define APP_INTERVAL 0x0010
// Messages from the application to autopin+
#define APP_NEW_PHASE 0x0100
#define APP_USER 0x1000

struct __attribute__((__packed__)) autopin_msg {
	unsigned long event_id;
	unsigned long arg;
	double val;
};

#endif // LIBAUTOPIN_MSG_H
