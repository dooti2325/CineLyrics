#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>

void drawBackgroundEffect(const char* effectName, float beatPhase, float energy);
void applyCameraEffect(const char* effectName, float beatPhase, float energy, int& xOffset, int& yOffset);

#endif // EFFECTS_H
