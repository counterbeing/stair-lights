#include "config.h"
#include "mqtt_functions.h"
#include "globals.h"

void callback(char *topic, byte *payload, unsigned int length)
{
	String message;
	for (unsigned int i = 0; i < length; i++)
	{
		message += (char)payload[i]; // Build the message directly from payload
	}
	String topicStr = String(topic); // Convert topic to String once

	Serial.println(message);

	// Handling topics
	if (topicStr == "home/stair-balls/white_light_detector/set" && message == "ON")
	{
		switchAnimation(WHITE_MODE);
	}
	else if (topicStr == "home/stair-balls/mode_switch/set" && message == "ON")
	{
		switchAnimation(DEBUG_MODE);
	}
	else if (topicStr == "home/stair-balls/animation/trigger")
	{
		switchAnimation(MOVIE_MODE);
	}
}

void reconnect()
{
	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection...");
		if (client.connect("stair-balls"))
		{
			Serial.println("Connected to MQTT server!");
			// client.subscribe("home/stair-balls/light/on");
			client.subscribe("home/stair-balls/white_light_detector/set");
			client.subscribe("home/stair-balls/mode_switch/set");
			client.subscribe("home/stair-balls/animation/trigger");

			const char *button_discovery_payload = R"({
				"name": "Animation Trigger",
				"unique_id": "animation_trigger_btn",
				"command_topic": "home/stair-balls/animation/trigger",
				"payload_press": "trigger_animation",
				"device_class": "button"
			})";

			String mode_discovery_payload = "{"
																			"\"name\": \"Balls Mode Switch\","
																			"\"command_topic\": \"home/stair-balls/mode_switch/set\","
																			"\"state_topic\": \"home/stair-balls/mode_switch/state\","
																			"\"payload_on\": \"ON\","
																			"\"payload_off\": \"OFF\","
																			"\"unique_id\": \"BALLS_MODE_SWITCH\""
																			"}";

			String white_light_detector_payload = "{"
																						"\"name\": \"White light detector switch\","
																						"\"command_topic\": \"home/stair-balls/white_light_detector/set\","
																						"\"state_topic\": \"home/stair-balls/white_light_detector/state\","
																						"\"payload_on\": \"ON\","
																						"\"payload_off\": \"OFF\","
																						"\"unique_id\": \"BALLS_WHITE_LIGHT_DETECTOR\""
																						"}";

			Serial.println("Sending config json... ");

			bool result = client.publish("homeassistant/button/stair-balls/animation_trigger_btn/config", button_discovery_payload, true);
			bool result2 = client.publish("homeassistant/switch/stair-balls/BALLS_MODE_SWITCH/config", mode_discovery_payload.c_str(), true);
			bool result3 = client.publish("homeassistant/switch/stair-balls/BALLS_WHITE_LIGHT_DETECTOR/config", white_light_detector_payload.c_str(), true);

			if (result2 && result3)
			{
				Serial.println("Payload published successfully.");
			}
			else
			{
				Serial.println("Payload publishing failed.");
			}
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println("Trying again in 5 seconds");

			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void sendDebugMessage(const char *message)
{
	client.publish("debug", message);
}