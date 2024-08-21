// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H
#include <string> // Add this line

enum AnimationState
{
	DEBUG_MODE,
	SNAKE_MODE,
	MOVIE_MODE,
	WHITE_MODE,
	NIGHT_VISION,
	TWINKLE_TWANKLE,
	FUZZ_WAVE
};

struct AnimationConfig
{
	const char *name;
	const char *uniqueId;
	enum AnimationState state;

	std::string buildTriggerTopic() const;
	std::string buildStateTopic() const;
};

extern const AnimationConfig animations[];

extern const int animationSize;

extern volatile AnimationState currentAnimation;

extern volatile bool animationInitialized;

void switchAnimation(AnimationState nextAnimation);

#endif