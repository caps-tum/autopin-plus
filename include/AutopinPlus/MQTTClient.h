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

#pragma once

#include <QObject>
#include <mosquitto.h>
#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/StandardConfiguration.h>

namespace AutopinPlus {

/*!
 * \brief Singleton, which handles the MQQT Communication
 */

class MQTTClient : public QObject {
	Q_OBJECT
  public:
	/*!
	 * \brief Get the instance of the MQTTClient
	 */
	static MQTTClient &getInstance() {
		static MQTTClient instance;
		return instance;
	}

	/*!
	 * \brief Return status of MQTTClient::init()
	 *
	 * \sa init
	 */
	enum MQTT_STATUS { OK, MOSQUITTO, CONNECT, LOOP, SUSCRIBE };

	/*!
	 * \brief Initalizes the MQTTClient
	 */
	static MQTTClient::MQTT_STATUS init(std::string hostname, int port);

	/*!
	 * Delete funtions to ensure singleton functionality
	 */
	MQTTClient(MQTTClient const &) = delete;
	void operator=(MQTTClient const &) = delete;

signals:
	/*!
	 * Is emitted when the MQTTClient receives a message to a add a
	 * process.
	 *
	 * \param[in] text  QString, which contains the configuration
	 */
	void sig_receivedProcessConfig(const QString config);

  private:
	/*!
	 * \brief Constructor
	 */
	MQTTClient();

	/*!
	 * \brief Strings, which this client suscripes to
	 */
	static const std::vector<std::string> commands;

	/*!
	 * \brief Base suscription topic
	 */
	static const std::string baseSuscriptionTopic;

	/*!
	 * Mosquitto instance
	 */
	struct mosquitto *mosq;

	/*!
	 * Callback for mosquitto, when a message is received
	 */
	static void messageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

	/*!
	 * \brief Helper function to emit the signal sig_receivedProcessConfig
	 */
	void emitSignalReceivedProcessConfig(const QString config);
};

} // namespace AutopinPlus
