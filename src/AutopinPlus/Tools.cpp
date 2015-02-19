/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Tools.h>

#include <AutopinPlus/Exception.h> // for Exception
#include <qfile.h>				   // for QFile
#include <qlist.h>				   // for QList
#include <qstring.h>			   // for QString, operator+
#include <qstringlist.h>		   // for QStringList
#include <qtextstream.h>		   // for QTextStream, operator<<, etc
#include <utility>				   // for pair

namespace AutopinPlus {
namespace Tools {

double readDouble(const QString &string) {
	bool valid;

	auto result = string.toDouble(&valid);

	if (!valid) {
		throw Exception("Tools::readDouble(" + string + ") failed: Invalid.");
	}

	return result;
}

int readInt(const QString &string) {
	bool valid;

	auto result = string.toInt(&valid, 0);

	if (!valid) {
		throw Exception("Tools::readInt(" + string + ") failed: Invalid.");
	}

	return result;
}

QList<int> readInts(const QStringList &list) {
	QList<int> result;

	for (auto element : list) {
		result.append(readInt(element));
	}

	return result;
}

QString readLine(const QString &path) {
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly)) {
		throw Exception("Tools::readLine(" + path + ") failed: Couldn't open file.");
	}

	auto result = QTextStream(&file).readLine();

	// No need to close the file, the destructor will do that for us.

	return result;
}

std::pair<QString, QString> readPair(const QString &string, const QString &separator) {
	auto list = string.split(separator);

	if (list.size() != 2) {
		throw Exception("Tools::readPair(" + string + "," + separator + ") failed: Not exactly one " + separator + ".");
	}

	return std::pair<QString, QString>(list[0], list[1]);
}

unsigned long readULong(const QString &string) {
	bool valid;

	auto result = string.toULong(&valid, 0);

	if (!valid) {
		throw Exception("Tools::readULong(" + string + ") failed: Invalid.");
	}

	return result;
}

QStringList showInts(const QList<int> &list) {
	QStringList result;

	for (int element : list) {
		result.append(QString::number(element));
	}

	return result;
}

} // namespace Tools
} // namespace AutopinPlus
