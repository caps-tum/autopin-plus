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

#pragma once

#include "Configuration.h"

#include <list>
#include <map>
#include <utility>
#include "Error.h"
#include <QTextStream>
#include <QStringList>

/*!
 * \brief Implementation of Configuration using a simple option=value configuration format
 */
class StandardConfiguration : public Configuration {
  public:
	/*!
	 * \brief Constructor
	 *
	 * The configuration should not be read here but in the the method readConfiguration().
	 *
	 * \param[in] argc     Number of command line arguments
	 * \param[in] argv     String array containing all commandline argumetns autopin+ has been started with
	 * \param[in] context  Refernce to the context of the object calling the constructor
	 */
	StandardConfiguration(int argc, char **argv, const AutopinContext &context);

	void init() override;
	Configuration::configopts getConfigOpts() override;
	QStringList getConfigOptionList(QString opt) override;

  private:
	/*!
	 * \brief Data structure for storing key-value-pairs of the configuration
	 */
	typedef std::map<QString, QStringList> string_map;

	/*!
	 * \brief Data structure for storing a single key-value-pair
	 */
	typedef std::pair<QString, QStringList> arg_pair;

	/*!
	 * Stores the command line parameters of the application
	 */
	QStringList arguments;

	/*!
	 * Path to the default configuration
	 */
	QString default_config_path;

	/*!
	 * Path to the user configuration
	 */
	QString user_config_path;

	/*!
	 * Parsed command line options
	 */
	QStringList cmdline_options;

	/*!
	 * Internal representation of the configuration
	 */
	string_map options;

	/*!
	 * \brief Parses a configuration file
	 *
	 * Reads a configuration file line by line and adds the options in the file
	 * to the configuration.
	 *
	 * \param[in] file Reference to a Textstream containing the configuration.
	 */
	void parseConfigurationFile(QTextStream &file);

	/*!
	 * \brief Simple method for parsing configuration options
	 *
	 * Takes a string of the form "var=val", "var+=val1 val2" or "var-=val1 val2" and
	 * sets var to val, adds val1, val2 to var or removes val1, val2 from var. Leading
	 * and trailing whitespace in arg and val will be removed.
	 *
	 * \param[in] arg String of the form "var=val", "var+=val1 val2" or "var-=val1 val2"
	 */
	void getArgs(QString arg);

	/*!
	 * \brief Sets an options corresponding to a value (corresponds to =)
	 *
	 * \param[in] opt   Pair of a configuration variable and options
	 */
	void setOption(arg_pair opt);

	/*!
	 * \brief Add values to an option (corresponds to +=)
	 *
	 * \param[in] opt   Pair of a configuration variable and options
	 */
	void addOption(arg_pair opt);

	/*!
	 * \brief Remove values to an option (corresponds to -=)
	 *
	 * \param[in] opt   Pair of a configuration variable and options
	 */
	void delOption(arg_pair opt);
};
