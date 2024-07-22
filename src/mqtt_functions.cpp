#include "config.h"
#include "mqtt_functions.h"
#include "globals.h"

void callback(char *topic, byte *payload, unsigned int length)
{
	String message;
	for (unsigned int i = 0; i < length; i++)
	{
		message += (char)payload[i];
	}

	Serial.println(message);

	if (String(topic) == "home/stair-balls/mode_switch/set")
	{
		if (message == "ON")
		{
			currentAnimation = DEBUG_MODE;
		}
		else if (message == "OFF")
		{
			currentAnimation = SNAKE_MODE;
		}
	}

	// Handle received message
	if (String(topic) == "home/stair-balls/light/on" && message == "ON")
	{
		digitalWrite(LED_BUILTIN, LOW);
	}
	else if (String(topic) == "home/stair-balls/light/on" && message == "OFF")
	{
		digitalWrite(LED_BUILTIN, HIGH);
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
			client.subscribe("home/stair-balls/light/on");
			client.subscribe("home/stair-balls/mode_switch/set");

			String discovery_payload = "{"
																 "\"name\": \"OnBoard LED\","
																 "\"command_topic\": \"home/stair-balls/light/on\","
																 "\"state_topic\": \"home/stair-balls/light/state\","
																 "\"payload_on\": \"ON\","
																 "\"payload_off\": \"OFF\","
																 "\"unique_id\": \"ESP32_ONBOARD_LED\""
																 "}";

			String mode_discovery_payload = "{"
																			"\"name\": \"Balls Mode Switch\","
																			"\"command_topic\": \"home/stair-balls/mode_switch/set\","
																			"\"state_topic\": \"home/stair-balls/mode_switch/state\","
																			"\"payload_on\": \"ON\","
																			"\"payload_off\": \"OFF\","
																			"\"unique_id\": \"BALLS_MODE_SWITCH\""
																			"}";

			Serial.println("Sending config json... ");
			bool result1 = client.publish("homeassistant/light/stair-balls/light/config", discovery_payload.c_str(), true);
			bool result2 = client.publish("homeassistant/switch/stair-balls/BALLS_MODE_SWITCH/config", mode_discovery_payload.c_str(), true);

			if (result1 && result2)
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