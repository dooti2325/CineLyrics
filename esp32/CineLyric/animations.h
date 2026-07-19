#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>

struct AnimPacket {
    char lyric[128];
    char nextLyric[128];
    char animType[32];
    char title[128];
    char artist[128];
    float bpm;
    float energy;
    int duration;
    float beatStrength;
    float bass;
    char emotion[16];
    char scene[16];
    char secondary[16];
    char font[16];
    int x;
    int y;
    bool shake;
    bool invert;
    char particles[16];
};

void setAnimationState(const AnimPacket& pkt);
void updateAnimation();
void cycleAnimation();

#endif // ANIMATIONS_H
