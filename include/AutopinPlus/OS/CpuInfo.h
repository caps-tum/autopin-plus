/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
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
