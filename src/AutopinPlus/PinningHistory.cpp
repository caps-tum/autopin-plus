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

#include <AutopinPlus/PinningHistory.h>

namespace AutopinPlus {

PinningHistory::PinningHistory(Configuration *config, const AutopinContext &context)
	: config(config), context(context), history_modified(false) {}

PinningHistory::~PinningHistory() {}

void PinningHistory::addPinning(int phase, PinningHistory::autopin_pinning pinning, double value) {
	pinning_result res;
	res.first = pinning;
	res.second = value;

	if (pinmap.find(phase) == pinmap.end()) {
		pinmap[phase].push_back(res);
		bestpinmap[phase] = res;
		history_modified = true;
	} else {
		for (auto &elem : pinmap[phase]) {
			if (elem.first == res.first) {
				if (elem.second != value) {
					elem.second = value;
					history_modified = true;
				}
				return;
			}
		}
		pinmap[phase].push_back(res);
		history_modified = true;

		if (bestpinmap[phase].second < value) bestpinmap[phase] = res;
	}
}

PinningHistory::pinning_result PinningHistory::getBestPinning(int phase) const {

	auto result = bestpinmap.find(phase);
	if (result != bestpinmap.end()) return result->second;

	REPORTV(Error::UNSUPPORTED, "uncritical",
			"Could not find best pinning for phase " + QString::number(phase) + ". Keeping original pinning.");

	return pinning_result();
}

std::list<PinningHistory::pinning_result> PinningHistory::getPinnings(int phase) const {

	auto result = pinmap.find(phase);

	if (result != pinmap.end()) return result->second;

	return std::list<pinning_result>();
}

const QString &PinningHistory::getStrategy() const { return strategy; }

const Configuration::configopts &PinningHistory::getStrategyOptions() const { return strategy_options; }

Configuration::configopts::const_iterator PinningHistory::getStrategyOption(QString s) const {
	return std::find_if(getStrategyOptions().begin(), getStrategyOptions().end(),
						[&](const std::pair<QString, QStringList> &p) {
		if (p.first == s) return true;
		return false;
	});
}

void PinningHistory::setHostname(QString hostname) { this->hostname = hostname; }

void PinningHistory::setConfiguration(QString type, Configuration::configopts opts) {
	configuration = type;
	configuration_options = opts;
}

void PinningHistory::setObservedProcess(QString cmd, QString trace, QString comm, QString comm_timeout) {
	this->cmd = cmd;
	this->trace = trace;
	this->comm = comm;
	this->comm_timeout = comm_timeout;
}

void PinningHistory::addPerformanceMonitor(PinningHistory::monitor_config mon) { monitors.push_back(mon); }

void PinningHistory::setControlStrategy(QString type, Configuration::configopts opts) {
	strategy = type;
	strategy_options = opts;
}

} // namespace AutopinPlus
