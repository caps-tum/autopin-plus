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

#include <AutopinPlus/MQQTClient.h>
#include <AutopinPlus/OS/Linux/OSServicesLinux.h>

using AutopinPlus::OS::Linux::OSServicesLinux;

namespace AutopinPlus {

MQQTClient::MQQTClient() {};

int MQQTClient::init() {
	int ret = MOSQ_ERR_SUCCESS;

	mosquitto_lib_init();

	mosq = mosquitto_new(nullptr, true, nullptr);
	if (mosq == nullptr)
		return -1;

	// TODO: Configure callbacks
	mosquitto_message_callback_set(mosq, messageCallback);
	
	ret = mosquitto_connect(mosq, "localhost", 1883, 60);
	if (ret != MOSQ_ERR_SUCCESS)
		return -2;

	ret = mosquitto_loop_start(mosq);
	if (ret != MOSQ_ERR_SUCCESS)
		return -3;

	const char* suscription_topic = "AutopinPlus/" + OSServicesLinux::getHostname_static().toAscii() + "/addProcess";
	ret = mosquitto_subscribe(mosq, nullptr, suscription_topic, 2);
	if (ret != MOSQ_ERR_SUCCESS)
		return -4;

	return 0;
}

void MQQTClient::messageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	QString text = "";
	char* ptr = static_cast<char*>(message->payload);

	for (int i = 0; i <message->payloadlen; i++) {
		text += ptr[i];
	}

	getInstance().emitSignalReceivedProcessConfig(text);
}

void MQQTClient::emitSignalReceivedProcessConfig(const QString config) {
	emit sig_receivedProcessConfig(config);
}	

} //Namespace AutopinPlus
