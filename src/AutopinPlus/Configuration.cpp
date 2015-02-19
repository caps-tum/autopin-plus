/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Configuration.h>

namespace AutopinPlus {

Configuration::Configuration(AutopinContext &context)
	: context(context), env(QProcessEnvironment::systemEnvironment()), name("Configuration") {}

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
