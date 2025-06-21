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

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print('.');
  }
  client.setServer("home.corylogan.com", 1883);
  client.setBufferSize(512);
  client.setCallback(callback);
  Serial.println("BOOTED");

  ArduinoOTA.begin();
  printWifiStatus();
}

void loop()
{
  // testingMotion();
  ArduinoOTA.handle();
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

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
  }

  // This has to be added because the esp32 is too fast
  delay(1);
}
