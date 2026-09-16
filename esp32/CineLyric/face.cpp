#include "face.h"
#include "display.h"
#include <math.h>

struct EyeParams {
    float x, y, w, h, radius, topAngle, bottomAngle, pupilX, pupilY, pupilSize;
};

EyeParams cLeft, cRight; // Current
EyeParams tLeft, tRight; // Target
EyeParams sLeft, sRight; // Start

FaceAnim currentAnim = ANIM_IDLE;
int animStep = 0;
unsigned long stepStartTime = 0;
int stepDuration = 0;
bool isSequencePlaying = false;

bool isMusicPlaying = false;
float currentMusicBPM = 120.0;

// Timers
unsigned long nextMicroMoveTime = 0;
unsigned long nextBlinkTime = 0;

// Blink
float blinkFactor = 0.0;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
float currentBlinkDuration = 150.0;

const float defaultY = 26.0;
const float defaultLeftX = 40.0;
const float defaultRightX = 88.0;
const float defaultW = 34.0;
const float defaultH = 34.0;
const float defaultR = 12.0;

void setDefault(EyeParams& e, float x) {
    e.x = x; e.y = defaultY;
    e.w = defaultW; e.h = defaultH; e.radius = defaultR;
    e.topAngle = 0; e.bottomAngle = 0;
    e.pupilX = 0; e.pupilY = 0; 
    e.pupilSize = 10.0; // Scaled cartoon pupil
}

void triggerBlink(float duration) {
    if (!isBlinking) {
        isBlinking = true;
        blinkStartTime = millis();
        currentBlinkDuration = duration;
    }
}

void startStep(int duration) {
    sLeft = cLeft; sRight = cRight;
    stepStartTime = millis();
    stepDuration = duration;
}

void executeStep(FaceAnim anim, int step) {
    setDefault(tLeft, defaultLeftX);
    setDefault(tRight, defaultRightX);
    
    switch (anim) {
        case ANIM_IDLE:
            isSequencePlaying = false;
            break;
            
        case ANIM_HAPPY:
            if (step == 0) {
                triggerBlink(150);
                startStep(150);
            } else if (step == 1) {
                tLeft.topAngle = 20; tRight.topAngle = -20;
                tLeft.bottomAngle = 20; tRight.bottomAngle = -20;
                tLeft.y -= 10; tRight.y -= 10;
                startStep(200);
            } else if (step == 2) {
                tLeft.topAngle = 20; tRight.topAngle = -20;
                tLeft.bottomAngle = 20; tRight.bottomAngle = -20;
                tLeft.y -= 16; tRight.y -= 16; // bounce up
                startStep(100);
            } else if (step == 3) {
                tLeft.topAngle = 20; tRight.topAngle = -20;
                tLeft.bottomAngle = 20; tRight.bottomAngle = -20;
                tLeft.y -= 10; tRight.y -= 10; // bounce down
                startStep(100);
            } else if (step == 4) {
                tLeft.topAngle = 20; tRight.topAngle = -20;
                tLeft.bottomAngle = 20; tRight.bottomAngle = -20;
                tLeft.y -= 10; tRight.y -= 10;
                startStep(2000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_LAUGH:
            if (step == 0) {
                tLeft.h = 16; tRight.h = 16;
                tLeft.topAngle = 25; tRight.topAngle = -25;
                tLeft.bottomAngle = 25; tRight.bottomAngle = -25;
                startStep(150);
            } else if (step == 1) {
                tLeft.h = 16; tRight.h = 16;
                tLeft.topAngle = 25; tRight.topAngle = -25;
                tLeft.bottomAngle = 25; tRight.bottomAngle = -25;
                tLeft.y -= 12; tRight.y -= 12;
                startStep(100);
            } else if (step == 2) {
                tLeft.h = 16; tRight.h = 16;
                tLeft.topAngle = 25; tRight.topAngle = -25;
                tLeft.bottomAngle = 25; tRight.bottomAngle = -25;
                tLeft.y += 6; tRight.y += 6;
                startStep(100);
            } else if (step == 3) {
                tLeft.h = 16; tRight.h = 16;
                tLeft.topAngle = 25; tRight.topAngle = -25;
                tLeft.bottomAngle = 25; tRight.bottomAngle = -25;
                tLeft.y -= 12; tRight.y -= 12;
                startStep(100);
            } else if (step == 4) {
                tLeft.h = 16; tRight.h = 16;
                tLeft.topAngle = 25; tRight.topAngle = -25;
                tLeft.bottomAngle = 25; tRight.bottomAngle = -25;
                tLeft.y += 6; tRight.y += 6;
                startStep(800);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_LOVE:
            if (step == 0) {
                triggerBlink(150);
                startStep(150);
            } else if (step == 1) {
                tLeft.h += 12; tRight.h += 12;
                tLeft.w -= 6; tRight.w -= 6; // tall and narrow
                tLeft.y -= 6; tRight.y -= 6;
                startStep(300);
            } else if (step == 2) {
                tLeft.h += 12; tRight.h += 12;
                tLeft.w -= 6; tRight.w -= 6;
                tLeft.y -= 6; tRight.y -= 6;
                startStep(2000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_SAD:
            if (step == 0) {
                tLeft.y += 10; tRight.y += 10;
                tLeft.topAngle = -35; tRight.topAngle = 35; 
                tLeft.bottomAngle = -15; tRight.bottomAngle = 15;
                tLeft.x -= 3; tRight.x += 3; // spread
                startStep(400);
            } else if (step == 1) {
                tLeft.y += 10; tRight.y += 10;
                tLeft.topAngle = -35; tRight.topAngle = 35;
                tLeft.bottomAngle = -15; tRight.bottomAngle = 15;
                tLeft.x -= 3; tRight.x += 3;
                triggerBlink(400);
                startStep(2000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_ANGRY:
            if (step == 0) {
                tLeft.topAngle = 35; tRight.topAngle = -35;
                tLeft.bottomAngle = 10; tRight.bottomAngle = -10;
                tLeft.y += 4; tRight.y += 4;
                tLeft.x += 6; tRight.x -= 6; // pushed together
                startStep(200);
            } else if (step == 1) {
                tLeft.topAngle = 35; tRight.topAngle = -35;
                tLeft.bottomAngle = 10; tRight.bottomAngle = -10;
                tLeft.y += 4; tRight.y += 4;
                tLeft.x += 6; tRight.x -= 6;
                triggerBlink(120);
                startStep(150);
            } else if (step == 2) {
                tLeft.topAngle = 35; tRight.topAngle = -35;
                tLeft.bottomAngle = 10; tRight.bottomAngle = -10;
                tLeft.y += 4; tRight.y += 4;
                tLeft.x += 3; tRight.x -= 9;
                startStep(50);
            } else if (step == 3) {
                tLeft.topAngle = 35; tRight.topAngle = -35;
                tLeft.bottomAngle = 10; tRight.bottomAngle = -10;
                tLeft.y += 4; tRight.y += 4;
                tLeft.x += 9; tRight.x -= 3;
                startStep(50);
            } else if (step == 4) {
                tLeft.topAngle = 35; tRight.topAngle = -35;
                tLeft.bottomAngle = 10; tRight.bottomAngle = -10;
                tLeft.y += 4; tRight.y += 4;
                tLeft.x += 6; tRight.x -= 6;
                startStep(2000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_SHOCK:
            if (step == 0) {
                tLeft.w += 6; tLeft.h += 12; tLeft.radius = 24;
                tRight.w += 6; tRight.h += 12; tRight.radius = 24;
                tLeft.pupilSize = 6.0; tRight.pupilSize = 6.0; // lens shrinks rapidly
                tLeft.x -= 6; tRight.x += 6; // pop outwards
                startStep(150);
            } else if (step == 1) {
                tLeft.w += 6; tLeft.h += 12; tLeft.radius = 24;
                tRight.w += 6; tRight.h += 12; tRight.radius = 24;
                tLeft.pupilSize = 6.0; tRight.pupilSize = 6.0;
                tLeft.x -= 6; tRight.x += 6;
                startStep(2000);
            } else {
                triggerBlink(150);
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_SLEEP:
            if (step == 0) {
                tLeft.h = 24; tRight.h = 24;
                tLeft.y += 10; tRight.y += 10;
                tLeft.topAngle = -15; tRight.topAngle = 15; // droop
                startStep(400); // half
            } else if (step == 1) {
                tLeft.h = 6; tRight.h = 6;
                tLeft.y += 20; tRight.y += 20;
                tLeft.topAngle = -15; tRight.topAngle = 15;
                startStep(600); // closed
            } else if (step == 2) {
                tLeft.h = 6; tRight.h = 6;
                tLeft.y += 20; tRight.y += 20;
                tLeft.topAngle = -15; tRight.topAngle = 15;
                startStep(100000); // Zzz
            }
            break;
            
        case ANIM_WAKE:
            if (step == 0) {
                setDefault(tLeft, defaultLeftX);
                setDefault(tRight, defaultRightX);
                tLeft.h = 6; tRight.h = 6;
                tLeft.y += 20; tRight.y += 20;
                startStep(1);
            } else if (step == 1) {
                tLeft.h = 24; tRight.h = 24;
                tLeft.y += 10; tRight.y += 10;
                startStep(300);
            } else if (step == 2) {
                tLeft.h = defaultH; tRight.h = 24; // One eye open
                startStep(400);
            } else if (step == 3) {
                tLeft.h = defaultH; tRight.h = defaultH;
                tLeft.y -= 6; tRight.y -= 6;
                tLeft.topAngle = 15; tRight.topAngle = -15; // little happy lift
                startStep(300);
            } else if (step == 4) {
                tLeft.y -= 6; tRight.y -= 6;
                tLeft.topAngle = 15; tRight.topAngle = -15;
                startStep(1500);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_CURIOUS:
            if (step == 0) {
                tLeft.x -= 12; tRight.x -= 12; 
                startStep(300);
            } else if (step == 1) {
                tLeft.x += 12; tRight.x += 12; 
                startStep(400);
            } else if (step == 2) {
                tLeft.x = defaultLeftX; tRight.x = defaultRightX; 
                tLeft.y -= 10; tRight.y -= 2; 
                tLeft.topAngle = 20; tLeft.bottomAngle = 20; 
                tRight.topAngle = -10; tRight.bottomAngle = -10;
                startStep(300);
            } else if (step == 3) {
                tLeft.y -= 10; tRight.y -= 2;
                tLeft.topAngle = 20; tLeft.bottomAngle = 20;
                tRight.topAngle = -10; tRight.bottomAngle = -10;
                startStep(1200);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_THINKING:
            if (step == 0) {
                tLeft.y -= 10; tRight.y -= 10; // Look up
                tLeft.topAngle = 15; tRight.topAngle = 15; 
                tLeft.bottomAngle = 15; tRight.bottomAngle = 15;
                startStep(300);
            } else if (step == 1) {
                tLeft.y -= 10; tRight.y -= 10;
                tLeft.topAngle = 15; tRight.topAngle = 15;
                tLeft.bottomAngle = 15; tRight.bottomAngle = 15;
                startStep(600);
            } else if (step == 2) {
                tLeft.y -= 10; tRight.y -= 10;
                tLeft.topAngle = 5; tRight.topAngle = 5;
                tLeft.bottomAngle = 5; tRight.bottomAngle = 5;
                tLeft.x -= 6; tRight.x -= 6;
                startStep(300);
            } else if (step == 3) {
                tLeft.y -= 10; tRight.y -= 10;
                tLeft.topAngle = 15; tRight.topAngle = 15;
                tLeft.bottomAngle = 15; tRight.bottomAngle = 15;
                tLeft.x += 3; tRight.x += 3;
                startStep(300);
            } else if (step == 4) {
                triggerBlink(150);
                startStep(150);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;
            
        case ANIM_VIBE:
            if (step == 0) {
                tLeft.y -= 4; tRight.y -= 4;
                tLeft.topAngle = 10; tRight.topAngle = -10;
                startStep(200);
            } else if (step == 1) {
                tLeft.y += 4; tRight.y += 4;
                tLeft.topAngle = -10; tRight.topAngle = 10;
                startStep(200);
            } else if (step == 2) {
                if (isMusicPlaying) {
                    animStep = -1; // loop
                    startStep(10);
                } else {
                    playFaceAnimation(ANIM_IDLE);
                }
            }
            break;
    }
}

void playFaceAnimation(FaceAnim anim) {
    currentAnim = anim;
    animStep = 0;
    isSequencePlaying = true;
    executeStep(currentAnim, animStep);
}

void setMusicState(bool isPlaying, float bpm) {
    if (isPlaying && !isMusicPlaying) {
        playFaceAnimation(ANIM_HAPPY); // Reaction when song plays
    } else if (!isPlaying && isMusicPlaying) {
        playFaceAnimation(ANIM_SAD); // Reaction when song pauses
    }
    isMusicPlaying = isPlaying;
    if (bpm > 0) {
        currentMusicBPM = bpm;
    }
}

void setupFace() {
    setDefault(cLeft, defaultLeftX);
    setDefault(cRight, defaultRightX);
    playFaceAnimation(ANIM_IDLE);
    nextBlinkTime = millis() + random(2000, 5000);
    nextMicroMoveTime = millis() + random(1000, 4000);
}

// Easing & Interpolation
float easeInOutCubic(float t) {
    return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

float lerpVal(float s, float t, float factor) {
    return s + (t - s) * factor;
}

void interpolateEye(EyeParams& current, EyeParams& start, EyeParams& target, float t) {
    current.x = lerpVal(start.x, target.x, t);
    current.y = lerpVal(start.y, target.y, t);
    current.w = lerpVal(start.w, target.w, t);
    current.h = lerpVal(start.h, target.h, t);
    current.radius = lerpVal(start.radius, target.radius, t);
    current.topAngle = lerpVal(start.topAngle, target.topAngle, t);
    current.bottomAngle = lerpVal(start.bottomAngle, target.bottomAngle, t);
    current.pupilX = lerpVal(start.pupilX, target.pupilX, t);
    current.pupilY = lerpVal(start.pupilY, target.pupilY, t);
    current.pupilSize = lerpVal(start.pupilSize, target.pupilSize, t);
}

void drawMask(float cx, float cy, float w, float h, float angle, bool isTop) {
    if (abs(angle) < 0.1) return;
    
    u8g2.setDrawColor(0);
    float rad = angle * PI / 180.0;
    float maskY = isTop ? (cy - h/2.0 + 4.0) : (cy + h/2.0 - 4.0);
    
    int p1x = cx - w;
    int p2x = cx + w;
    int p1y = maskY - tan(rad) * w;
    int p2y = maskY + tan(rad) * w;
    int p3y = isTop ? (cy - h) : (cy + h);
    
    u8g2.drawTriangle(p1x, p1y, p2x, p2y, p1x, p3y);
    u8g2.drawTriangle(p2x, p2y, p1x, p3y, p2x, p3y);
    u8g2.setDrawColor(1);
}

void drawEyeShape(EyeParams& eye, float blinkAmt, float breathScale, float vibeYOffset = 0.0) {
    float drawH = eye.h * breathScale * (1.0 - blinkAmt);
    if (drawH < 2.0) drawH = 2.0;
    float drawW = eye.w * breathScale;
    
    float drawY = eye.y + vibeYOffset;
    if (blinkAmt > 0) {
        drawY += (eye.h * blinkAmt * 0.5); 
    }
    
    u8g2.setDrawColor(1);
    // Draw white sclera
    u8g2.drawRBox(eye.x - drawW/2.0, drawY - drawH/2.0, drawW, drawH, eye.radius);
    
    // Draw pupil & cute cartoon catchlight highlight
    if (eye.pupilSize > 0.1 && blinkAmt < 0.5) {
        u8g2.setDrawColor(0);
        float pX = eye.x + eye.pupilX;
        float pY = drawY + eye.pupilY;
        u8g2.drawDisc(pX, pY, eye.pupilSize);
        
        // Specular catchlight sparkle (Disney/Pixar style highlight)
        u8g2.setDrawColor(1);
        u8g2.drawDisc(pX - eye.pupilSize * 0.35, pY - eye.pupilSize * 0.35, max(1.5f, eye.pupilSize * 0.30f));
        u8g2.drawPixel(pX + eye.pupilSize * 0.3, pY + eye.pupilSize * 0.3);
    }
    
    if (blinkAmt < 0.8) {
        drawMask(eye.x, drawY, drawW, drawH, eye.topAngle, true);
        drawMask(eye.x, drawY, drawW, drawH, eye.bottomAngle, false);
    }
}

void drawEyebrow(float x, float y, float w, float angle, float arch) {
    u8g2.setDrawColor(1);
    float rad = angle * PI / 180.0;
    int halfW = (int)(w / 2.0);
    
    for (int i = -halfW; i <= halfW; i++) {
        float norm = (float)i / (float)halfW;
        float curveY = -(1.0f - norm * norm) * arch + i * tan(rad);
        int px = (int)(x + i);
        int py = (int)(y + curveY);
        u8g2.drawPixel(px, py);
        u8g2.drawPixel(px, py - 1);
        if (abs(norm) < 0.6f) {
            u8g2.drawPixel(px, py - 2); // Thicker center arch
        }
    }
}

void drawCartoonEyebrows(FaceAnim anim, float vibeYOffset, unsigned long currentMillis) {
    float leftY = cLeft.y - (cLeft.h / 2.0) - 4.0 + vibeYOffset;
    float rightY = cRight.y - (cRight.h / 2.0) - 4.0 + vibeYOffset;

    switch (anim) {
        case ANIM_HAPPY:
        case ANIM_LAUGH:
            drawEyebrow(cLeft.x, leftY - 4, 22, -6, 5);
            drawEyebrow(cRight.x, rightY - 4, 22, 6, 5);
            break;
        case ANIM_SAD:
            // Tilted inward and up / \
            drawEyebrow(cLeft.x, leftY - 2, 20, 22, 1);
            drawEyebrow(cRight.x, rightY - 2, 20, -22, 1);
            break;
        case ANIM_ANGRY:
            // Fierce V-brow \ /
            drawEyebrow(cLeft.x, leftY + 1, 24, -26, 1);
            drawEyebrow(cRight.x, rightY + 1, 24, 26, 1);
            break;
        case ANIM_CURIOUS:
            // Asymmetrical: Left high arch, Right flat
            drawEyebrow(cLeft.x, leftY - 7, 22, -8, 6);
            drawEyebrow(cRight.x, rightY, 20, 4, 1);
            break;
        case ANIM_SHOCK:
            // High floating arches
            drawEyebrow(cLeft.x, leftY - 7, 24, 0, 6);
            drawEyebrow(cRight.x, rightY - 7, 24, 0, 6);
            break;
        case ANIM_THINKING:
            drawEyebrow(cLeft.x, leftY - 1, 20, 12, 1);
            drawEyebrow(cRight.x, rightY - 6, 24, -12, 5);
            break;
        case ANIM_LOVE:
            drawEyebrow(cLeft.x, leftY - 4, 22, -4, 4);
            drawEyebrow(cRight.x, rightY - 4, 22, 4, 4);
            break;
        case ANIM_VIBE: {
            float tilt = sin(currentMillis / 180.0) * 12.0;
            drawEyebrow(cLeft.x, leftY - 3, 22, tilt, 4);
            drawEyebrow(cRight.x, rightY - 3, 22, tilt, 4);
            break;
        }
        case ANIM_SLEEP:
            drawEyebrow(cLeft.x, leftY - 1, 20, 0, 2);
            drawEyebrow(cRight.x, rightY - 1, 20, 0, 2);
            break;
        case ANIM_IDLE:
        default:
            drawEyebrow(cLeft.x, leftY - 2, 22, -2, 3);
            drawEyebrow(cRight.x, rightY - 2, 22, 2, 3);
            break;
    }
}

void drawCartoonMouth(FaceAnim anim, float breathScale, float vibeYOffset, unsigned long currentMillis) {
    u8g2.setDrawColor(1);
    int cx = 64;
    int cy = (int)(51 + vibeYOffset * 0.5f);

    switch (anim) {
        case ANIM_HAPPY: {
            // Open smiling mouth with tongue
            u8g2.drawRBox(cx - 10, cy - 2, 20, 8, 3);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy + 5, 4); // Tongue indentation
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_LAUGH: {
            // Big open laughing mouth with white teeth & tongue
            u8g2.drawRBox(cx - 14, cy - 4, 28, 12, 4);
            u8g2.setDrawColor(0);
            u8g2.drawHLine(cx - 13, cy + 1, 26); // Teeth separator
            u8g2.drawVLine(cx - 7, cy - 3, 4);
            u8g2.drawVLine(cx, cy - 3, 4);
            u8g2.drawVLine(cx + 7, cy - 3, 4);
            u8g2.drawDisc(cx, cy + 6, 4); // Tongue
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_SHOCK: {
            // Classic open cartoon 'O' mouth
            u8g2.drawDisc(cx, cy + 1, 6);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy + 1, 4);
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_SAD: {
            // Downward frown with teardrops
            u8g2.drawLine(cx - 8, cy + 3, cx, cy - 1);
            u8g2.drawLine(cx, cy - 1, cx + 8, cy + 3);
            u8g2.drawLine(cx - 8, cy + 4, cx, cy);
            u8g2.drawLine(cx, cy, cx + 8, cy + 4);

            // Teardrops streaming down cheeks
            int tearY1 = 36 + ((currentMillis / 35) % 22);
            int tearY2 = 36 + (((currentMillis + 250) / 35) % 22);
            u8g2.drawDisc(35, tearY1, 2);
            u8g2.drawDisc(93, tearY2, 2);
            break;
        }
        case ANIM_LOVE: {
            // Warm smile with blushing cheeks
            u8g2.drawRBox(cx - 9, cy - 2, 18, 7, 3);
            // Blushing marks /// on cheeks
            u8g2.drawLine(18, 38, 22, 32); u8g2.drawLine(23, 38, 27, 32);
            u8g2.drawLine(101, 38, 105, 32); u8g2.drawLine(106, 38, 110, 32);
            break;
        }
        case ANIM_ANGRY: {
            // Clenched teeth rectangular grimace
            u8g2.drawFrame(cx - 12, cy - 3, 24, 8);
            u8g2.drawHLine(cx - 12, cy + 1, 24);
            for (int x = cx - 8; x < cx + 12; x += 4) {
                u8g2.drawVLine(x, cy - 3, 8);
            }
            break;
        }
        case ANIM_CURIOUS: {
            // Lopsided smirk hooked up on right
            u8g2.drawLine(cx - 7, cy + 2, cx + 2, cy + 1);
            u8g2.drawLine(cx + 2, cy + 1, cx + 9, cy - 3);
            u8g2.drawLine(cx + 8, cy - 4, cx + 10, cy - 2);
            break;
        }
        case ANIM_THINKING: {
            // Pursed wavy mouth
            u8g2.drawLine(cx - 6, cy, cx - 2, cy + 2);
            u8g2.drawLine(cx - 2, cy + 2, cx + 2, cy - 1);
            u8g2.drawLine(cx + 2, cy - 1, cx + 6, cy + 1);
            break;
        }
        case ANIM_VIBE: {
            // Whistling / singing mouth pulsing to music beat
            float r = 3.5f + sin(currentMillis / 100.0f) * 1.5f;
            u8g2.drawDisc(cx, cy, (int)r);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy, max(1, (int)(r - 2.0f)));
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_SLEEP: {
            // Small peaceful line and floating Z letters
            u8g2.drawHLine(cx - 5, cy, 10);
            int z1Y = 24 - ((currentMillis / 55) % 24);
            int z2Y = 32 - (((currentMillis + 400) / 55) % 24);
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(108, z1Y, "Z");
            u8g2.drawStr(116, z2Y, "z");
            break;
        }
        case ANIM_IDLE:
        default: {
            // Warm smiling line
            u8g2.drawPixel(cx - 8, cy - 1);
            u8g2.drawHLine(cx - 7, cy, 14);
            u8g2.drawPixel(cx + 7, cy - 1);
            break;
        }
    }
}

void updateFace() {
    unsigned long currentMillis = millis();
    
    if (isSequencePlaying) {
        float t = 0.0;
        if (stepDuration > 0) {
            t = (float)(currentMillis - stepStartTime) / stepDuration;
            if (t > 1.0) t = 1.0;
        } else {
            t = 1.0;
        }
        
        float easedT = easeInOutCubic(t);
        interpolateEye(cLeft, sLeft, tLeft, easedT);
        interpolateEye(cRight, sRight, tRight, easedT);
        
        if (t >= 1.0) {
            animStep++;
            executeStep(currentAnim, animStep);
        }
    } else {
        // Idle micro-movements
        if (currentMillis >= nextMicroMoveTime) {
            sLeft = cLeft; sRight = cRight;
            setDefault(tLeft, defaultLeftX);
            setDefault(tRight, defaultRightX);
            
            // Move whole eye mechanically instead of just pupil
            float rx = random(-4, 6);
            float ry = random(-3, 5);
            tLeft.x += rx; tRight.x += rx;
            tLeft.y += ry; tRight.y += ry;
            
            stepStartTime = currentMillis;
            stepDuration = 300;
            isSequencePlaying = true;
            currentAnim = ANIM_IDLE;
            animStep = 0;
            
            nextMicroMoveTime = currentMillis + random(1500, 4000);
        }
    }
    
    // Auto Blink
    if (currentAnim != ANIM_SLEEP && currentMillis >= nextBlinkTime) {
        float bDur = 150.0;
        if (random(0, 5) == 0) bDur = 120.0;
        else if (random(0, 10) == 0) bDur = 400.0;
        triggerBlink(bDur);
        nextBlinkTime = currentMillis + random(2000, 6000);
    }
    
    if (isBlinking) {
        float t = (float)(currentMillis - blinkStartTime) / currentBlinkDuration;
        if (t >= 1.0) {
            isBlinking = false;
            blinkFactor = 0.0;
        } else {
            if (t < 0.3) {
                float normT = t / 0.3;
                blinkFactor = easeInOutCubic(normT);
            } else {
                float normT = (t - 0.3) / 0.7;
                blinkFactor = 1.0 - easeInOutCubic(normT);
            }
        }
    }
    
    // Breathing or Vibing effect
    float breathScale = 1.0;
    float vibeYOffset = 0.0;
    
    if (isMusicPlaying && currentMusicBPM > 0) {
        float beatMs = 60000.0 / currentMusicBPM;
        if (beatMs > 0) {
            float phase = (fmod(currentMillis, beatMs) / beatMs) * PI * 2.0;
            vibeYOffset = sin(phase) * -4.0; // Bounce with the beat
            breathScale = 1.0 + (sin(phase) * 0.04);
        }
    } else {
        breathScale = 1.0 + (sin(currentMillis / 500.0) * 0.04);
        if (currentAnim == ANIM_SLEEP) {
            breathScale = 1.0 + (sin(currentMillis / 800.0) * 0.08);
        }
    }

    displayClear();
    drawEyeShape(cLeft, blinkFactor, breathScale, vibeYOffset);
    drawEyeShape(cRight, blinkFactor, breathScale, vibeYOffset);
    drawCartoonEyebrows(currentAnim, vibeYOffset, currentMillis);
    drawCartoonMouth(currentAnim, breathScale, vibeYOffset, currentMillis);
    displayUpdate();
}
