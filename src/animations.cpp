#include "animations.h"
#include "mqtt_functions.h"

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

void bootPattern()
{
	fill_solid(leds, NUM_LEDS, CRGB::Red);
	FastLED.show();
	delay(100);
	fill_solid(leds, NUM_LEDS, CRGB::Green);
	FastLED.show();
	delay(100);
	fill_solid(leds, NUM_LEDS, CRGB::Blue);
	FastLED.show();
	delay(100);
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

float sinWave()
{
	const unsigned long period = 20000; // 10 seconds in milliseconds
	const float pi = 3.14159265359;

	// Calculate the current position in the wave cycle
	float position = (millis() % period) / static_cast<float>(period);

	// Calculate the sine value and normalize it to 0-1 range
	return (sin(position * 2 * pi) + 1) / 2;
}

// color fade
void incrementColor()
{
	static int fade = 0;

	fade = (fade + 1) % 256;
	fill_solid(leds, NUM_LEDS, CHSV(fade, 255, 255));
	FastLED.show();
}

void sinColorFade()
{
	static unsigned long lastColorChange = 0;
	unsigned long currentMillis = millis();

	// Get the sine wave value (0 to 1)
	float waveValue = sinWave();

	// Map the sine wave value to a delay between 10ms and 100ms
	unsigned long delayTime = map(waveValue * 1000, 0, 1000, 10, 500);

	if (currentMillis - lastColorChange >= delayTime)
	{
		incrementColor();
		lastColorChange = currentMillis;
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
				hue = random(256);						 // Initialize hue with a random value
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
			leds[i] = CHSV(hue[i], 255, 0);	 // Start with LEDs off
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
			leds[i] = CHSV(hue[i], 255, brightness[i]);						// Update the LED with new brightness
		}
	}
	FastLED.show();
}

bool weightedOnOrOff(int ledNumber)
{
	double status = ((double)ledNumber / NUM_LEDS - 0.2);
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
	static int hue[NUM_LEDS];
	unsigned long currentMillis = millis();
	static boolean motionDetected = false;

	if (!animationInitialized)
	{
		FastLED.setBrightness(255);
		for (int i = 0; i < NUM_LEDS; i++)
		{
			leds[i] = CHSV(random(256), 255, 255);
		}
		animationInitialized = true;
	}

	if (digitalRead(MOTION_SENSOR_PIN) == HIGH)
	{
		for (int i = 0; i < NUM_LEDS; i++)
		{
			if (weightedOnOrOff(i))
			{
				leds[i] = CHSV(random(256), 255, 255);
			}
		}
	}

	if (digitalRead(MOTION_SENSOR_PIN_2) == HIGH)
	{

		sendDebugMessage("motion detected");

		for (int i = NUM_LEDS - 1; i >= 0; i--)
		{
			if (weightedOnOrOff(NUM_LEDS - i))
			{
				leds[i] = CHSV(random(256), 255, 255);
			}
		}
	}

	FastLED.show();
}

void addLEDRange(int start, int end, CRGB color)
{
	for (int i = start; i <= end; i++)
	{
		leds[i] = leds[i] + color; // Set the LED to the specified color
	}
}

void pushArray(int arr[], int size)
{
	for (int i = size - 1; i > 0; i--)
	{
		arr[i] = arr[i - 1]; // Shift elements down one position
	}
	arr[0] = 0; // Insert 0 at the beginning after shifting
}

void pushArrayReverse(int arr[], int size)
{
	for (int i = 0; i < size - 1; i++)
	{
		arr[i] = arr[i + 1]; // Shift elements up one position
	}
	arr[size - 1] = 0; // Insert 0 at the end after shifting
}

void prefixArray(int existingArray[], int newValues[], int newSize)
{
	for (int i = 0; i < newSize; i++)
	{
		existingArray[i] = newValues[i];
	}
}

void energyBattle()
{
	static const int waveLength = 5;
	static const int virtualRows = NUM_ROWS + (waveLength * 2);
	static int waveIndex[virtualRows] = {0};				// Initialize all elements to 0
	static int waveIndexReverse[virtualRows] = {0}; // Initialize all elements to 0 for reverse wave
	static unsigned long lastWaveTime = 0;
	static unsigned long lastReverseWaveTime = 0;
	static unsigned long lastAnimationTime = 0;

	if ((millis() - lastAnimationTime) > 200)
	{
		lastAnimationTime = millis();
		fill_solid(leds, NUM_LEDS, CRGB::Black);
		pushArray(waveIndex, virtualRows);
		pushArrayReverse(waveIndexReverse, virtualRows);

		if (digitalRead(MOTION_SENSOR_PIN) == HIGH && (millis() - lastWaveTime) > 1000)
		{
			lastWaveTime = millis(); // Update the wave timer
			int waveValues[5] = {1, 2, 3, 4, 5};

			prefixArray(waveIndex, waveValues, 5); // Now passing the size explicitly
		}

		if (digitalRead(MOTION_SENSOR_PIN_2) == HIGH && (millis() - lastReverseWaveTime) > 100)
		{ // Different timing for reverse wave
			lastReverseWaveTime = millis();
			int reverseWaveValues[5] = {1, 2, 3, 4, 5}; // Example values
			waveIndexReverse[virtualRows - 1] = 5;			// Start the wave at the end
			for (int j = 1; j < 5; j++)
			{
				waveIndexReverse[virtualRows - 1 - j] = reverseWaveValues[j];
			}
		}

		for (int i = 0; i < NUM_ROWS; i++)
		{
			int indexValue = waveIndex[i + waveLength];								// Access the adjusted index for visible rows from the forward wave
			int reverseIndexValue = waveIndexReverse[i + waveLength]; // Access the adjusted index for visible rows from the reverse wave

			CRGB forwardColor = CRGB::Black;
			CRGB reverseColor = CRGB::Black;

			if (indexValue > 0)
			{
				forwardColor = CRGB::Lime;
				forwardColor.nscale8(indexValue * 51); // Scale down based on indexValue
			}

			if (reverseIndexValue > 0)
			{
				reverseColor = CRGB::Coral;
				reverseColor.nscale8(reverseIndexValue * 51); // Scale down based on reverseIndexValue
			}

			// Blend colors where both waves exist
			CRGB blendedColor = blend(forwardColor, reverseColor, 128); // Blend equally for simplicity, adjust as needed
			addLEDRange(i * NUM_COLS, (i + 1) * NUM_COLS - 1, blendedColor);
		}

		FastLED.show(); // Show the updated LED state

		// Debug output to see how the waveIndex changes over time
		String waveIndexString;
		for (int i = 0; i < virtualRows; i++)
		{
			waveIndexString += String(waveIndex[i]);
			if (i < virtualRows - 1)
			{
				waveIndexString += ",";
			}
		}
		// sendDebugMessage(waveIndexString.c_str());
	}
}

void testingMotion()
{
	// int out = analogRead(MOTION_SENSOR_PIN_2);
	int out = digitalRead(MOTION_SENSOR_PIN_2);
	sendDebugMessage(String(out).c_str());
}

void bouncingBall()
{
	static unsigned long lastUpdate = 0;
	static float ballX = NUM_COLS / 2.0;
	static float ballY = NUM_ROWS / 2.0;
	static float velocityX = 0.0;
	static float velocityY = 0.0;
	static uint8_t hue = 0;
	static bool initialized = false;
	
	const unsigned long QUARTER_SECOND = 250;
	const float MOMENTUM_FACTOR = 0.7;
	const float RANDOM_FACTOR = 0.3;
	const float SPEED = 1.0;
	
	if (!initialized)
	{
		// Initialize with random direction
		velocityX = (random(2) == 0 ? -SPEED : SPEED);
		velocityY = (random(2) == 0 ? -SPEED : SPEED);
		initialized = true;
	}
	
	// Update every quarter second
	if (millis() - lastUpdate >= QUARTER_SECOND)
	{
		// Clear all LEDs
		fill_solid(leds, NUM_LEDS, CRGB::Black);
		
		// Continuously change color
		hue += 3;
		
		// Calculate new velocity with momentum and randomness
		float newVelX = velocityX * MOMENTUM_FACTOR;
		float newVelY = velocityY * MOMENTUM_FACTOR;
		
		// Add random component
		float randomX = (random(201) - 100) / 100.0 * RANDOM_FACTOR * SPEED;
		float randomY = (random(201) - 100) / 100.0 * RANDOM_FACTOR * SPEED;
		
		newVelX += randomX;
		newVelY += randomY;
		
		// Normalize speed to maintain consistent movement
		float magnitude = sqrt(newVelX * newVelX + newVelY * newVelY);
		if (magnitude > 0)
		{
			newVelX = (newVelX / magnitude) * SPEED;
			newVelY = (newVelY / magnitude) * SPEED;
		}
		
		velocityX = newVelX;
		velocityY = newVelY;
		
		// Update position
		ballX += velocityX;
		ballY += velocityY;
		
		// Bounce off walls (pong-style)
		if (ballX <= 0)
		{
			ballX = 0;
			velocityX = abs(velocityX);
		}
		else if (ballX >= NUM_COLS - 1)
		{
			ballX = NUM_COLS - 1;
			velocityX = -abs(velocityX);
		}
		
		if (ballY <= 0)
		{
			ballY = 0;
			velocityY = abs(velocityY);
		}
		else if (ballY >= NUM_ROWS - 1)
		{
			ballY = NUM_ROWS - 1;
			velocityY = -abs(velocityY);
		}
		
		// Convert 2D coordinates to LED index
		int x = (int)round(ballX);
		int y = (int)round(ballY);
		
		// Calculate LED index based on matrix layout
		int ledIndex;
		if (y % 2 == 0)
		{
			// Even rows: left to right
			ledIndex = y * NUM_COLS + x;
		}
		else
		{
			// Odd rows: right to left (serpentine)
			ledIndex = y * NUM_COLS + (NUM_COLS - 1 - x);
		}
		
		// Light up the ball with current color
		if (ledIndex >= 0 && ledIndex < NUM_LEDS)
		{
			leds[ledIndex] = CHSV(hue, 255, 255);
		}
		
		FastLED.show();
		lastUpdate = millis();
	}
}