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

#include <AutopinPlus/StandardConfiguration.h>

#include <QFile>

namespace AutopinPlus {

StandardConfiguration::StandardConfiguration(int argc, char **argv, const AutopinContext &context)
	: Configuration(argc, argv, context), default_config_path(""), user_config_path("") {
	// Store command line arguments. Leave first argument out as it
	// corresponds to the command for starting the application.
	for (int i = 1; i < argc; i++) {
		arguments.push_back(argv[i]);
	}

	this->name = "StandardConfiguration";
}

void StandardConfiguration::init() {
	context.enableIndentation();

	// Load default configuration if available
	default_config_path = getEnvVariable("AUTOPIN_DEFAULT_CONFIG");
	if (default_config_path != "") {
		QFile default_config(default_config_path);
		if (default_config.open(QIODevice::ReadOnly)) {
			context.info("> Reading default configuration");
			QTextStream default_stream(&default_config);
			CHECK_ERRORV(parseConfigurationFile(default_stream));
		} else
			REPORTV(Error::FILE_NOT_FOUND, "config",
					"Could not read default configuration \"" + default_config_path + "\"");
	}

	// Load specified configuration if available
	for (int i = 0; i < arguments.size(); i++) {
		QString arg = arguments[i];
		if (arg == "-c") {
			user_config_path = arguments[++i];
			QFile user_config(user_config_path);
			if (user_config.open(QIODevice::ReadOnly)) {
				context.info("> Reading user configuration");
				QTextStream user_stream(&user_config);
				CHECK_ERRORV(parseConfigurationFile(user_stream));
			} else {
				REPORTV(Error::FILE_NOT_FOUND, "user_config",
						"Could not read user configuration \"" + user_config_path + "\"");
			}
			break;
		}
	}

	// Read other command line arguments if available
	context.info("> Reading command line options");
	for (int i = 0; i < arguments.size(); i++) {
		QString arg = arguments[i];

		if (arg == "-c") {
			i++;
			continue;
		}
		if (arg != "-c") {
			if (arg.mid(0, 2) != "--")
				REPORTV(Error::BAD_CONFIG, "option_format", "Invalid option format: \"" + arg + "\"");
			arg = arg.mid(2);
			CHECK_ERRORV(getArgs(arg));
			cmdline_options.push_back(arg);
		}
	}

	context.disableIndentation();
}

Configuration::configopts StandardConfiguration::getConfigOpts() {
	Configuration::configopts result;

	if (default_config_path != "")
		result.push_back(Configuration::configopt("default_config", QStringList(default_config_path)));
	if (user_config_path != "")
		result.push_back(Configuration::configopt("user_config", QStringList(user_config_path)));
	if (!cmdline_options.empty()) result.push_back(Configuration::configopt("cmdline_options", cmdline_options));

	return result;
}

QStringList StandardConfiguration::getConfigOptionList(QString opt) {
	QStringList result;

	if (options.find(opt) != options.end()) return options[opt];

	return result;
}

void StandardConfiguration::parseConfigurationFile(QTextStream &file) {
	QString line;

	while (!file.atEnd()) {
		line = file.readLine();

		if (line[0] == '#') continue;

		CHECK_ERRORV(getArgs(line));
	}
}

void StandardConfiguration::getArgs(QString arg) {
	QString argl, argr;
	QStringList optlist;
	int sep_pos = -1;
	arg_pair opt_pair;

	if ((sep_pos = arg.indexOf("+=")) != -1) {
		argl = arg.mid(0, sep_pos);
		argr = arg.mid(sep_pos + 2);

		argl = argl.trimmed();
		optlist = argr.split(' ', QString::SkipEmptyParts);

		opt_pair = arg_pair(argl, optlist);

		addOption(opt_pair);
	} else if ((sep_pos = arg.indexOf("-=")) != -1) {
		argl = arg.mid(0, sep_pos);
		argr = arg.mid(sep_pos + 2);

		argl = argl.trimmed();
		optlist = argr.split(' ', QString::SkipEmptyParts);

		opt_pair = arg_pair(argl, optlist);

		delOption(opt_pair);
	} else if ((sep_pos = arg.indexOf("=")) != -1) {
		argl = arg.mid(0, sep_pos);
		argr = arg.mid(sep_pos + 1);

		// remove white whitspace from the start and the end of the arguments
		argl = argl.trimmed();
		optlist = argr.split(' ', QString::SkipEmptyParts);

		opt_pair = arg_pair(argl, optlist);

		setOption(opt_pair);
	} else
		REPORTV(Error::BAD_CONFIG, "option_format", "Invalid option format: \"" + arg + "\"");
}

void StandardConfiguration::setOption(StandardConfiguration::arg_pair opt) {
	if (opt.second.isEmpty()) return;

	options[opt.first] = opt.second;
}

void StandardConfiguration::addOption(StandardConfiguration::arg_pair opt) {
	if (opt.second.isEmpty()) return;
	QStringList new_opts = options[opt.first];
	new_opts.append(opt.second);
	new_opts.removeDuplicates();
	options[opt.first] = new_opts;
}

void StandardConfiguration::delOption(StandardConfiguration::arg_pair opt) {
	if (opt.second.isEmpty()) return;

	if (options.find(opt.first) != options.end()) {
		QStringList new_opts = options[opt.first];
		for (int i = 0; i < opt.second.size(); i++) new_opts.removeAll(opt.second[i]);
		options[opt.first] = new_opts;
	}
}

} // namespace AutopinPlus
