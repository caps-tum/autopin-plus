/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#pragma once

#include <QObject>
#include <mosquitto.h>
#include <AutopinPlus/Configuration.h>

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
	 * \brief Initalizes the MQTTClient
	 *
	 * \param[in] config Reference to the global configuration.
	 */
	static std::string init(const Configuration &config);

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
