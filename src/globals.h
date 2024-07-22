// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

enum AnimationState
{
	DEBUG_MODE,
	SNAKE_MODE,
	MOVIE_MODE
};

extern volatile AnimationState currentAnimation;

#endif