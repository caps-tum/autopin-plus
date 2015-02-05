/*
 * Autopin+ - Automatic thread-to-core-pinning tool
 * Copyright (C) 2014 LRR
 *
 * Author:
 * Lukas Fürmetz
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

#include <AutopinPlus/OS/CpuInfo.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <QFile>
#include <QTextStream>
#include <QStringList>

namespace AutopinPlus {
namespace OS {

/*!
 * Distances between numa nodes.
 */
static std::vector<std::vector<int>> distances;

/*!
 * Mapping between cpu and numa node.
 */
static std::vector<int> mapping;

void CpuInfo::setupCpuInfo() {
	for (int i = 0; i < get_nprocs(); i++) mapping.push_back(0);

	auto nodes = parseSysRangeFile("/sys/devices/system/node/online");
	for (int node : nodes) {
		// Mapping
		auto cpus = parseSysRangeFile("/sys/devices/system/node/node" + QString::number(node) + "/cpulist");
		for (int cpu : cpus) {
			mapping[cpu] = node;
		}

		// Distance
		auto distance = parseSysNodeDistance("/sys/devices/system/node/node" + QString::number(node) + "/distance");
		distances.push_back(distance);
	}
}

int CpuInfo::getCpuCount() { return get_nprocs(); }

int CpuInfo::getCpuDistance(int cpu1, int cpu2) {
	int node1 = mapping[cpu1];
	int node2 = mapping[cpu2];
	return distances[node1][node2];
}

std::vector<int> CpuInfo::parseSysRangeFile(QString path) {
	std::vector<int> result;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly)) {
		QTextStream stream(&file);
		auto text = stream.readAll();
		for (auto range : text.split(",")) {
			auto values = range.split("-");
			if (values.size() == 2) {
				int start = values.at(0).toInt();
				int stop = values.at(1).toInt();
				for (int value = start; value <= stop; value++) result.push_back(value);
			} else {
				result.push_back(values.at(0).toInt());
			}
		}
	}
	return result;
}

std::vector<int> CpuInfo::parseSysNodeDistance(QString path) {
	std::vector<int> result;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly)) {
		QTextStream stream(&file);
		auto text = stream.readAll();
		for (auto value : text.split(" ")) {
			result.push_back(value.toInt());
		}
	}
	return result;
}

std::vector<int> CpuInfo::getCpusByNode(int node) {
	std::vector<int> result;
	for (uint i = 0; i < mapping.size(); i++) {
		if (mapping[i] == node) result.push_back(i);
	}
	return result;
}

int CpuInfo::getNodeCount() { return distances.size(); }

int CpuInfo::getNodeByCpu(int cpu) { return mapping[cpu]; }

} // namespace OS
} // namespace AutopinPlus
