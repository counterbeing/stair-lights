#include "config.h"
#include "mqtt_functions.h"
#include "globals.h"
#include "mqtt_topics.h"
#include <map>
#include <string>

void callback(char *topic, byte *payload, unsigned int length)
{
	String message(payload, length); // Directly initialize with length
	String topicStr(topic);

	Serial.println(message);

	// Initialize the current animation state to None (or some default value)
	AnimationState currentState = WHITE_MODE; // Replace WHITE_MODE with a valid default value

	// Iterate over the animations array to find the matching topic
	for (size_t i = 0; i < animationSize; ++i)
	{
		std::string stdTopic = animations[i].buildTriggerTopic();
		String topicStr = String(stdTopic.c_str());
		if (topicStr.equals(topic))
		{
			currentState = animations[i].state;
			break;
		}
	}
	if (message == "PRESS" || message == String(MqttTopics::MovieModeSet))
	{
		switchAnimation(currentState);
	}
}

struct Payload
{
	String topic; // Change type to String for automatic memory management
	String payload;
	bool retain;
};

Payload generateDiscoveryPayload(const AnimationConfig &config)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), R"({
        "name": "%s",
        "unique_id": "%s",
        "command_topic": "%s",
        "state_topic": "%s"
    })",
					 config.name, config.uniqueId, config.buildTriggerTopic().c_str(), config.buildStateTopic().c_str());

	String topic = "homeassistant/button/" + String(config.uniqueId) + "/config";
	return {topic, String(buffer), true};
}

void sendDiscoveryPayload(const Payload payload)
{
	if (!client.connected())
	{
		reconnect();
	}

	Serial.println("Publishing to topic: " + payload.topic);
	if (!client.publish(payload.topic.c_str(), payload.payload.c_str(), payload.retain))
	{
		Serial.println("Payload publishing failed.");
	}
	else
	{
		Serial.println("Payload published successfully.");
	}
	delay(100);
}

void sendDebugMessage(const char *message)
{
	client.publish("debug", message);
}

void subscribeToAnimations()
{
	for (int i = 0; i < animationSize; ++i)
	{
		client.subscribe(animations[i].buildTriggerTopic().c_str());
	}
}

void sendDiscoveryPayloads()
{
	for (int i = 0; i < animationSize; ++i)
	{

		const Payload payload = generateDiscoveryPayload(animations[i]);
		sendDiscoveryPayload(payload);
		client.subscribe(animations[i].buildTriggerTopic().c_str());
	}
}

void reconnect()
{
	Serial.print("Attempting MQTT connection...");
	while (!client.connected())
	{
		if (client.connect("stair-balls"))
		{
			Serial.println("Connected to MQTT server!");

			// Subscribe to trigger topics
			subscribeToAnimations();

			// Send configuration payloads
			sendDiscoveryPayloads();
		}
		else
		{
			Serial.print("Failed, rc=");
			Serial.println(client.state());
			Serial.println("Trying again in 5 seconds");
			delay(5000);
		}
	}
}

bool reconnectNonBlocking()
{
	Serial.print("Attempting MQTT connection (non-blocking)...");

	if (client.connect("stair-balls"))
	{
		Serial.println("Connected to MQTT server!");

		// Subscribe to trigger topics
		subscribeToAnimations();

		// Send configuration payloads
		sendDiscoveryPayloads();

		return true;
	}
	else
	{
		Serial.print("Failed, rc=");
		Serial.println(client.state());
		return false;
	}
}