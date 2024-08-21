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

int currentBrightness = 0;
unsigned long lastMotionDetectedTime = 0;
unsigned long lastDimmingTime = 0;

void nightVision()
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

void whiteWhenMotionDetected()
{
  unsigned long currentMillis = millis();

  static boolean motionDetected = false;
  static boolean fadingOut = false;
  static unsigned long lastMotionTime = 0;
  static int fade = 0;

  if (digitalRead(MOTION_SENSOR_PIN) == HIGH)
  {
    if (!motionDetected)
    { // Only reset these if motion was previously not detected
      motionDetected = true;
      fadingOut = false;
      lastMotionTime = currentMillis;
      fade = 0; // Start fading in from dark
    }
  }
  else
  {
    if (currentMillis - lastMotionTime > 10000)
    {
      motionDetected = false;
      fadingOut = true;
    }
  }

  if (motionDetected && fade < 255)
  {
    fade += 1;
    fill_solid(leds, NUM_LEDS, CRGB(fade, fade, fade));
    FastLED.show();
  }
  else if (fadingOut)
  {
    if (fade > 0)
    {
      fade -= 1;
      fill_solid(leds, NUM_LEDS, CRGB(fade, fade, fade));
      FastLED.show();
    }
    else
    {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      fadingOut = false; // Stop fading out after reaching black
    }
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
    if (!motionDetected || (currentMillis - lastMotionDetectedTime) > 100)
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

void pushRow()
{
  for (int i = NUM_LEDS - 1; i >= 7; i--)
  {
    leds[i] = leds[i - 7];
  }
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB::Black;
  }
}

void arrows()
{
  static int i = 0;

  if (millis() - lastShiftTime > 200)
  {

    lastShiftTime = millis();
    i++;
    pushRow();
    switch (i)
    {
    case 1:
      leds[3] = CRGB::Red;
      break;

    case 2:
      leds[2] = CRGB::Red;
      leds[3] = CRGB::Purple;
      leds[4] = CRGB::Red;
      break;

    case 3:
      leds[1] = CRGB::Red;
      leds[2] = CRGB::Purple;
      leds[3] = CRGB::Purple;
      leds[4] = CRGB::Purple;
      leds[5] = CRGB::Red;
      break;

    case 4:
      leds[0] = CRGB::Red;
      leds[1] = CRGB::Purple;
      leds[2] = CRGB::Purple;
      leds[3] = CRGB::Purple;
      leds[4] = CRGB::Purple;
      leds[5] = CRGB::Purple;
      leds[6] = CRGB::Red;
      break;

    default:
      i = 0;
      break;
    }

    FastLED.show();
  }
}

void twinkleTwankle()
{
  static unsigned long lastMotionDetectedTime = 0;
  static int glowDelay[NUM_LEDS];
  static int hue[NUM_LEDS];
  static int brightness[NUM_LEDS];
  static long glowStartTime[NUM_LEDS];

  if (!animationInitialized)
  {
    FastLED.setBrightness(255);
    for (int i = 0; i < NUM_LEDS; i++)
    {
      brightness[i] = 0;
      hue[i] = random(256);
      leds[i] = CHSV(hue[i], 255, 0);  // Start with LEDs off
      glowDelay[i] = random(50, 1000); // Shorter delay for faster activation
      glowStartTime[i] = millis() + random(5000);
    }
    animationInitialized = true;
  }

  unsigned long currentTime = millis();
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (currentTime > (glowStartTime[i] + glowDelay[i]))
    {
      glowStartTime[i] = currentTime + glowDelay[i];
      brightness[i] = constrain(brightness[i] + 1, 0, 255); // Increase brightness more quickly
      leds[i] = CHSV(hue[i], 255, brightness[i]);           // Update the LED with new brightness
    }
  }
  FastLED.show();
}

bool weightedOnOrOff(int ledNumber)
{
  double status = (double)ledNumber / NUM_LEDS;
  if (random(10000) < (status * 1000))
  {
    return true;
  }
  else
  {
    return false;
  }
}

void fuzzWave()
{
  static unsigned long lastMotionDetectedTime = 0;
  static int glowDelay[NUM_LEDS];
  static int hue[NUM_LEDS];
  unsigned long currentMillis = millis();
  static boolean motionDetected = false;

  if (!animationInitialized)
  {
    FastLED.setBrightness(255);
    for (int i = 0; i < NUM_LEDS; i++)
    {
      hue[i] = random(256);
      leds[i] = CHSV(hue[i], 255, 255); // Start with LEDs off
      glowDelay[i] = random(50, 1000);  // Shorter delay for faster activation
    }
    animationInitialized = true;
  }

  if (digitalRead(MOTION_SENSOR_PIN) == HIGH)
  {
    if (!motionDetected || (currentMillis - lastMotionDetectedTime) > 100)
    {
      sendDebugMessage("motion detected");
      motionDetected = true;
      lastMotionDetectedTime = currentMillis;

      for (int i = 0; i < NUM_LEDS; i++)
      {
        if (weightedOnOrOff(i))
        {
          leds[i] = CHSV(random(256), 255, 255);
        }
      }
    }
  }

  FastLED.show();
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

  case WHITE_MODE:
    whiteWhenMotionDetected();
    break;
  }

  // This has to be added because the esp32 is too fast
  delay(1);
}
