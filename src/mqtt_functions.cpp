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

	// Mapping topic strings to animation states
	static const std::map<String, AnimationState> topicToAnimation{
			{MqttTopics::WhiteModeSet, WHITE_MODE},
			{MqttTopics::SnakeModeSet, SNAKE_MODE},
			{MqttTopics::DebugModeSet, DEBUG_MODE},
			{MqttTopics::MovieModeSet, MOVIE_MODE}};

	if (message == "PRESS" || topicStr == MqttTopics::MovieModeSet)
	{
		auto it = topicToAnimation.find(topicStr);
		if (it != topicToAnimation.end())
		{
			switchAnimation(it->second);
		}
	}
}

struct Payload
{
	String topic; // Change type to String for automatic memory management
	String payload;
	bool retain;
};

Payload generateDiscoveryPayload(const char *name, const char *uniqueId)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), R"({
        "name": "%s",
        "unique_id": "%s",
        "command_topic": "home/stair-balls/%s/trigger",
        "state_topic": "home/stair-balls/%s/state"
    })",
					 name, uniqueId, uniqueId, uniqueId);

	String topic = "homeassistant/button/" + String(uniqueId) + "/config";
	return {topic, String(buffer), true};
}

void sendDiscoveryPayloads()
{

	static const Payload payloads[] = {
			generateDiscoveryPayload("White Mode", "white_mode"),
			generateDiscoveryPayload("Debug Mode", "debug_mode"),
			generateDiscoveryPayload("Snake Mode", "snake_mode"),
			generateDiscoveryPayload("Movie Mode", "movie_mode")};

	Serial.println("Sending config json...");
	for (const auto &payload : payloads)
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
}

void sendDebugMessage(const char *message)
{
	client.publish("debug", message);
}

void reconnect()
{
	static const char *topics[] = {
			"home/stair-balls/movie_mode/trigger",
			"home/stair-balls/white_mode/trigger",
			"home/stair-balls/debug_mode/trigger",
			"home/stair-balls/snake_mode/trigger"};

	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection...");
		if (client.connect("stair-balls"))
		{
			Serial.println("Connected to MQTT server!");
			for (const auto &topic : topics)
			{
				client.subscribe(topic);
			}

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