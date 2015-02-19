/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>		// for AutopinContext
#include <AutopinPlus/Configuration.h>		// for Configuration, etc
#include <AutopinPlus/PerformanceMonitor.h> // for PerformanceMonitor, etc
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
	DataLogger(const Configuration &config, const PerformanceMonitor::monitor_list &monitors, AutopinContext &context);

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
	virtual Configuration::configopts getConfigOpts() const = 0;

  protected:
	/*!
	 * \brief Name of the data logger.
	 */
	QString name;

	/*!
	 * \brief The Configuration class to use.
	 */
	const Configuration &config;

	/*!
	 * The list of performance monitors to use.
	 */
	const PerformanceMonitor::monitor_list &monitors;

	/*!
	 * The instance of the AutopinContext class to use.
	 */
	AutopinContext &context;
}; // class DataLogger

} // namespace AutopinPlus
