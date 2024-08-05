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
			{MqttTopics::WhiteLightDetectorSet, WHITE_MODE},
			{MqttTopics::ModeSwitchSet, DEBUG_MODE},
			{MqttTopics::AnimationTrigger, MOVIE_MODE}};

	if (message == "ON" || topicStr == MqttTopics::AnimationTrigger)
	{
		auto it = topicToAnimation.find(topicStr);
		if (it != topicToAnimation.end())
		{
			switchAnimation(it->second);
		}
	}
}

String createDiscoveryPayload(const char *name, const char *uniqueId, const char *commandTopic, const char *stateTopic)
{
	char buffer[512]; // Adjust size as needed for your payloads
	snprintf(buffer, sizeof(buffer), R"({
        "name": "%s",
        "unique_id": "%s",
        "command_topic": "%s",
        "state_topic": "%s"
    })",
					 name, uniqueId, commandTopic, stateTopic);

	return String(buffer);
}

struct Payload
{
	const char *topic;
	const char *payload;
	bool retain;
};

Payload generatePizayload(const char *name, const char *uniqueId)
{
	char buffer[512]; // Adjust size as needed for your payloads
	snprintf(buffer, sizeof(buffer), R"({
		"name": "%s",
		"unique_id": "%s",
		"command_topic": "home/stair-balls/%s/trigger",
		"state_topic": "home/stair-balls/%s/state",
		"payload_press": "trigger_animation",
		"device_class": "button"
	})",
					 name, uniqueId, uniqueId, uniqueId);

	std::string topic = "homeassistant/button/" + std::string(uniqueId) + "/config";
	return {topic.c_str(), buffer, true};
}

void sendDiscoveryPayloads()
{

	static const Payload payloads[] = {
			generatePizayload("My test name", "my-test-name"),
			{"homeassistant/button/stair-balls/animation_trigger_btn/config", R"({
            "name": "Animation Trigger",
            "unique_id": "animation_trigger_btn",
            "command_topic": "home/stair-balls/animation/trigger",
            "payload_press": "trigger_animation",
            "device_class": "button"
        })",
			 true},
			{"homeassistant/switch/stair-balls/BALLS_MODE_SWITCH/config", R"({
            "name": "Balls Mode Switch",
            "command_topic": "home/stair-balls/mode_switch/set",
            "state_topic": "home/stair-balls/mode_switch/state",
            "payload_on": "ON",
            "payload_off": "OFF",
            "unique_id": "BALLS_MODE_SWITCH"
        })",
			 true},
			{"homeassistant/button/trigger_action_btn/config", R"({
            "name": "EXAMPLEBUTTON",
            "command_topic": "home/example-topic",
            "state_topic": "home/exmaple-state",
            "unique_id": "EXAMPLE_BUTTON"
        })",
			 true},
			{"homeassistant/switch/stair-balls/BALLS_WHITE_LIGHT_DETECTOR/config", R"({
            "name": "White light detector switch",
            "command_topic": "home/stair-balls/white_light_detector/set",
            "state_topic": "home/stair-balls/white_light_detector/state",
            "payload_on": "ON",
            "payload_off": "OFF",
            "unique_id": "BALLS_WHITE_LIGHT_DETECTOR"
        })",
			 true}};

	Serial.println("Sending config json... ");
	for (const auto &payload : payloads)
	{
		if (!client.publish(payload.topic, payload.payload, payload.retain))
		{
			Serial.println("Payload publishing failed.");
		}
	}
}

void sendDebugMessage(const char *message)
{
	client.publish("debug", message);
}

void reconnect()
{
	static const char *topics[] = {
			"home/stair-balls/white_light_detector/set",
			"home/stair-balls/mode_switch/set",
			"home/stair-balls/animation/trigger"};

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