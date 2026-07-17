#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>

void setAnimationState(const char* text, const char* nextText, const char* animType, const char* title, const char* artist, float bpm, float energy, int dur);
void updateAnimation();

#endif // ANIMATIONS_H
