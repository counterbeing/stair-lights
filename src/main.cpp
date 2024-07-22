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

CRGB leds[NUM_LEDS];

int lastState = LOW;
int currentState;
int currentAnimationIndex = 0;

String hostname = "stair-balls";

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

WiFiClient espClient;
PubSubClient client(espClient);

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

int currentBrightness = 0;
unsigned long lastMotionDetectedTime = 0;
unsigned long lastDimmingTime = 0;

void flashLedsOnMotion()
{
  unsigned long currentMillis = millis();

  if (digitalRead(MOTION_SENSOR_PIN) == HIGH && (currentMillis - lastMotionDetectedTime) > 10)
  {
    sendDebugMessage("motion detected");
    currentBrightness += 1;
    if (currentBrightness > 255)
      currentBrightness = 255;
    lastMotionDetectedTime = currentMillis;
  }

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(0, 255, currentBrightness);
  }
  FastLED.show();

  unsigned long dimmingInterval = map(currentBrightness, 0, 255, 1, 75); // As brightness decreases, dimming interval decreases
  if (currentMillis - lastMotionDetectedTime > 500 && currentMillis - lastDimmingTime > dimmingInterval)
  {
    currentBrightness -= 1;
    if (currentBrightness < 0)
      currentBrightness = 0;
    lastDimmingTime = currentMillis; // Update the last dimming time
  }
}

unsigned long lastShiftTime = 0;
void snakeLedsOnMotion()
{
  unsigned long currentMillis = millis();

  static boolean motionDetected = false;
  static boolean firstMotion = true;
  static uint8_t hue = 0; // Store hue to continuously update and loop through colors

  if (digitalRead(MOTION_SENSOR_PIN) == HIGH)
  {
    if (!motionDetected || (currentMillis - lastMotionDetectedTime) > 10)
    {
      sendDebugMessage("motion detected");
      motionDetected = true;
      lastMotionDetectedTime = currentMillis;
      if (firstMotion)
      {
        hue = random(256);             // Initialize hue with a random value
        leds[0] = CHSV(hue, 255, 255); // Use full saturation and value for vivid colors
        firstMotion = false;
      }
    }
  }
  else if (currentMillis - lastMotionDetectedTime > 2000)
  {
    motionDetected = false;
    firstMotion = true; // Reset for the next motion event
  }

  if (motionDetected)
  {
    if (currentMillis - lastShiftTime > 100)
    {
      for (int i = NUM_LEDS - 1; i > 0; i--)
      {
        leds[i] = leds[i - 1];
      }

      // Increment hue for the first LED to ensure color loop
      hue = (hue + 1) % 256;
      leds[0] = CHSV(hue, 255, 255);

      lastShiftTime = currentMillis;
    }
  }
  else
  {
    // After two seconds of no motion, start pushing off the LEDs
    if (currentMillis - lastShiftTime > 100)
    {
      for (int i = NUM_LEDS - 1; i > 0; i--)
      {
        leds[i] = leds[i - 1];
      }
      leds[0] = CHSV(0, 0, 0); // Turn off the first LED

      lastShiftTime = currentMillis;
    }
  }

  FastLED.show();
}

unsigned long lastPatternChangeTime = 0;
int currentPatternIndex = 0;

void rgbLoop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastPatternChangeTime >= 1000)
  {
    lastPatternChangeTime = currentMillis;
    currentPatternIndex = (currentPatternIndex + 1) % 3;
    switch (currentPatternIndex)
    {
    case 0:
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      break;
    case 1:
      fill_solid(leds, NUM_LEDS, CRGB::Green);
      break;
    case 2:
      fill_solid(leds, NUM_LEDS, CRGB::Blue);
      break;
    }
    FastLED.show();
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

  switch (currentAnimation)
  {
  case DEBUG_MODE:
    rgbLoop();
    break;

  case SNAKE_MODE:
    snakeLedsOnMotion();
    break;

  case MOVIE_MODE:
    flashLedsOnMotion();
    break;
  }

  delay(1);
  // rainbow();
}
