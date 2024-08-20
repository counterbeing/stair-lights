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
			{MqttTopics::NightVisionSet, NIGHT_VISION},
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

Payload generateDiscoveryPayload(const AnimationConfig &config)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), R"({
        "name": "%s",
        "unique_id": "%s",
        "command_topic": "%s",
        "state_topic": "%s"
    })",
					 config.name, config.uniqueId, config.buildTriggerTopic(), config.buildStateTopic());

	String topic = "homeassistant/button/" + String(config.uniqueId) + "/config";
	return {topic, String(buffer), true};
}

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
			generateDiscoveryPayload("Night Vision", "night_vision"),
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

void subscribeToAnimations()
{
	for (int i = 0; i < animationSize; ++i)
	{
		client.subscribe(animations[i].buildTriggerTopic().c_str());
	}
}

// void sendDiscoveryPayloads()
// {
// 	for (int i = 0; i < animationSize; ++i)
// 	{

// 		const Payload payload = generateDiscoveryPayload(animations[i]);
// 		client.subscribe(animations[i].buildTriggerTopic().c_str());
// 	}
// }

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