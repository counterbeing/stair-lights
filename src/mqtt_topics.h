#pragma once
#include <PubSubClient.h>
extern PubSubClient client;

void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void sendDebugMessage(const char *message);

struct MqttTopics
{
	static const String MovieModeSet;
	static const String WhiteModeSet;
	static const String SnakeModeSet;
	static const String DebugModeSet;
	static const String NightVisionSet;
};
