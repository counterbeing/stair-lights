// globals.cpp
#include "globals.h"

volatile AnimationState currentAnimation = FUZZ_WAVE; // Definition
volatile bool animationInitialized = false;

void switchAnimation(AnimationState nextAnimation)
{
	animationInitialized = false;
	currentAnimation = nextAnimation;
}

const AnimationConfig animations[] = {
		{"White Mode", "white_mode", WHITE_MODE},
		{"Debug Mode", "debug_mode", DEBUG_MODE},
		{"Snake Mode", "snake_mode", SNAKE_MODE},
		{"Night Vision", "night_vision", NIGHT_VISION},
		{"Movie Mode", "movie_mode", MOVIE_MODE},
		{"Fuzz Wave", "fuzz_wave", FUZZ_WAVE},
		{"Twinkle Twankle", "twinkle_twankle", TWINKLE_TWANKLE}};

const int animationSize = sizeof(animations) / sizeof(animations[0]);

std::string AnimationConfig::buildTriggerTopic() const
{
	return "home/stair-balls/" + std::string(uniqueId) + "/trigger";
}

std::string AnimationConfig::buildStateTopic() const
{
	return "home/stair-balls/" + std::string(uniqueId) + "/state";
}