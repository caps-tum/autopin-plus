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

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/Error.h>
#include <list>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace AutopinPlus {

/*!
 * \brief Abstract base class for the general handling of configuration files
 *
 * The actual parsing and access to configuration options has to be implemented in a subclass
 * of this class. However, any implementation of a subclass must respect that all configuration
 * options available in autopin+ must be handled in a single instance of this class.
 *
 * Additionaly to providing access to configuration files this class also provides functionality
 * for accessing environment variables.
 *
 */
class Configuration {
  public:
	/*!
	 * \brief Stores an option from the configuration
	 */
	typedef std::pair<QString, QStringList> configopt;

	/*!
	 * \brief Data type for storing a list of options
	 */
	typedef std::list<configopt> configopts;

  public:
	/*!
	 * \brief Constructor
	 *
	 * The configuration should not be read here but in the the method readConfiguration().
	 *
	 * \param[in] path	    Path to the configuration file
	 * \param[in] context	Refernce to the context of the object calling the constructor
	 */
	Configuration(const QString path, AutopinContext &context);

	virtual ~Configuration();

	/*!
	 * \brief Reads the configuration file and command line parameters
	 */
	virtual void init() = 0;

	/*!
	 * \brief Get the name of the configuration
	 *
	 * \return The name of the configuration
   */
	QString getName() const;

	/*!
	 * \brief Returns properties of the configuration
	 *
	 * \return A list with the properties of the configuration
	 */
	virtual configopts getConfigOpts() const = 0;

	/*!
	 * \brief Reads an option from the configuration
	 *
	 * \param[in] opt 	The name of the requested option
	 * \param[in] count	If opt contains more the one entries count
	 * 	specifies the entry which will be returned.
	 *
	 * \return	The value of the requested option. If no option with
	 * 	the specified name exits or count is to large an empty string
	 * 	will be returned
	 */
	virtual QString getConfigOption(QString opt, int count = 0) const;

	/*!
	 * \brief Returns all configuration options stored in opt
	 *
	 * \param[in] opt The name of the requested option
	 * \return A list of strings containing all configuration options
	 * 	stored in opt. If no such option exists the list will be empty.
	 */
	virtual QStringList getConfigOptionList(QString opt) const = 0;

	/**
	 * \brief Reads an integer option from the configuration
	 *
	 * \param[in] opt 	The name of the requested option
	 * \param[in] count	If opt contains more the one entries count
	 * 	specifies the entry which will be returned.
	 *
	 * \return	The value of the requested option as an integer.
	 * 	If the option does not exist or if it is not a valid
	 * 	integer 0 will be returned. If count is to large 0 will
	 * 	be returned, too.
	 */
	int getConfigOptionInt(QString opt, int count = 0) const;

	/**
	 * \brief Reads a double option from the configuration
	 *
	 * \param[in] opt 	The name of the requested option
	 * \param[in] count	If opt contains more the one entries count
	 * 	specifies the entry which will be returned.
	 *
	 * \return	The value of the requested option as a double.
	 * 	If the option does not exist or if it is not a valid
	 * 	integer 0 will be returned. If count is to large 0 will
	 * 	be returned, too.
	 */
	double getConfigOptionDouble(QString opt, int count = 0) const;

	/**
	 * \brief Reads a bool option from the configuration
	 *
	 * Configuration options of the Form "true", "false", "True",
	 * "False", "TRUE", "FALSE" and numbers are recognized and
	 * returned as a bool value. In order to make sure that the
	 * requested option is a valid bool value configOptionBool()
	 * should be called first.
	 *
	 * \param[in] opt 	The name of the requested option
	 * \param[in] count	If opt contains more the one entries count
	 * 	specifies the entry which will be returned.
	 *
	 * \return	The value of the requested option as a bool.
	 * 	If the option does not exist or if it is not a valid
	 * 	integer false will be returned. If count is to large false will
	 * 	be returned, too.
	 */
	virtual bool getConfigOptionBool(QString opt, int count = 0) const;

	/*!
	 * \brief Checks if a configuration option with the specified name exists
	 *
	 * \param[in] opt	The name of the requested option
	 *
	 * \return	The number of configuration options saved in the configuration
	 * 	entry opt. If there exists no such entry -1 is returned
	 */
	virtual int configOptionExists(QString opt) const;

	/*!
	 * \brief Checks if a config option is a valid bool value
	 *
	 * Configuration options of the Form "true", "false", "True",
	 * "False", "TRUE", "FALSE" and numbers are valid bool values.
	 *
	 * \param[in] opt 	The name of the requested option
	 * \param[in] count	If opt contains more the one entries count
	 * 	specifies the entry which will be examined.
	 * \return true if the specified option is a valid bool value and
	 * 	false in all other cases.
	 */
	virtual bool configOptionBool(QString opt, int count = 0) const;

	/*!
	 * \brief Accesses environment variables
	 *
	 * \param[in] name	Name of the requested variable
	 * \return	The Value stored in the requested variable. If
	 * 	no variable with the specified name exists an empty string
	 * 	will be returned.
	 */
	virtual QString getEnvVariable(QString name);

  protected:
	/*!
	 * Stores the path to the configuration file
	 */
	QString path;

	/*!
	 * A reference to the runtime context
	 */
	AutopinContext &context;

	/*!
	 * The process environment storing the environment variables
	 */
	QProcessEnvironment env;

	/*!
	 * The name of the configuration
	 */
	QString name;
};

} // namespace AutopinPlus
