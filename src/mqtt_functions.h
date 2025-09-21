#pragma once
#include <PubSubClient.h>
extern PubSubClient client;

void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
bool reconnectNonBlocking();
void sendDebugMessage(const char *message);