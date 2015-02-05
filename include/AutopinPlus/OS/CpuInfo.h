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

#pragma once

#include <vector>
#include <QString>

namespace AutopinPlus {
namespace OS {
namespace CpuInfo {
	/*!
	 * Must be called before using the CpuInfo.
	 * Should be called usally once at the beginning of
	 * the lifetime of the application.
	 */
	void setupCpuInfo();

	/*!
	 * \brief Returns the number of available cpus.
	 *
	 * \return The number of cpus.
	 */
	int getCpuCount();

	/*!
	 * \brief Returns the distance between two cpus.
	 *
	 * \return The distance between cpu0 and cpu1.
	 */
	int getCpuDistance(int cpu0, int cpu1);

	/*!
	 * \brief Returns the index of the cpus on a nume node
	 *
	 * \param[in] node Specifies the node.
	 *
	 * \return A vector of indexes.
	 */
	std::vector<int> getCpusByNode(int node);

	/*!
	 * \brief Returns the number of numa nodes.
	 *
	 * \return The the number of numa nodes.
	 */
	int getNodeCount();

	/*!
	 * \brief Returns the node of a cpu.
	 *
	 * \param[in] cpu Specifies the cpu.
	 *
	 * \return Index of the node.
	 */
	int getNodeByCpu(int cpu);

	std::vector<int> parseSysRangeFile(QString path);

	std::vector<int> parseSysNodeDistance(QString path);
};

} // namespace OS
} // namespace AutopinPlus
