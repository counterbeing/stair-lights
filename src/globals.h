// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

enum AnimationState
{
	DEBUG_MODE,
	SNAKE_MODE,
	MOVIE_MODE,
	WHITE_MODE
};

extern volatile AnimationState currentAnimation;

extern volatile bool animationInitialized;

void switchAnimation(AnimationState nextAnimation);

#endif