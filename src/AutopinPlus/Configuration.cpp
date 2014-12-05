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

#include <AutopinPlus/Configuration.h>

namespace AutopinPlus {

Configuration::Configuration(const QString path, AutopinContext &context)
	: path(path), context(context), env(QProcessEnvironment::systemEnvironment()), name("Configuration") {}

Configuration::~Configuration() {}

QString Configuration::getName() const { return name; }

QString Configuration::getConfigOption(QString opt, int count) const {
	QStringList opts = getConfigOptionList(opt);

	if (count >= 0 && count < opts.size())
		return opts[count];
	else
		return "";
}

int Configuration::configOptionExists(QString opt) const {
	QStringList opts = getConfigOptionList(opt);
	return opts.size();
}

int Configuration::getConfigOptionInt(QString opt, int count) const {
	QString val = getConfigOption(opt, count);
	return val.toInt();
}

double Configuration::getConfigOptionDouble(QString opt, int count) const {
	QString val = getConfigOption(opt, count);
	return val.toDouble();
}

bool Configuration::getConfigOptionBool(QString opt, int count) const {
	QString val = getConfigOption(opt, count);

	if (val == "true" || val == "True" || val == "TRUE")
		return true;
	else if (val == "false" || val == "False" || val == "FALSE")
		return false;
	else
		return (val.toDouble() != 0);
}

bool Configuration::configOptionBool(QString opt, int count) const {
	QString val = getConfigOption(opt, count);
	bool isdouble;
	val.toDouble(&isdouble);

	if (isdouble)
		return true;
	else if (val == "true" || val == "True" || val == "TRUE")
		return true;
	else if (val == "false" || val == "False" || val == "FALSE")
		return true;

	return false;
}

QString Configuration::getEnvVariable(QString name) { return env.value(name, ""); }

} // namespace AutopinPlus
