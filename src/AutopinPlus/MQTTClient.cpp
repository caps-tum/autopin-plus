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

#include <AutopinPlus/MQTTClient.h>
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>

using AutopinPlus::OS::Linux::OSServicesLinux;

namespace AutopinPlus {

const std::vector<std::string> MQTTClient::commands = {"AddProcess"};
const std::string MQTTClient::baseSuscriptionTopic =
	"AutopinPlus/" + OSServicesLinux::getHostname_static().toStdString() + "/";

MQTTClient::MQTTClient(){};

MQTTClient::MQTT_STATUS MQTTClient::init(std::string hostname, int port) {

	int ret = MOSQ_ERR_SUCCESS;
	mosquitto_lib_init();
	getInstance().mosq = mosquitto_new(nullptr, true, nullptr);

	// Just an local alias
	struct mosquitto *mosq = getInstance().mosq;
	if (mosq == nullptr) return MOSQUITTO;

	// TODO: Configure callbacks
	mosquitto_message_callback_set(mosq, messageCallback);

	ret = mosquitto_connect(mosq, hostname.c_str(), port, 60);
	if (ret != MOSQ_ERR_SUCCESS) return CONNECT;

	ret = mosquitto_loop_start(mosq);
	if (ret != MOSQ_ERR_SUCCESS) return LOOP;

	for (auto command : commands) {
		const std::string suscriptionTopic = baseSuscriptionTopic + command;
		ret = mosquitto_subscribe(mosq, nullptr, suscriptionTopic.c_str(), 2);
		if (ret != MOSQ_ERR_SUCCESS) return SUSCRIBE;
	}

	return OK;
}

void MQTTClient::messageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	std::string topic(message->topic);

	// AddProcess
	std::string command = commands[0];
	if (topic.compare(topic.length() - command.length(), command.length(), command)) {
		QString text = "";
		char *ptr = static_cast<char *>(message->payload);

		for (int i = 0; i < message->payloadlen; i++) {
			text += ptr[i];
		}

		getInstance().emitSignalReceivedProcessConfig(text);
	}
}

void MQTTClient::emitSignalReceivedProcessConfig(const QString config) { emit sig_receivedProcessConfig(config); }

} // Namespace AutopinPlus
