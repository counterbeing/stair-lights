#include <Arduino.h>
#include "FastLED.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include "arduino_secrets.h"
#include <ArduinoOTA.h>

#define LED_DATA_PIN 25
#define NUM_LEDS 3
#define LED_BUILTIN 2

CRGB leds[NUM_LEDS];

int lastState = LOW;
int currentState;
int currentAnimationIndex = 0;

int countUPAndDown(int min, int max, int step)
{
  static int count = min;
  static bool up = true;

  if (up)
  {
    count += step;
    if (count >= max)
    {
      count = max;
      up = false;
    }
  }
  else
  {
    count -= step;
    if (count <= min)
    {
      count = min;
      up = true;
    }
  }
  return count;
}

void rainbow()
{
  static unsigned long lastRainbow = 0;
  static int hue = 0;

  if (millis() - lastRainbow > 40)
  {
    uint8_t deltaHue = countUPAndDown(0, 10, 1);
    lastRainbow = millis();
    hue = (hue + 1) % 256;
    fill_rainbow(leds, NUM_LEDS, hue, 1);
    FastLED.show();
  }
}

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

WiFiClient espClient;
PubSubClient client(espClient);

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

      client.publish("debug", "running");

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

void bootPattern()
{
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(800);
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(800);
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(800);
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  bootPattern();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print('.');
  }
  client.setServer("home.corylogan.com", 1883);
  client.setCallback(callback);
  Serial.println("BOOTED");

  ArduinoOTA.setHostname("stair-balls");
  // ArduinoOTA.setPassword("password");
  ArduinoOTA.begin();
  // ArduinoOTA.begin(WiFi.localIP(), "stair-balls", "password", InternalStorage);

  printWifiStatus();
}

// A function that moves a rainbow from one end of the strip to the other

// color fade
void fadeAllColors()
{
  static unsigned long lastFade = 0;
  static int fade = 0;

  if (millis() - lastFade > 400)
  {
    lastFade = millis();
    fade = (fade + 1) % 256;
    fill_solid(leds, NUM_LEDS, CHSV(fade, 255, 255));
    FastLED.show();
  }
}

void cleanup()
{
  fill_solid(leds, NUM_LEDS, CRGB::BlanchedAlmond);
  FastLED.show();
}

void night_vision()
{
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}

void handleAnimations()
{
  switch (currentAnimationIndex)
  {
  case 0:
    FastLED.setBrightness(64);
    night_vision();
    break;
  case 1:
    FastLED.setBrightness(240);
    night_vision();
    break;
  case 2:
    FastLED.setBrightness(150);
    cleanup();
    break;
  case 3:
    FastLED.setBrightness(255);
    cleanup();
    break;
  case 4:
    FastLED.setBrightness(255);
    rainbow();
    break;
  default:
    FastLED.setBrightness(255);
    fadeAllColors();
    break;
  }
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  delay(1);
  rainbow();
}
