#ifndef FACE_H
#define FACE_H

#include <Arduino.h>

enum FaceAnim {
    ANIM_IDLE,
    ANIM_HAPPY,
    ANIM_LAUGH,
    ANIM_SAD,
    ANIM_LOVE,
    ANIM_SHOCK,
    ANIM_SLEEP,
    ANIM_WAKE,
    ANIM_ANGRY,
    ANIM_CURIOUS,
    ANIM_THINKING,
    ANIM_VIBE
};

void setupFace();
void updateFace();
void playFaceAnimation(FaceAnim anim);
void setMusicState(bool isPlaying, float bpm);

#endif // FACE_H
