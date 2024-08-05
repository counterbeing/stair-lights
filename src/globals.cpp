// globals.cpp
#include "globals.h"

volatile AnimationState currentAnimation = WHITE_MODE; // Definition
volatile bool animationInitialized = false;

void switchAnimation(AnimationState nextAnimation)
{
	animationInitialized = false;
	currentAnimation = nextAnimation;
}