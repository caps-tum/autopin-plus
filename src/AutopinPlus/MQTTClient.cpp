/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/MQTTClient.h>
#include <AutopinPlus/OS/OSServices.h>
#include <QCoreApplication>

using AutopinPlus::OS::OSServices;

namespace AutopinPlus {

const std::vector<std::string> MQTTClient::commands = {"AddProcess"};
const std::string MQTTClient::baseSuscriptionTopic =
	"AutopinPlus/" + OSServices::getHostname_static().toStdString() + "/";

MQTTClient::MQTTClient(){};

std::string MQTTClient::init(const Configuration &config) {

	std::string hostname = config.getConfigOption("mqtt.hostname").toStdString();
	if (hostname == "") hostname = "localhost";

	int port = config.getConfigOptionInt("mqtt.port");
	if (port == 0) port = 1883;

	int ret = MOSQ_ERR_SUCCESS;
	mosquitto_lib_init();

	std::string id = "autopin_" + std::to_string(QCoreApplication::applicationPid());
	getInstance().mosq = mosquitto_new(id.c_str(), false, &getInstance());

	// Just an local alias
	struct mosquitto *mosq = getInstance().mosq;
	if (mosq == nullptr) return "Cannot initalize MQTT client";

	// TODO: Configure callbacks
	mosquitto_message_callback_set(mosq, messageCallback);

	ret = mosquitto_connect(mosq, hostname.c_str(), port, 60);
	if (ret != MOSQ_ERR_SUCCESS) return "Cannot connect to MQTT broker on host " + hostname;

	mosquitto_reconnect_delay_set(mosq, 1, 30, false);
	ret = mosquitto_loop_start(mosq);
	if (ret != MOSQ_ERR_SUCCESS) return "Cannot initalize MQTT loop";

	for (auto command : commands) {
		const std::string suscriptionTopic = baseSuscriptionTopic + command;
		ret = mosquitto_subscribe(mosq, nullptr, suscriptionTopic.c_str(), 2);
		if (ret != MOSQ_ERR_SUCCESS) return "Cannot suscripe to MQTT topics";
	}

	return "";
}

void MQTTClient::messageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	// mosq is not needed, so this silences the warning.
	(void)mosq;

	MQTTClient *client = static_cast<MQTTClient *>(obj);
	std::string topic(message->topic);

	// Implementation for the command "AddProcess"
	const std::string &command = commands[0];
	if (topic.length() >= command.length() &&
		topic.compare(topic.length() - command.length(), command.length(), command) == 0) {
		QString text(static_cast<char *>(message->payload));
		client->emitSignalReceivedProcessConfig(text);
	}
}

void MQTTClient::emitSignalReceivedProcessConfig(const QString config) { emit sig_receivedProcessConfig(config); }

} // Namespace AutopinPlus
