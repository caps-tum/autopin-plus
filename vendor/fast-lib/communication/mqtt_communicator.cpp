/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#include "mqtt_communicator.hpp"

#include <boost/regex.hpp>

#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <queue>

namespace fast {

/// Helper function to make error codes human readable.
std::string mosq_err_string(const std::string &str, int code)
{
	return str + mosqpp::strerror(code);
}

class MQTT_subscription
{
public:
	virtual void add_message(const mosquitto_message *msg) = 0;
	virtual std::string get_message(const std::chrono::duration<double> &duration) = 0;
};

class MQTT_subscription_get : public MQTT_subscription
{
public:
	void add_message(const mosquitto_message *msg) override;
	std::string get_message(const std::chrono::duration<double> &duration) override;
private:
	std::mutex msg_queue_mutex;
	std::condition_variable msg_queue_empty_cv;
	std::queue<mosquitto_message*> messages; /// \todo Consider using unique_ptr.
};

class MQTT_subscription_callback : public MQTT_subscription
{
public:
	MQTT_subscription_callback(std::function<void(std::string)> callback);
	void add_message(const mosquitto_message *msg) override;
	std::string get_message(const std::chrono::duration<double> &duration) override;
private:
	std::string message;
	std::function<void(std::string)> callback;
};

void MQTT_subscription_get::add_message(const mosquitto_message *msg)
{
	mosquitto_message* buf = nullptr;
	buf = static_cast<mosquitto_message*>(std::malloc(sizeof(mosquitto_message)));
	if (!buf)
		throw std::runtime_error("malloc failed allocating mosquitto_message.");
	std::lock_guard<std::mutex> lock(msg_queue_mutex);
	messages.push(buf);
	mosquitto_message_copy(messages.back(), msg);
	if (messages.size() == 1)
		msg_queue_empty_cv.notify_one();
}

std::string MQTT_subscription_get::get_message(const std::chrono::duration<double> &duration)
{
	std::unique_lock<std::mutex> lock(msg_queue_mutex);
	if (duration == std::chrono::duration<double>::max()) {
		// Wait without timeout
		msg_queue_empty_cv.wait(lock, [this]{return !messages.empty();});
	} else {
		// Wait with timeout
		if (!msg_queue_empty_cv.wait_for(lock, duration, [this]{return !messages.empty();}))
			throw std::runtime_error("Timeout while waiting for message.");
	}
	auto msg = messages.front();
	messages.pop();
	lock.unlock();
	std::string buf(static_cast<char*>(msg->payload), msg->payloadlen);
	mosquitto_message_free(&msg);
	return buf;
}

MQTT_subscription_callback::MQTT_subscription_callback(std::function<void(std::string)> callback) :
	callback(std::move(callback))
{
}
		

void MQTT_subscription_callback::add_message(const mosquitto_message *msg)
{
	callback(std::string(static_cast<char*>(msg->payload), msg->payloadlen));
}

std::string MQTT_subscription_callback::get_message(const std::chrono::duration<double> &duration)
{
	(void) duration;
	throw std::runtime_error("Error in get_message: This topic is subscribed with callback.");
}

MQTT_communicator::MQTT_communicator(const std::string &id,
				     const std::string &publish_topic,
				     const std::string &host,
				     int port,
				     int keepalive,
				     const std::chrono::duration<double> &timeout) :
	mosqpp::mosquittopp(id.c_str()),
	default_publish_topic(publish_topic),
	connected(false)
{
	// Initialize mosquitto library if no other instance did
	if (ref_count++ == 0)
		mosqpp::lib_init();

	int ret;
	// Start threaded mosquitto loop
	if ((ret = loop_start()) != MOSQ_ERR_SUCCESS)
		throw std::runtime_error(mosq_err_string("Error starting mosquitto loop: ", ret));
	// Connect to MQTT broker. Uses condition variable that is set in on_connect, because
	// (re-)connect returning MOSQ_ERR_SUCCESS does not guarantee an fully established connection.
	{
		auto start = std::chrono::high_resolution_clock::now();
		ret = connect(host.c_str(), port, keepalive);
		while (ret != MOSQ_ERR_SUCCESS) {
			std::cout << mosq_err_string("Failed connecting to MQTT broker: ", ret) << std::endl;
			if (std::chrono::high_resolution_clock::now() - start > timeout)
				throw std::runtime_error("Timeout while trying to connect to MQTT broker.");
			std::this_thread::sleep_for(std::chrono::seconds(1));
			ret = reconnect();
		}
		std::unique_lock<std::mutex> lock(connected_mutex);
		auto time_left = timeout - (std::chrono::high_resolution_clock::now() - start);
		// Branch between wait and wait_for because if time_left is max wait_for does not work 
		// (waits until now + max -> overflow?).
		if (time_left != std::chrono::duration<double>::max()) {
			if (!connected_cv.wait_for(lock, time_left, [this]{return connected;}))
				throw std::runtime_error("Timeout while trying to connect to MQTT broker.");
		} else {
			connected_cv.wait(lock, [this]{return connected;});
		}
	}
}
MQTT_communicator::MQTT_communicator(const std::string &id,
				     const std::string &subscribe_topic,
				     const std::string &publish_topic,
				     const std::string &host,
				     int port,
				     int keepalive,
				     const std::chrono::duration<double> &timeout) :
	MQTT_communicator(id, publish_topic, host, port, keepalive, timeout)
{
	// Subscribe to default topic.
	default_subscribe_topic = subscribe_topic;
	add_subscription(default_subscribe_topic);
}

MQTT_communicator::~MQTT_communicator()
{
	// Disconnect from MQTT broker.
	disconnect();
	// Stop mosquitto loop
	loop_stop();
	// Cleanup of mosquitto library if this was the last MQTT_communicator object.
	if (--ref_count == 0)
		mosqpp::lib_cleanup();
}

void MQTT_communicator::add_subscription(const std::string &topic, int qos)
{
	// Save subscription in unordered_map.
	std::shared_ptr<MQTT_subscription> ptr = std::make_shared<MQTT_subscription_get>();
	std::unique_lock<std::mutex> lock(subscriptions_mutex);
	subscriptions.emplace(std::make_pair(topic, ptr));
	lock.unlock();
	// Send subscribe to MQTT broker.
	auto ret = subscribe(nullptr, topic.c_str(), qos);
	if (ret != MOSQ_ERR_SUCCESS)
		throw std::runtime_error(mosq_err_string("Error subscribing to topic \"" + topic + "\": ", ret));
}

void MQTT_communicator::add_subscription(const std::string &topic, std::function<void(std::string)> callback, int qos)
{
	// Save subscription in unordered_map.
	std::shared_ptr<MQTT_subscription> ptr = std::make_shared<MQTT_subscription_callback>(std::move(callback));
	std::unique_lock<std::mutex> lock(subscriptions_mutex);
	subscriptions.emplace(std::make_pair(topic, ptr));
	lock.unlock();
	// Send subscribe to MQTT broker.
	auto ret = subscribe(nullptr, topic.c_str(), qos);
	if (ret != MOSQ_ERR_SUCCESS)
		throw std::runtime_error(mosq_err_string("Error subscribing to topic \"" + topic + "\": ", ret));
}

void MQTT_communicator::remove_subscription(const std::string &topic)
{
	// Delete subscription from unordered_map. 
	// This does not invalidate references used by other threads due to use of shared_ptr.
	std::unique_lock<std::mutex> lock(subscriptions_mutex);
	subscriptions.erase(topic);
	lock.unlock();
	// Send unsubscribe to MQTT broker.
	auto ret = unsubscribe(nullptr, topic.c_str());
	if (ret != MOSQ_ERR_SUCCESS)
		throw std::runtime_error(mosq_err_string("Error subscribing to topic \"" + topic + "\": ", ret));
}

void MQTT_communicator::on_connect(int rc)
{
	if (rc == 0) {
		std::unique_lock<std::mutex> lock(connected_mutex);
		connected = true;
		lock.unlock();
		connected_cv.notify_one();
		std::cout << "Connection established." << std::endl;
	} else {
		std::cout << "Error on connect: " << mosqpp::connack_string(rc) << std::endl;
	}
}

void MQTT_communicator::on_disconnect(int rc)
{
	if (rc == 0) {
		std::cout << "Disconnected." << std::endl;
	} else {
		std::cout << mosq_err_string("Unexpected disconnect: ", rc) << std::endl;
	}
	std::lock_guard<std::mutex> lock(connected_mutex);
	connected = false;
}

boost::regex topic_to_regex(const std::string &topic)
{
	// Replace "+" by "\w*"
	auto regex_topic = boost::regex_replace(topic, boost::regex(R"((\+))"), R"(\\w*)");
	// Replace "#" by "\w*(?:/\w*)*$"
	regex_topic = boost::regex_replace(regex_topic, boost::regex(R"((#))"), R"(\\w*(?:/\\w*)*$)");
	return boost::regex(regex_topic);
}

void MQTT_communicator::on_message(const mosquitto_message *msg)
{
	try {
		std::vector<decltype(subscriptions)::mapped_type> matched_subscriptions;
		// Get all subscriptions matching the topic
		std::unique_lock<std::mutex> lock(subscriptions_mutex);
		for (auto &subscription : subscriptions) {
			if (boost::regex_match(msg->topic, topic_to_regex(subscription.first)))
				matched_subscriptions.push_back(subscription.second);
		}
		lock.unlock();
		// Add message to all matched subscriptions
		for (auto &subscription : matched_subscriptions)
			subscription->add_message(msg);
	} catch (const std::exception &e) { // Catch exceptions and do nothing to not break mosquitto loop.
		std::cout << "Exception in on_message: " << e.what() << std::endl;
	}

}

void MQTT_communicator::send_message(const std::string &message)
{
	send_message(message, "", 2);
}

void MQTT_communicator::send_message(const std::string &message, const std::string &topic, int qos)
{
	// Use default topic if empty string is passed.
	auto &real_topic = topic == "" ? default_publish_topic : topic;
	// Publish message to topic.
	int ret = publish(nullptr, real_topic.c_str(), message.size(), message.c_str(), qos, false);
	if (ret != MOSQ_ERR_SUCCESS)
		throw std::runtime_error(mosq_err_string("Error sending message: ", ret));
}

std::string MQTT_communicator::get_message()
{
	return get_message(default_subscribe_topic, std::chrono::duration<double>::max());
}

std::string MQTT_communicator::get_message(const std::string &topic)
{
	return get_message(topic, std::chrono::duration<double>::max());
}

std::string MQTT_communicator::get_message(const std::chrono::duration<double> &duration)
{
	return get_message(default_subscribe_topic, duration);
}

std::string MQTT_communicator::get_message(const std::string &topic, const std::chrono::duration<double> &duration)
{
	try {
		std::unique_lock<std::mutex> lock(subscriptions_mutex);
		auto &subscription = subscriptions.at(topic);
		lock.unlock();
		return subscription->get_message(duration);
	} catch (const std::out_of_range &e) {
		throw std::out_of_range("Topic not found in subscriptions.");
	}
}

unsigned int MQTT_communicator::ref_count = 0;

} // namespace fast
