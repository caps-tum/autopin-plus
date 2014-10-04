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

#include <AutopinPlus/OutputChannel.h>

#include <iostream>
#include <QMutexLocker>

namespace AutopinPlus {

OutputChannel::OutputChannel() : outstream(stdout), debug_msg(false) {}

void OutputChannel::info(QString msg) {
	QMutexLocker locker(&mutex);

	if (outfile.isOpen())
		outfilestream << msg << endl;
	else
		outstream << msg << endl;
}

void OutputChannel::biginfo(QString msg) {
	QMutexLocker locker(&mutex);

	if (outfile.isOpen())
		outfilestream << msg << endl;
	else
		outstream << "\033[49;1m" << msg << "\033[0m" << endl;
}

void OutputChannel::error(QString msg) {
	QMutexLocker locker(&mutex);

	if (outfile.isOpen()) outfilestream << msg << endl;

	outstream << "\033[49;1;31m" << msg << "\033[0m" << endl;
}

void OutputChannel::warning(QString msg) {
	QMutexLocker locker(&mutex);

	if (outfile.isOpen())
		outfilestream << msg << endl;
	else
		outstream << "\033[49;1;33m" << msg << "\033[0m" << endl;
}

void OutputChannel::debug(QString msg) {
	QMutexLocker locker(&mutex);

	if (debug_msg) {
		if (outfile.isOpen())
			outfilestream << msg << endl;
		else
			outstream << msg << endl;
	}
}

void OutputChannel::enableDebug() {
	QMutexLocker locker(&mutex);

	debug_msg = true;
}

void OutputChannel::disableDebug() {
	QMutexLocker locker(&mutex);

	debug_msg = false;
}

bool OutputChannel::getDebug() { return debug_msg; }

bool OutputChannel::writeToFile(QString path) {
	if (outfile.isOpen()) return true;

	outfile.setFileName(path);

	if (!outfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;

	outfilestream.setDevice(&outfile);

	return true;
}

void OutputChannel::writeToConsole() {
	if (outfile.isOpen()) outfile.close();
}

} // namespace AutopinPlus
