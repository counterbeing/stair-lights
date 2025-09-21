#include <Arduino.h>
#include "FastLED.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include "arduino_secrets.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "globals.h"

#include "config.h"
#include "mqtt_functions.h"
#include "animations.h"

CRGB leds[NUM_LEDS];

int lastState = LOW;
int currentState;
int currentAnimationIndex = 0;

String hostname = "stair-balls";

WiFiClient espClient;
PubSubClient client(espClient);

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTION_SENSOR_PIN, INPUT);

  pinMode(MOTION_SENSOR_PIN_2, INPUT);
  // analogSetPinAttenuation(MOTION_SENSOR_PIN_2, ADC_11db);

  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  bootPattern();

  WiFi.mode(WIFI_STA);
  ArduinoOTA.setHostname("stair-balls");
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  if (MDNS.begin(hostname.c_str()))
  {
    Serial.println("MDNS responder started");
  }

  // Non-blocking WiFi connection with 30-second timeout
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 30000)
  {
    delay(100);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    printWifiStatus();
  }
  else
  {
    Serial.println("\nWiFi connection timeout - continuing without network");
  }
  client.setServer("home.corylogan.com", 1883);
  client.setBufferSize(512);
  client.setCallback(callback);
  Serial.println("BOOTED");

  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.begin();
  }
}

void loop()
{
  // testingMotion();

  // Handle WiFi reconnection non-blocking
  static unsigned long lastWifiReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - lastWifiReconnectAttempt > 10000) // Try every 10 seconds
    {
      lastWifiReconnectAttempt = millis();
      Serial.println("Attempting WiFi reconnection...");
      WiFi.disconnect();
      WiFi.begin(SECRET_SSID, SECRET_PASS);
    }
  }
  else
  {
    // Only handle OTA when WiFi is connected
    ArduinoOTA.handle();

    // Handle MQTT reconnection non-blocking
    static unsigned long lastMqttReconnectAttempt = 0;
    if (!client.connected())
    {
      if (millis() - lastMqttReconnectAttempt > 5000) // Try every 5 seconds
      {
        lastMqttReconnectAttempt = millis();
        reconnectNonBlocking();
      }
    }
    else
    {
      client.loop();
    }
  }

  // static int lastReportTime = millis();

  // if ((millis() - lastReportTime) > 50)
  // {

  //   lastReportTime = millis();
  //   if (digitalRead(MOTION_SENSOR_PIN_2) == HIGH)
  //   {
  //     sendDebugMessage("HIGH");
  //   }
  //   else
  //   {
  //     sendDebugMessage("LOW");
  //   }
  // }
  switch (currentAnimation)
  {
  case DEBUG_MODE:
    rgbLoop();
    break;

  case SNAKE_MODE:
    snakeLedsOnMotion();
    break;

  case MOVIE_MODE:
    arrows();
    break;

  case NIGHT_VISION:
    nightVision();
    break;

  case TWINKLE_TWANKLE:
    twinkleTwankle();
    break;

  case FUZZ_WAVE:
    fuzzWave();
    break;

  case ENERGY_BATTLE:
    energyBattle();
    break;

  case WHITE_MODE:
    whiteWhenMotionDetected();
    break;

  case SIN_COLOR_FADE:
    sinColorFade();
    break;

  case BOUNCING_BALL:
    bouncingBall();
    break;
  }

  // This has to be added because the esp32 is too fast
  delay(1);
}
