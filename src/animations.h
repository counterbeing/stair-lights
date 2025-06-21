#pragma once

#include "FastLED.h"
#include "globals.h"
#include "config.h"

// Function declarations for animations
void rainbow();
void bootPattern();
void sinColorFade();
void nightVision();
void whiteWhenMotionDetected();
void snakeLedsOnMotion();
void rgbLoop();
void arrows();
void twinkleTwankle();
void fuzzWave();
void energyBattle();
void bouncingBall();

// Helper function declarations
int countUPAndDown(int min, int max, int step);
float sinWave();
void incrementColor();
void pushRow();
bool weightedOnOrOff(int ledNumber);
void addLEDRange(int start, int end, CRGB color);
void pushArray(int arr[], int size);
void pushArrayReverse(int arr[], int size);
void prefixArray(int existingArray[], int newValues[], int newSize);

// Declare external variables
extern CRGB leds[NUM_LEDS];