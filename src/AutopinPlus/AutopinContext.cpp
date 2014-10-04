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

#include <AutopinPlus/AutopinContext.h>

namespace AutopinPlus {

AutopinContext::AutopinContext() {}

AutopinContext::AutopinContext(OutputChannel *outchan, Error *err, int layer) : outchan(outchan), err(err) {
	whitespace = "";
	for (int i = 0; i < layer; i++) whitespace += "  ";
}

AutopinContext::AutopinContext(const AutopinContext &context) {
	outchan = context.outchan;
	err = context.err;
	whitespace = context.whitespace + "  ";
}

void AutopinContext::info(QString msg) {
	if (indent) msg = whitespace + msg;
	outchan->info(msg);
}

void AutopinContext::biginfo(QString msg) {
	if (indent) msg = whitespace + msg;
	outchan->biginfo(msg);
}

void AutopinContext::warning(QString msg) {
	if (indent) msg = whitespace + msg;
	outchan->warning(msg);
}

void AutopinContext::error(QString msg) {
	if (indent) msg = whitespace + msg;
	outchan->error(msg);
}

void AutopinContext::debug(QString msg) {
	if (indent) msg = whitespace + msg;
	outchan->debug("DEBUG: " + msg);
}

autopin_estate AutopinContext::report(Error::autopin_errors error, QString opt, QString msg) const {
	autopin_estate result;
	result = err->report(error, opt);
	if (result == AUTOPIN_ERROR)
		outchan->error("ERROR: " + msg);
	else {
		if (indent)
			outchan->warning(whitespace + "WARNING: " + msg);
		else
			outchan->warning("WARNING: " + msg);
	}

	return result;
}

autopin_estate AutopinContext::autopinErrorState() { return err->autopinErrorState(); }

void AutopinContext::enableIndentation() { indent = true; }

void AutopinContext::disableIndentation() { indent = false; }

} // namespace AutopinPlus
