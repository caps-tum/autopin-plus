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

#include <AutopinPlus/PinningHistory.h>
#include <QFile>
#include <QXmlStreamWriter>

namespace AutopinPlus {

/*!
 * \brief Implementation of PinningHistory using XML
 *
 */
class XMLPinningHistory : public PinningHistory {
  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] context	Reference to the context of the object calling the constructor
	 *
	 */
	XMLPinningHistory(const Configuration &config, AutopinContext &context);

	void init() override;
	void deinit() override;

  private:
	/*!
	 * \brief Writes configuration options to a XML stream
	 *
	 * Processes a list of configuration options and writes it to an open
	 * XML stream.
	 *
	 * \param[in]	xmlstream	Reference to the QXmlStreamWriter object which is used for writing
	 * \param[in] opts		The configuration options which will be written
	 */
	void writeNodeChildren(QXmlStreamWriter &xmlstream, Configuration::configopts opts);

	/*!
	 * \brief Writes the pinnings to the xml file
	 *
	 * \param[in]	xmlstream	Reference to the QXmlStreamWriter object which is used for writing
	 */
	void writePinnings(QXmlStreamWriter &xmlstream);

  private:
	/*!
	 * File path from which pinning history is loaded
	 */
	QString history_load_path;

	/*!
	 * File path to which the pinning history is saved
	 */
	QString history_save_path;
};

} // namespace AutopinPlus
