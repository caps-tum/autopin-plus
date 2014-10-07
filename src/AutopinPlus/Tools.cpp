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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <AutopinPlus/Tools.h>

#include <AutopinPlus/Exception.h> // for Exception
#include <qatomic_x86_64.h>		   // for QBasicAtomicInt::deref
#include <qfile.h>				   // for QFile
#include <qglobal.h>			   // for qFree
#include <qiodevice.h>			   // for QIODevice, etc
#include <qlist.h>				   // for QList
#include <qstring.h>			   // for QString, operator+
#include <qstringlist.h>		   // for QStringList
#include <qtextstream.h>		   // for QTextStream, operator<<, etc
#include <utility>				   // for pair

namespace AutopinPlus {

double Tools::readDouble(const QString &string) {
	bool valid;

	auto result = string.toDouble(&valid);

	if (!valid) {
		throw Exception("Tools::readDouble(" + string + ") failed: Invalid.");
	}

	return result;
}

int Tools::readInt(const QString &string) {
	bool valid;

	auto result = string.toInt(&valid, 0);

	if (!valid) {
		throw Exception("Tools::readInt(" + string + ") failed: Invalid.");
	}

	return result;
}

QList<int> Tools::readInts(const QStringList &list) {
	QList<int> result;

	for (auto element : list) {
		result.append(readInt(element));
	}

	return result;
}

QString Tools::readLine(const QString &path) {
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly)) {
		throw Exception("Tools::readLine(" + path + ") failed: Couldn't open file.");
	}

	auto result = QTextStream(&file).readLine();

	// No need to close the file, the destructor will do that for us.

	return result;
}

std::pair<QString, QString> Tools::readPair(const QString &string, const QString &separator) {
	auto list = string.split(separator);

	if (list.size() != 2) {
		throw Exception("Tools::readPair(" + string + "," + separator + ") failed: Not exactly one " + separator + ".");
	}

	return std::pair<QString, QString>(list[0], list[1]);
}

unsigned long Tools::readULong(const QString &string) {
	bool valid;

	auto result = string.toULong(&valid, 0);

	if (!valid) {
		throw Exception("Tools::readULong(" + string + ") failed: Invalid.");
	}

	return result;
}

QStringList Tools::showInts(const QList<int> &list) {
	QStringList result;

	for (int element : list) {
		result.append(QString::number(element));
	}

	return result;
}

} // namespace AutopinPlus
