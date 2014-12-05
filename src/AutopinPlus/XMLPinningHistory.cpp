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

#include <AutopinPlus/XMLPinningHistory.h>

#include <deque>
#include <iostream>
#include <QDate>
#include <QStringList>
#include <QTime>
#include <QXmlStreamReader>

namespace AutopinPlus {

XMLPinningHistory::XMLPinningHistory(const Configuration &config, AutopinContext &context)
	: PinningHistory(config, context) {}

void XMLPinningHistory::init() {
	context.info("Initializing xml pinning history");

	/*
	 * Get the file paths for the pinning history from the configuration.
	 * Configuration checks have already been performed in the factory
	 * function of Autopin.
	 */
	history_load_path = "";
	history_save_path = "";
	if (config.configOptionExists("PinningHistory.load"))
		history_load_path = config.getConfigOption("PinningHistory.load");
	if (config.configOptionExists("PinningHistory.save"))
		history_save_path = config.getConfigOption("PinningHistory.save");

	context.info("  :: Reading pinning history from " + history_load_path);

	// Read history file if requested by the user
	if (!history_load_path.isEmpty()) {
		QFile hfile;
		hfile.setFileName(history_load_path);

		// Try to open and parse the history file
		if (!hfile.open(QIODevice::ReadWrite))
			context.report(Error::FILE_NOT_FOUND, "file_open", "Could not open file " + history_load_path);

		// Try to parse the contents of the file
		QXmlStreamReader hreader(&hfile);
		hreader.setNamespaceProcessing(false);

		// Check if the file is valid
		while (!hreader.atEnd()) {
			hreader.readNextStartElement();
			if (hreader.name() != "XMLPinningHistory") {
				context.report(Error::HISTORY, "bad_file",
							   "File " + history_load_path + " is not a valid pinning history");
			} else
				break;
		}

		// Find start for config
		while (!hreader.atEnd()) {
			hreader.readNextStartElement();
			if (hreader.name() == "ControlStrategy") {
				strategy = hreader.attributes().value("type").toString();
				break;
			}
		}

		while (!hreader.atEnd()) {
			hreader.readNext();
			if (hreader.isStartElement()) {
				if (hreader.name() == "opt") {
					QString id = hreader.attributes().value("id").toString();
					QStringList elem;
					elem << hreader.readElementText();
					strategy_options.push_back(std::pair<QString, QStringList>(id, elem));
				}
			}
			if (hreader.isEndElement()) {
				if (hreader.name() == "ControlStrategy") break;
			}
		}

		// Find start node for pinnings
		while (!hreader.atEnd()) {
			hreader.readNextStartElement();
			if (hreader.name() == "Pinnings") break;
		}

		// Read pinnings
		int current_phase = -1;
		while (!hreader.atEnd()) {
			hreader.readNext();
			if (hreader.isStartElement()) {
				if (hreader.name() == "Phase")
					current_phase = hreader.attributes().value("id").toString().toInt();
				else if (hreader.name() == "Pinning") {
					QString sched = hreader.attributes().value("sched").toString();
					QStringList sched_list = sched.split(':');
					autopin_pinning pinning;
					double value = hreader.readElementText().toDouble();
					for (int i = 0; i < sched_list.size(); ++i) pinning.push_back(sched_list[i].toInt());
					addPinning(current_phase, pinning, value);
					// context.info("Added pinning: phase: "+QString::number(current_phase)+", pinning: "+sched+",
					// value: "+QString::number(value));
				}
			}
		}

		if (hreader.hasError()) {
			QString errLine, errCol, errMsg;
			errLine = QString::number(hreader.lineNumber());
			errCol = QString::number(hreader.columnNumber());
			errMsg = hreader.errorString();

			context.report(Error::HISTORY, "bad_syntax", "Error in file " + history_load_path + " (Line " + errLine +
															 ", Column " + errCol + "): " + errMsg);
		}

		hfile.close();
	}

	if (!history_save_path.isEmpty()) context.info("  :: Changes will be saved to " + history_save_path);

	if (!history_modified && !history_load_path.isEmpty())
		context.info("  :: No pinnings found in history file");
	else
		history_modified = false;
}

void XMLPinningHistory::deinit() {
	if (history_modified && !history_save_path.isEmpty()) {
		context.info(":: Storing xml pinning history to file " + history_save_path);

		QFile hfile(history_save_path);
		if (!hfile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
			context.report(Error::FILE_NOT_FOUND, "file_open",
						   "Could not save pinning history to file " + history_save_path);

		QXmlStreamWriter hwriter(&hfile);
		hwriter.setAutoFormatting(true);
		hwriter.writeStartDocument();
		hwriter.writeStartElement("XMLPinningHistory");

		hwriter.writeStartElement("Environment");
		hwriter.writeTextElement("Host", hostname);
		hwriter.writeTextElement("Date", QDate::currentDate().toString(Qt::ISODate));
		hwriter.writeTextElement("Time", QTime::currentTime().toString());
		hwriter.writeEndElement();

		hwriter.writeStartElement("Configuration");
		hwriter.writeAttribute("type", configuration);
		writeNodeChildren(hwriter, configuration_options);
		hwriter.writeEndElement();

		hwriter.writeStartElement("ObservedProcess");
		hwriter.writeTextElement("Command", cmd);
		hwriter.writeTextElement("Trace", trace);
		hwriter.writeTextElement("CommChan", comm);
		hwriter.writeTextElement("CommChanTimeout", comm_timeout);
		hwriter.writeEndElement();

		hwriter.writeStartElement("PerformanceMonitors");
		for (auto &elem : monitors) {
			hwriter.writeStartElement("Monitor");
			hwriter.writeAttribute("name", elem.name);
			hwriter.writeAttribute("type", elem.type);
			writeNodeChildren(hwriter, elem.opts);
			hwriter.writeEndElement();
		}
		hwriter.writeEndElement();

		hwriter.writeStartElement("ControlStrategy");
		hwriter.writeAttribute("type", strategy);
		writeNodeChildren(hwriter, strategy_options);
		hwriter.writeEndElement();

		hwriter.writeStartElement("Pinnings");
		writePinnings(hwriter);
		hwriter.writeEndElement();

		hwriter.writeEndElement();
		hwriter.writeEndDocument();
		hfile.close();

		history_modified = false;
	}
}

void XMLPinningHistory::writeNodeChildren(QXmlStreamWriter &xmlstream, Configuration::configopts opts) {
	for (auto &opt : opts) {
		xmlstream.writeStartElement("opt");
		xmlstream.writeAttribute("id", opt.first);
		xmlstream.writeCharacters(opt.second.join(" "));
		xmlstream.writeEndElement();
	}
}

void XMLPinningHistory::writePinnings(QXmlStreamWriter &xmlstream) {
	// Get available phases and sort them
	std::deque<int> phases;
	for (auto &elem : pinmap) phases.push_back(elem.first);
	std::sort(phases.begin(), phases.end());

	// Write pinnings to the xml file
	for (auto &phase : phases) {
		xmlstream.writeStartElement("Phase");
		xmlstream.writeAttribute("id", QString::number(phase));
		for (auto jt = pinmap[phase].begin(); jt != pinmap[phase].end(); jt++) {
			xmlstream.writeStartElement("Pinning");
			QString pinstr;
			for (auto kt = jt->first.begin(); kt != jt->first.end(); kt++) {
				if (kt != jt->first.begin()) pinstr += ":";
				pinstr += QString::number(*kt);
			}
			xmlstream.writeAttribute("sched", pinstr);
			xmlstream.writeCharacters(QString::number(jt->second));
			xmlstream.writeEndElement();
		}
		xmlstream.writeEndElement();
	}
}

} // namespace AutopinPlus
