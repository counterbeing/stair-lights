#include <Arduino.h>
#include "FastLED.h"
#include <ezButton.h>

#define LED_DATA_PIN 25
#define NUM_LEDS 2

CRGB leds[NUM_LEDS];

// set up push button with debounce
// #define BUTTON_PIN 2
ezButton button(23);
int lastState = LOW;
int currentState;
int currentAnimationIndex = 0;

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

void setup()
{
  Serial.begin(115200);
  Serial.println("Happy camping!");
  button.setDebounceTime(100);

  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  bootPattern();
}

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

// A function that moves a rainbow from one end of the strip to the other
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
  button.loop();
  delay(1);
  rainbow();
}
