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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
#include <qobjectdefs.h>					// for Q_OBJECT
#include <qobject.h>						// for QObject
#include <qstring.h>						// for QString

namespace AutopinPlus {

/*!
 * \brief Base class for data loggers.
 *
 * Data loggers have access to the configured performance monitors and can show or save their values at a fixed time or
 * periodically.
 */
class DataLogger : public QObject {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor.
	 * \param[in] config Pointer to the instance of the Configuration class to use.
	 * \param[in] monitors Reference to the list of performance monitors to use
	 * \param[in] context Reference to the instance of the AutopinContext class to use.
	 */
	DataLogger(Configuration *config, PerformanceMonitor::monitor_list &monitors, const AutopinContext &context);

	/*!
	 * \brief Initializes the data logger.
	 */
	virtual void init() = 0;

	/*!
	 * \brief Get the name of the data logger.
	 *
	 * \return The name of the data logger.
	 */
	QString getName();

	/*!
	 * \brief Returns the configuration options of the data logger.
	 *
	 * \return A list with the configuration options.
	 */
	virtual Configuration::configopts getConfigOpts() = 0;

  protected:
	/*!
	 * \brief Name of the data logger.
	 */
	QString name;

	/*!
	 * \brief The Configuration class to use.
	 */
	Configuration *config;

	/*!
	 * The list of performance monitors to use.
	 */
	PerformanceMonitor::monitor_list monitors;

	/*!
	 * The instance of the AutopinContext class to use.
	 */
	AutopinContext context;
}; // class DataLogger

} // namespace AutopinPlus
