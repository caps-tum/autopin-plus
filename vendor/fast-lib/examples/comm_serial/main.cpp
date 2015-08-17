/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#include "communication/mqtt_communicator.hpp"
#include "serialization/serializable.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

// Inherit from fast::Serializable
struct Data : fast::Serializable
{
	std::string task;
	unsigned int id;
	std::vector<std::string> vms;

	// Override these two methods to state which members should be serialized
	YAML::Node emit() const override;
	void load(const YAML::Node &node) override;
};

int main(int argc, char *argv[])
{
	(void)argc;(void)argv;
	try {
		std::string id = "test-id";
		std::string subscribe_topic = "topic1";
		std::string publish_topic = "topic1";
		std::string host = "localhost";
		int port = 1883;
		int keepalive = 60;

		std::cout << "Starting communicator." << std::endl;
		fast::MQTT_communicator comm(id, subscribe_topic, publish_topic, host, port, keepalive);
		{
			Data d;
			d.task = "greet";
			d.id = 42;
			d.vms = {"vm-name-1", "vm-name-2"};

			std::cout << "Sending data message." << std::endl;
			comm.send_message(d.to_string());
		}
		{
			Data d;
			std::cout << "Waiting for message." << std::endl;
			d.from_string(comm.get_message());
			std::cout << "Message received." << std::endl;
			std::cout << d.to_string() << std::endl;
		}
	} catch (const std::exception &e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
	return EXIT_SUCCESS;
}

YAML::Node Data::emit() const
{
	YAML::Node node;
	node["task"] = task;
	node["data-id"] = id;
	node["vms"] = vms;
	return node;
}

void Data::load(const YAML::Node &node)
{
	fast::load(task, node["task"], "idle"); // "idle" is the default value if yaml does not contain the node "task"
	fast::load(id, node["data-id"]); // fast::load is like calling "id = node.as<decltype(id)>()"
	fast::load(vms, node["vms"]);
}
