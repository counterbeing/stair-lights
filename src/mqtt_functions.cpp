#include "config.h"
#include "mqtt_functions.h"

void callback(char *topic, byte *payload, unsigned int length)
{
	String message;
	for (unsigned int i = 0; i < length; i++)
	{
		message += (char)payload[i];
	}

	Serial.println(message);

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
			Serial.println("connected");
			client.subscribe("home/stair-balls/light/on");

			String discovery_payload = "{"
																 "\"name\": \"Onboard LED\","
																 "\"command_topic\": \"home/stair-balls/light/on\","
																 "\"state_topic\": \"home/stair-balls/light/state\","
																 "\"payload_on\": \"ON\","
																 "\"payload_off\": \"OFF\","
																 "\"unique_id\": \"ESP32_ONBOARD_LED\""
																 "}";

			Serial.println("Sending config json... ");
			bool result = client.publish("homeassistant/light/stair-balls/light/config", discovery_payload.c_str(), true);
			if (result)
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
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void sendDebugMessage(const char *message)
{
	client.publish("debug", message);
}