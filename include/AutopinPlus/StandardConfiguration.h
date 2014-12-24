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

#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/Error.h>
#include <list>
#include <map>
#include <memory>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <utility>
#include <QFile>

namespace AutopinPlus {

/*!
 * \brief Implementation of Configuration using a simple option=value configuration format
 */
class StandardConfiguration : public Configuration {
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] configText   QString, which contains the configuration
	 * \param[in] context      Reference to the context of the object calling the constructor
	 */
	StandardConfiguration(const QString configText, AutopinContext &context);

	void init() override;
	QStringList getConfigOptionList(QString opt) const override;

  private:
	/*!
	 * \brief Data structure for storing key-value-pairs of the configuration
	 */
	using string_map = std::map<QString, QStringList>;

	/*!
	 * \brief Data structure for storing a single key-value-pair
	 */
	using arg_pair = std::pair<QString, QStringList>;

	/*!
	 * Internal representation of the configuration
	 */
	string_map options;

	/*!
	 * \brief Parses a configuration file
	 *
	 * Reads a configuration file line by line and adds the options in the file
	 * to the configuration.
	 */
	void parseConfigurationFile();

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

	/*!
	 * \brief QString, which contains the configuration
	 */
	QString configText;
};

} // namespace AutopinPlus
