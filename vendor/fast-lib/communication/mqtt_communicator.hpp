/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#ifndef FAST_LIB_MQTT_COMMUNICATOR_HPP
#define FAST_LIB_MQTT_COMMUNICATOR_HPP

#include "communicator.hpp"

#include <mosquittopp.h>

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>

namespace fast {

class MQTT_subscription;

/**
 * \brief A specialized Communicator to provide communication using mqtt framework mosquitto.
 *
 * Initialize mosquitto before using this class (e.g. using mosqpp::lib_init() and mosqpp::lib_cleanup() in main)
 */
class MQTT_communicator : 
	public Communicator, 
	private mosqpp::mosquittopp
{
public:
	/**
	 * \brief Constructor for MQTT_communicator.
	 *
	 * Establishes a connection, starts async mosquitto loop and subscribes to topic.
	 * The async mosquitto loop runs in a seperate thread so internal functions should be threadsafe.
	 * There is no need to initialize mosquitto before using this class, because this is handled in the constructor
	 * and destructor.
	 * Establishing a connection is retried every second until success or timeout.
	 * \param id
	 * \param publish_topic The topic to publish messages to by default.
	 * \param host The host to connect to.
	 * \param port The port to connect to.
	 * \param keepalive The number of seconds the broker sends periodically ping messages to test if client is still alive.
	 * \param timeout The timeout of establishing a connection to the MQTT broker e.g. std::chrono::seconds(10).
	 */
	MQTT_communicator(const std::string &id,
			  const std::string &publish_topic,
			  const std::string &host,
			  int port,
			  int keepalive,
			  const std::chrono::duration<double> &timeout = std::chrono::duration<double>::max());
	/**
	 * \brief Constructor for MQTT_communicator.
	 *
	 * Establishes a connection, starts async mosquitto loop and subscribes to topic.
	 * The async mosquitto loop runs in a seperate thread so internal functions should be threadsafe.
	 * There is no need to initialize mosquitto before using this class, because this is handled in the constructor
	 * and destructor.
	 * Establishing a connection is retried every second until success or timeout.
	 * This overload also adds an initial subscription to a topic.
	 * \param id
	 * \param subscribe_topic The topic to subscribe to.
	 * \param publish_topic The topic to publish messages to by default.
	 * \param host The host to connect to.
	 * \param port The port to connect to.
	 * \param keepalive The number of seconds the broker sends periodically ping messages to test if client is still alive.
	 * \param timeout The timeout of establishing a connection to the MQTT broker e.g. std::chrono::seconds(10).
	 */
	MQTT_communicator(const std::string &id,
			  const std::string &subscribe_topic,
			  const std::string &publish_topic,
			  const std::string &host,
			  int port,
			  int keepalive,
			  const std::chrono::duration<double> &timeout = std::chrono::duration<double>::max());
	/**
	 * \brief Destructor for MQTT_communicator.
	 *
	 * Stops async mosquitto loop and disconnects.
	 */
	~MQTT_communicator();
	/**
	 * \brief Add a subscription to listen on for messages.
	 *
	 * Adds a subscription on a topic. The messages can be retrieved by calling get_message().
	 * Messages are queued seperate per topic. Therefore multiple topics can be subscribed simultaneously.
	 * \param topic The topic to listen on.
	 * \param qos The quality of service (0|1|2 - see mosquitto documentation for further information)
	 */
	void add_subscription(const std::string &topic, int qos = 2);
	/**
	 * \brief Add a subscription with a callback to retrieve messages.
	 *
	 * Adds a subscription on a topic. On each message that arrives the callback is called with the payload
	 * string as parameter. All exceptions derived from std::exception are caught if thrown by callback.
	 * \param topic The topic to listen on.
	 * \param callback The function to call when a new message arrives on topic.
	 * \param qos The quality of service (see mosquitto documentation for further information)
	 */
	void add_subscription(const std::string &topic, std::function<void(std::string)> callback, int qos = 2);
	/**
	 * \brief Remove a subscription.
	 *
	 * \param topic The topic the subscription was listening on.
	 */
	void remove_subscription(const std::string &topic);
	/**
	 * \brief Send a message to the default topic.
	 *
	 * \param message The message string to send on the default topic.
	 */
	void send_message(const std::string &message) override;
	/**
	 * \brief Send a message to a specific topic.
	 *
	 * \param message The message string to send on the topic.
	 * \param topic The topic to send the message on.
	 * \param qos The quality of service (0|1|2 - see mosquitto documentation for further information)
	 */
	void send_message(const std::string &message, const std::string &topic, int qos = 2);
	/**
	 * \brief Get a message from a default topic.
	 */
	std::string get_message() override;
	/**
	 * \brief Get a message from a specific topic.
	 *
	 * \param topic The topic to listen on for a message.
	 */
	std::string get_message(const std::string &topic);
	/**
	 * \brief Get a message from a default topic with timeout.
	 *
	 * \param duration The duration until timeout.
	 */
	std::string get_message(const std::chrono::duration<double> &duration);
	/**
	 * \brief Get a message from a specific topic with timeout.
	 *
	 * \param topic The topic to listen on for a message.
	 * \param duration The duration until timeout.
	 */
	std::string get_message(const std::string &topic, 
				const std::chrono::duration<double> &duration);
private:
	/**
	 * \brief Callback for established connections.
	 */
	void on_connect(int rc) override;
	/**
	 * \brief Callback for disconnected connections.
	 */
	void on_disconnect(int rc) override;
	/**
	 * \brief Callback for received messages.
	 */
	void on_message(const mosquitto_message *msg) override;

	std::string default_subscribe_topic;
	std::string default_publish_topic;

	std::unordered_map<std::string, std::shared_ptr<MQTT_subscription>> subscriptions;
	std::mutex subscriptions_mutex;

	std::mutex connected_mutex;
	std::condition_variable connected_cv;
	bool connected;
	static unsigned int ref_count;
};

} // namespace fast
#endif
