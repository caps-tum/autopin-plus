/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#include "communication/mqtt_communicator.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <thread>

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
		fast::MQTT_communicator comm(id, subscribe_topic, publish_topic, host, port, keepalive, std::chrono::seconds(5));
		std::cout << "Communicator started." << std::endl << std::endl;
		
		// Sending and receiving
		{
			// Expected: success
			std::cout << "Sending message." << std::endl;
			comm.send_message("Hallo Welt");
			std::cout << "Waiting for message." << std::endl;
			std::string msg = comm.get_message();
			std::cout << "Message received: " << msg << std::endl;
			if (msg != "Hallo Welt")
				return EXIT_FAILURE;
		}
		std::cout << std::endl;
		// Receiving with timeout
		{
			try {
				// Expected: success
				std::cout << std::endl << "Sending message." << std::endl;
				comm.send_message("Hallo Welt");
				std::cout << "Waiting for message. (3s timeout)" << std::endl;
				std::string msg = comm.get_message(std::chrono::seconds(3));
				std::cout << "Message received: " << msg << std::endl << std::endl;
				if (msg != "Hallo Welt")
					return EXIT_FAILURE;

				// Expected: timeout
				std::cout << std::endl << "No message this time." << std::endl;
				std::cout << "Waiting for message. (3s timeout)" << std::endl;
				msg = comm.get_message(std::chrono::seconds(3));
				return EXIT_FAILURE; // Should not be reached due to timeout exception.
			} catch (const std::runtime_error &e) {
				if (e.what() != std::string("Timeout while waiting for message."))
					return EXIT_FAILURE;
			}
		}
		std::cout << std::endl;
		// Add and remove subscription
		{
			// Expected: success
			std::cout << "Add subscription on \"topic2\"." << std::endl;
			comm.add_subscription("topic2");
			std::cout << "Sending message on \"topic2\"." << std::endl;
			comm.send_message("Hallo Welt", "topic2");
			std::cout << "Waiting for message on \"topic2\"." << std::endl;
			std::string msg = comm.get_message("topic2");
			std::cout << "Message received on \"topic2\": " << msg << std::endl << std::endl;
			if (msg != "Hallo Welt")
				return EXIT_FAILURE;

			// Expected: missing subscription
			std::cout << "Remove subscription on \"topic2\"." << std::endl;
			comm.remove_subscription("topic2");
			try {
				std::cout << "Sending message on \"topic2\"." << std::endl;
				comm.send_message("Hallo Welt", "topic2");
				std::cout << "Waiting for message on \"topic2\"." << std::endl;
				std::string msg = comm.get_message("topic2", std::chrono::seconds(1));
				return EXIT_FAILURE; // Should not be reached due to missing subscription.
			} catch (const std::out_of_range &e) {
				if (e.what() != std::string("Topic not found in subscriptions."))
					return EXIT_FAILURE;
			}
		}
		std::cout << std::endl;
		// Add subscription with callback
		{
			// Expected: success
			std::string msg;
			std::cout << "Add subscription on \"topic3\" with callback." << std::endl;
			comm.add_subscription("topic3", [&msg](std::string received_msg) {
					std::cout << "Received in callback: " << received_msg << std::endl;
					msg = std::move(received_msg);
			});
			std::cout << "Sending message on \"topic3\"." << std::endl;
			comm.send_message("Hallo Welt", "topic3");
			std::this_thread::sleep_for(std::chrono::seconds(1));
			if (msg != "Hallo Welt")
				return EXIT_FAILURE;
			std::cout << "Message received on \"topic3\": " << msg << std::endl << std::endl;

			// Expected: cannot get_message from subscription with callback
			try {
				std::cout << "Try to get message from topic with callback." << std::endl;
				comm.get_message("topic3");
				return EXIT_FAILURE;
			} catch (const std::runtime_error &e) {
				if (e.what() != std::string("Error in get_message: This topic is subscribed with callback."))
					return EXIT_FAILURE;
			}
			std::cout << "Remove subscription on \"topic2\"" << std::endl;
			comm.remove_subscription("topic3");
		}
		std::cout << std::endl;
		// Wildcard +
		{
			// Expected: success
			std::cout << "Add subscription on \"A/+/B\"." << std::endl;
			comm.add_subscription("A/+/B");
			std::cout << "Sending message on \"A/C/B\"." << std::endl;
			comm.send_message("Hallo Welt", "A/C/B");
			std::cout << "Waiting for message on \"A/+/B\"." << std::endl;
			auto msg = comm.get_message("A/+/B", std::chrono::seconds(3));
			std::cout << "Message received on \"A/+/B\"." << std::endl;
			if (msg != "Hallo Welt")
				return EXIT_FAILURE;
			comm.remove_subscription("A/+/B");
		}
		std::cout << std::endl;
		// Wildcard #
		{
			// Expected: success
			std::cout << "Add subscription on \"A/#\"." << std::endl;
			comm.add_subscription("A/#");
			std::cout << "Sending message on \"A/C/B\"." << std::endl;
			comm.send_message("Hallo Welt", "A/C/B");
			std::cout << "Waiting for message on \"A/#\"." << std::endl;
			auto msg = comm.get_message("A/#", std::chrono::seconds(3));
			std::cout << "Message received on \"A/#\"." << std::endl;
			if (msg != "Hallo Welt")
				return EXIT_FAILURE;
			comm.remove_subscription("A/#");
		}
		std::cout << std::endl;
	} catch (const std::exception &e) {
		std::cout << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
