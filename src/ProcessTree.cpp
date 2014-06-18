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

#include "ProcessTree.h"

ProcessTree ProcessTree::empty(0);

ProcessTree::ProcessTree() {}

ProcessTree::ProcessTree(int pid) : pid(pid) { tasks.insert(pid); }

ProcessTree::ProcessTree(int pid, ProcessTree::autopin_tid_list tasks) : pid(pid), tasks(std::move(tasks)) {
	this->tasks.insert(pid);
}

void ProcessTree::addChildProcess(int ppid, int cpid) {
	if (pid == ppid) {
		ProcessTree new_child_process(cpid);
		child_procs[cpid] = new_child_process;
	} else {
		for (auto &elem : child_procs) elem.second.addChildProcess(ppid, cpid);
	}
}

void ProcessTree::addProcessTask(int pid, int tid) {
	if (this->pid == pid) {
		tasks.insert(tid);
	} else {
		for (auto &elem : child_procs) elem.second.addProcessTask(pid, tid);
	}
}

void ProcessTree::addProcessTaskList(int pid, ProcessTree::autopin_tid_list &tids) {
	if (this->pid == pid) {
		tasks.insert(tids.begin(), tids.end());
	} else {
		for (auto &elem : child_procs) elem.second.addProcessTaskList(pid, tids);
	}
}

ProcessTree::autopin_tid_list ProcessTree::getTasks(int pid) {
	ProcessTree::autopin_tid_list result;

	const ProcessTree &tasknode = findTask(pid);
	if (tasknode != ProcessTree::empty) result = tasknode.tasks;

	return result;
}

ProcessTree::autopin_tid_list ProcessTree::getAllTasks() {
	ProcessTree::autopin_tid_list result;

	result.insert(tasks.begin(), tasks.end());

	for (auto &elem : child_procs) {
		ProcessTree::autopin_tid_list tmp = elem.second.getAllTasks();
		result.insert(tmp.begin(), tmp.end());
	}

	return result;
}

const ProcessTree &ProcessTree::findTask(int tid) {

	if (tasks.find(tid) != tasks.end())
		return *this;
	else {
		for (auto &elem : child_procs) {
			if (elem.second.findTask(tid) != ProcessTree::empty) return elem.second.findTask(tid);
		}
	}

	return ProcessTree::empty;
}

const ProcessTree &ProcessTree::findProcess(int pid) {
	if (this->pid == pid)
		return *this;
	else {
		for (auto &elem : child_procs) {
			if (elem.second.findProcess(pid) != ProcessTree::empty) return elem.second.findProcess(pid);
		}
	}

	return ProcessTree::empty;
}

bool ProcessTree::operator<(const ProcessTree &comp) const {
	if (pid < comp.pid)
		return true;
	else
		return false;
}

bool ProcessTree::operator==(const ProcessTree &comp) const {
	if (pid == comp.pid) return true;

	return false;
}

bool ProcessTree::operator!=(const ProcessTree &comp) const { return !(*this == comp); }
