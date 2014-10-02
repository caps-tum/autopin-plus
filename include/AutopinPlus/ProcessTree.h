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

#include <map>
#include <set>

namespace AutopinPlus {

/*!
 * \brief Represents dependences between processes and threads
 *
 * Every node of a ProcessTree represents a process and stores all threads of this
 * processes. The children of a node consequently represent the child processes of the
 * process stored in the parent node.
 */
class ProcessTree {
  public:
	/*!
	 * \brief Data structure for storing pids/tids
	 */
	typedef std::set<int> autopin_tid_list;

  private:
	/*!
	 * \brief Data structure for storing the children of a node in a ProcessTree
	 */
	typedef std::map<int, ProcessTree> proc_list;

  public:
	/*!
	 * \brief Represents the empty ProcessTree
	 */
	static ProcessTree empty;

  public:
	/*!
	 * \brief Constructor
	 */
	ProcessTree();

	/*!
	 * \brief Constructor
	 *
	 * Creates a node for a process.
	 *
	 * \param[in] pid	The pid of the process
	 */
	explicit ProcessTree(int pid);

	/*!
	 * \brief Constructor
	 *
	 * Creates a node for a process and adds the specified tasks to the node.
	 *
	 * \param[in] pid	The pid of the process
	 * \param[in] tasks	The tasks that will be added to the node
	 */
	ProcessTree(int pid, autopin_tid_list tasks);

	/*!
	 * \brief Adds a child process to the tree
	 *
	 * \param[in] ppid	The pid of the parent process
	 * \param[in] cpid	The pid of the child process
	 */
	void addChildProcess(int ppid, int cpid);

	/*!
	 * \brief Adds a task to a process in the tree
	 *
	 * \param[in] pid	The pid of the process
	 * \param[in] tid	The tid of the task
	 *
	 * \sa addProcessTaskList()
	 */
	void addProcessTask(int pid, int tid);

	/*!
	 * \brief Adds tasks to a process in the tree
	 *
	 * \param[in] pid	The pid of the process
	 * \param[in] tids	The tasks which will be added to the node
	 *
	 * \sa addProcessTask()
	 */
	void addProcessTaskList(int pid, autopin_tid_list &tids);

	/*!
	 * \brief Returns all tasks of a process
	 *
	 * \param[in] pid	The pid of the process
	 *
	 * \return A list with all tasks of the process. If there is no
	 * 	process with the specified pid the list will be empty.
	 */
	autopin_tid_list getTasks(int pid);

	/*!
	 * \brief Returns all taks in the process tree
	 *
	 * \returns A list with all tasks in the process tree
	 */
	autopin_tid_list getAllTasks();

	/*!
	 * \brief Searches for a task in the tree
	 *
	 * \return A reference to node containg the task. If no task
	 * 	with the specified tid can be found the empty tree will
	 * 	be returned.
	 */
	const ProcessTree &findTask(int tid);

	/*!
	 * \brief Searches for a process in the tree
	 *
	 * \return A reference to the node representing the process. If
	 * 	no process with the specified tid can be found the empty tree
	 * 	will be returned.
	 */
	const ProcessTree &findProcess(int pid);

	/*!
	 * \brief Implementation of the operator "<" for ProcessTree nodes
	 *
	 * This operator compares the nodes by applying "<" to the pid values
	 * stored in the root nodes.
	 *
	 * \param[in] comp	Another node
	 *
	 * \return The truth value of "this.pid < comp.pid"
	 */
	bool operator<(const ProcessTree &comp) const;

	/*!
	 * \brief Implementation of the operator "==" for ProcessTree nodes
	 *
	 * This operator compares the nodes by applying "==" to the pid values
	 * stored in the root nodes.
	 *
	 * \param[in] comp	Another node
	 *
	 * \return The truth value of "this.pid == comp.pid"
	 */
	bool operator==(const ProcessTree &comp) const;

	/*!
	 * \brief Implementation of the operator "!=" for ProcessTree nodes
	 *
	 * This operator compares the nodes by applying "!=" to the pid values
	 * stored in the root nodes.
	 *
	 * \param[in] comp	Another node
	 *
	 * \return The truth value of "this.pid != comp.pid"
	 */
	bool operator!=(const ProcessTree &comp) const;

  private:
	/*!
	 * Stores the pid of the process the node represents.
	 */
	int pid;

	/*!
	 * Stores the tasks of the process.
	 */
	autopin_tid_list tasks;

	/*!
	 * Stores the children of the node.
	 */
	proc_list child_procs;
};

} // namespace AutopinPlus
