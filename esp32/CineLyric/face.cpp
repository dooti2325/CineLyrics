#include "face.h"
#include "display.h"
#include <math.h>

// Cartoon Eye Parameters
struct EyeState {
    float x;           // Center X
    float y;           // Center Y
    float w;           // Width
    float h;           // Height
    float r;           // Corner radius
    float pupilX;      // Pupil offset X
    float pupilY;      // Pupil offset Y
    float pupilSize;   // Pupil radius
    float upperLid;    // 0.0 (open) to 1.0 (closed down)
    float lowerLid;    // 0.0 (open) to 1.0 (squint up)
    float lidAngle;    // Angle of upper eyelid (-30 to +30 deg)
    bool isHeart;      // Heart-shaped pupil for ANIM_LOVE
    bool isClosedArc;  // Fully closed line arc (sleep / hard squint)
};

static EyeState cLeft, cRight; // Current
static EyeState tLeft, tRight; // Target
static EyeState sLeft, sRight; // Start

static FaceAnim currentAnim = ANIM_IDLE;
static int animStep = 0;
static unsigned long stepStartTime = 0;
static int stepDuration = 0;
static bool isSequencePlaying = false;

static bool isMusicPlaying = false;
static float currentMusicBPM = 120.0;

// Natural Idle Timers
static unsigned long nextMicroMoveTime = 0;
static unsigned long nextBlinkTime = 0;

// Cartoon Blink State
static bool isBlinking = false;
static unsigned long blinkStartTime = 0;
static float currentBlinkDuration = 160.0;
static float blinkFactor = 0.0; // 0.0 (open) to 1.0 (shut)

// Default Geometry (Designed for 128x64 OLED)
const float defaultY = 26.0;
const float defaultLeftX = 40.0;
const float defaultRightX = 88.0;
const float defaultW = 34.0;
const float defaultH = 34.0;
const float defaultR = 14.0;
const float defaultPupil = 9.0;

static void setDefault(EyeState& e, float x) {
    e.x = x;
    e.y = defaultY;
    e.w = defaultW;
    e.h = defaultH;
    e.r = defaultR;
    e.pupilX = 0;
    e.pupilY = 0;
    e.pupilSize = defaultPupil;
    e.upperLid = 0.0;
    e.lowerLid = 0.0;
    e.lidAngle = 0.0;
    e.isHeart = false;
    e.isClosedArc = false;
}

static void triggerBlink(float duration = 160.0) {
    if (!isBlinking) {
        isBlinking = true;
        blinkStartTime = millis();
        currentBlinkDuration = duration;
    }
}

static void startStep(int duration) {
    sLeft = cLeft;
    sRight = cRight;
    stepStartTime = millis();
    stepDuration = duration;
}

// Easing Functions
static float easeInOutCubic(float t) {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3) / 2.0;
}

static float lerpVal(float s, float t, float factor) {
    return s + (t - s) * factor;
}

static void interpolateEye(EyeState& current, EyeState& start, EyeState& target, float t) {
    current.x = lerpVal(start.x, target.x, t);
    current.y = lerpVal(start.y, target.y, t);
    current.w = lerpVal(start.w, target.w, t);
    current.h = lerpVal(start.h, target.h, t);
    current.r = lerpVal(start.r, target.r, t);
    current.pupilX = lerpVal(start.pupilX, target.pupilX, t);
    current.pupilY = lerpVal(start.pupilY, target.pupilY, t);
    current.pupilSize = lerpVal(start.pupilSize, target.pupilSize, t);
    current.upperLid = lerpVal(start.upperLid, target.upperLid, t);
    current.lowerLid = lerpVal(start.lowerLid, target.lowerLid, t);
    current.lidAngle = lerpVal(start.lidAngle, target.lidAngle, t);
    current.isHeart = target.isHeart;
    current.isClosedArc = target.isClosedArc;
}

// Draw Heart Shape
static void drawHeart(int cx, int cy, int size) {
    int r = size / 2;
    if (r < 2) r = 2;
    u8g2.drawDisc(cx - r / 2, cy - r / 3, r / 2);
    u8g2.drawDisc(cx + r / 2, cy - r / 3, r / 2);
    u8g2.drawTriangle(cx - size / 2, cy - r / 4, cx + size / 2, cy - r / 4, cx, cy + size / 2 + 1);
}

// Draw Cartoon Closed Eye Arc (Image 1 row 1 col 4 / Image 2 row 3 col 5)
static void drawClosedEyeArc(float cx, float cy, float w, bool smileArc = true) {
    u8g2.setDrawColor(1);
    int halfW = (int)(w / 2.0);
    float arch = 5.0;

    for (int i = -halfW; i <= halfW; i++) {
        float norm = (float)i / (float)halfW;
        // Parabolic curve: smile arc curves up (⌒), inverted curves down (◠)
        float curveY = smileArc ? -(1.0f - norm * norm) * arch : (1.0f - norm * norm) * arch;
        int px = (int)(cx + i);
        int py = (int)(cy + curveY);
        u8g2.drawPixel(px, py);
        u8g2.drawPixel(px, py + 1); // 2px thickness
        if (abs(norm) < 0.6f) {
            u8g2.drawPixel(px, py + 2); // Thicker center
        }
    }
}

// Draw Cartoon Eye (Sclera + Pupil + Catchlight + Eyelids)
static void drawCartoonEye(const EyeState& eye, float blinkAmt, float breathScale, float vibeYOffset = 0.0) {
    float drawY = eye.y + vibeYOffset;
    float drawW = eye.w * breathScale;
    float drawH = eye.h * breathScale;

    // Total eyelid closure combining upperLid, lowerLid, and blink
    float effectiveUpper = eye.upperLid + blinkAmt;
    if (effectiveUpper > 1.0) effectiveUpper = 1.0;

    // 1. Fully Closed Cartoon Arc (Blink peak or Sleep/Laugh squint)
    if (eye.isClosedArc || effectiveUpper >= 0.85f) {
        drawClosedEyeArc(eye.x, drawY + 2.0f, drawW * 0.9f, true);
        return;
    }

    // 2. Open / Half-Open Cartoon Eye
    u8g2.setDrawColor(1);
    // Draw white sclera
    u8g2.drawRBox(eye.x - drawW / 2.0f, drawY - drawH / 2.0f, drawW, drawH, eye.r);

    // 3. Draw Pupil & Highlights
    if (effectiveUpper < 0.8f) {
        float pX = eye.x + eye.pupilX;
        float pY = drawY + eye.pupilY;

        if (eye.isHeart) {
            // Heart Pupil (ANIM_LOVE - Reference Image 1 row 3 col 5)
            u8g2.setDrawColor(0);
            drawHeart((int)pX, (int)pY, (int)(eye.pupilSize * 1.8f));
            u8g2.setDrawColor(1);
            // Cute sparkle glint
            u8g2.drawDisc((int)(pX - eye.pupilSize * 0.4f), (int)(pY - eye.pupilSize * 0.4f), 1.5f);
        } else {
            // Standard Cartoon Black Pupil
            u8g2.setDrawColor(0);
            u8g2.drawDisc((int)pX, (int)pY, (int)eye.pupilSize);

            // Specular Cartoon Catchlights (Big highlight + tiny secondary glint)
            u8g2.setDrawColor(1);
            u8g2.drawDisc((int)(pX - eye.pupilSize * 0.35f), (int)(pY - eye.pupilSize * 0.35f), max(1.5f, eye.pupilSize * 0.30f));
            u8g2.drawPixel((int)(pX + eye.pupilSize * 0.30f), (int)(pY + eye.pupilSize * 0.30f));
        }
    }

    // 4. Eyelid Masks (Clean horizontal or angled cuts)
    u8g2.setDrawColor(0);
    // Upper Eyelid
    if (effectiveUpper > 0.05f) {
        float lidDrop = drawH * effectiveUpper;
        float topY = drawY - drawH / 2.0f;

        if (abs(eye.lidAngle) < 1.0f) {
            // Flat horizontal lid (sleepy / bored / blink sweep)
            u8g2.drawBox((int)(eye.x - drawW / 2.0f - 1), (int)(topY - 2), (int)(drawW + 2), (int)(lidDrop + 2));
        } else {
            // Angled lid (Angry \ / or Sad / \)
            float rad = eye.lidAngle * PI / 180.0f;
            float p1y = topY + lidDrop - tan(rad) * (drawW / 2.0f);
            float p2y = topY + lidDrop + tan(rad) * (drawW / 2.0f);

            int x1 = (int)(eye.x - drawW / 2.0f - 2);
            int x2 = (int)(eye.x + drawW / 2.0f + 2);
            u8g2.drawTriangle(x1, (int)topY - 4, x2, (int)topY - 4, x1, (int)p1y);
            u8g2.drawTriangle(x2, (int)topY - 4, x1, (int)p1y, x2, (int)p2y);
        }
    }

    // Lower Eyelid (Happy squint curving up from bottom)
    if (eye.lowerLid > 0.05f) {
        float lidRise = drawH * eye.lowerLid;
        float bottomY = drawY + drawH / 2.0f;
        u8g2.drawBox((int)(eye.x - drawW / 2.0f - 1), (int)(bottomY - lidRise), (int)(drawW + 2), (int)(lidRise + 4));
    }
    u8g2.setDrawColor(1);
}

// Draw Expressive Eyebrow with thickness and arch
static void drawCartoonEyebrow(float x, float y, float w, float angle, float arch) {
    u8g2.setDrawColor(1);
    float rad = angle * PI / 180.0f;
    int halfW = (int)(w / 2.0f);

    for (int i = -halfW; i <= halfW; i++) {
        float norm = (float)i / (float)halfW;
        float curveY = -(1.0f - norm * norm) * arch + i * tan(rad);
        int px = (int)(x + i);
        int py = (int)(y + curveY);
        u8g2.drawPixel(px, py);
        u8g2.drawPixel(px, py - 1);
        if (abs(norm) < 0.6f) {
            u8g2.drawPixel(px, py - 2); // Thicker center peak
        }
    }
}

// Draw Eyebrows matching the Reference Sheet
static void drawAllEyebrows(FaceAnim anim, float vibeYOffset, unsigned long currentMillis) {
    float leftY = cLeft.y - (cLeft.h / 2.0f) - 4.0f + vibeYOffset;
    float rightY = cRight.y - (cRight.h / 2.0f) - 4.0f + vibeYOffset;

    switch (anim) {
        case ANIM_HAPPY:
        case ANIM_LAUGH:
            // High joyful arches
            drawCartoonEyebrow(cLeft.x, leftY - 4, 22, -6, 5);
            drawCartoonEyebrow(cRight.x, rightY - 4, 22, 6, 5);
            break;
        case ANIM_SAD:
            // Tilted inward and up / \ (Reference Image 2 row 1 col 4)
            drawCartoonEyebrow(cLeft.x, leftY - 2, 20, 22, 1);
            drawCartoonEyebrow(cRight.x, rightY - 2, 20, -22, 1);
            break;
        case ANIM_ANGRY:
            // Fierce sharp V-brows \ / (Reference Image 2 row 2 col 5)
            drawCartoonEyebrow(cLeft.x, leftY + 1, 24, -26, 1);
            drawCartoonEyebrow(cRight.x, rightY + 1, 24, 26, 1);
            break;
        case ANIM_CURIOUS:
            // Asymmetrical (Reference Image 2 row 2 col 1)
            drawCartoonEyebrow(cLeft.x, leftY - 7, 22, -8, 6);
            drawCartoonEyebrow(cRight.x, rightY, 20, 4, 1);
            break;
        case ANIM_SHOCK:
            // High startled arches (Reference Image 2 row 1 col 1)
            drawCartoonEyebrow(cLeft.x, leftY - 7, 24, 0, 6);
            drawCartoonEyebrow(cRight.x, rightY - 7, 24, 0, 6);
            break;
        case ANIM_THINKING:
            // Questioning brows
            drawCartoonEyebrow(cLeft.x, leftY - 1, 20, 12, 1);
            drawCartoonEyebrow(cRight.x, rightY - 6, 24, -12, 5);
            break;
        case ANIM_LOVE:
            // Gentle sweet floating arches
            drawCartoonEyebrow(cLeft.x, leftY - 4, 22, -4, 4);
            drawCartoonEyebrow(cRight.x, rightY - 4, 22, 4, 4);
            break;
        case ANIM_VIBE: {
            float tilt = sin(currentMillis / 180.0f) * 12.0f;
            drawCartoonEyebrow(cLeft.x, leftY - 3, 22, tilt, 4);
            drawCartoonEyebrow(cRight.x, rightY - 3, 22, tilt, 4);
            break;
        }
        case ANIM_SLEEP:
            // Soft flat sleeping curves
            drawCartoonEyebrow(cLeft.x, leftY - 1, 20, 0, 2);
            drawCartoonEyebrow(cRight.x, rightY - 1, 20, 0, 2);
            break;
        case ANIM_IDLE:
        default:
            drawCartoonEyebrow(cLeft.x, leftY - 2, 22, -2, 3);
            drawCartoonEyebrow(cRight.x, rightY - 2, 22, 2, 3);
            break;
    }
}

// Draw Expressive Cartoon Mouths (Reference Image 2)
static void drawCartoonMouth(FaceAnim anim, float vibeYOffset, unsigned long currentMillis) {
    u8g2.setDrawColor(1);
    int cx = 64;
    int cy = (int)(51 + vibeYOffset * 0.5f);

    switch (anim) {
        case ANIM_HAPPY: {
            // Open smiling mouth with curved tongue
            u8g2.drawRBox(cx - 10, cy - 2, 20, 8, 3);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy + 5, 4); // Tongue
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_LAUGH: {
            // Big open laughing mouth with white teeth & tongue (Image 2 row 1 col 1)
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
            // Classic open round 'O' mouth (Image 2 row 1 col 1)
            u8g2.drawDisc(cx, cy + 1, 6);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy + 1, 4);
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_SAD: {
            // Downward frown with teardrops streaming (Image 2 row 1 col 7)
            u8g2.drawLine(cx - 8, cy + 3, cx, cy - 1);
            u8g2.drawLine(cx, cy - 1, cx + 8, cy + 3);
            u8g2.drawLine(cx - 8, cy + 4, cx, cy);
            u8g2.drawLine(cx, cy, cx + 8, cy + 4);

            // Animated teardrops
            int tearY1 = 36 + ((currentMillis / 35) % 22);
            int tearY2 = 36 + (((currentMillis + 250) / 35) % 22);
            u8g2.drawDisc(35, tearY1, 2);
            u8g2.drawDisc(93, tearY2, 2);
            break;
        }
        case ANIM_LOVE: {
            // Sweet smile with blushing cheek slashes ///
            u8g2.drawRBox(cx - 9, cy - 2, 18, 7, 3);
            u8g2.drawLine(18, 38, 22, 32); u8g2.drawLine(23, 38, 27, 32);
            u8g2.drawLine(101, 38, 105, 32); u8g2.drawLine(106, 38, 110, 32);
            break;
        }
        case ANIM_ANGRY: {
            // Clenched teeth grimace box (Image 2 row 2 col 5)
            u8g2.drawFrame(cx - 12, cy - 3, 24, 8);
            u8g2.drawHLine(cx - 12, cy + 1, 24);
            for (int x = cx - 8; x < cx + 12; x += 4) {
                u8g2.drawVLine(x, cy - 3, 8);
            }
            break;
        }
        case ANIM_CURIOUS: {
            // Smirk hooked up on right (Image 2 row 1 col 8)
            u8g2.drawLine(cx - 7, cy + 2, cx + 2, cy + 1);
            u8g2.drawLine(cx + 2, cy + 1, cx + 9, cy - 3);
            u8g2.drawLine(cx + 8, cy - 4, cx + 10, cy - 2);
            break;
        }
        case ANIM_THINKING: {
            // Pursed wavy line (Image 2 row 3 col 6)
            u8g2.drawLine(cx - 6, cy, cx - 2, cy + 2);
            u8g2.drawLine(cx - 2, cy + 2, cx + 2, cy - 1);
            u8g2.drawLine(cx + 2, cy - 1, cx + 6, cy + 1);
            break;
        }
        case ANIM_VIBE: {
            // Whistling mouth pulsing to music
            float r = 3.5f + sin(currentMillis / 100.0f) * 1.5f;
            u8g2.drawDisc(cx, cy, (int)r);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(cx, cy, max(1, (int)(r - 2.0f)));
            u8g2.setDrawColor(1);
            break;
        }
        case ANIM_SLEEP: {
            // Calm line with floating animated Z letters (Image 2 row 4 col 2)
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

// Step Sequencer for Expressions
static void executeStep(FaceAnim anim, int step) {
    setDefault(tLeft, defaultLeftX);
    setDefault(tRight, defaultRightX);

    switch (anim) {
        case ANIM_IDLE:
            isSequencePlaying = false;
            break;

        case ANIM_HAPPY:
            if (step == 0) {
                // Joyful squint crescents (Image 1 row 2 col 1)
                tLeft.lowerLid = 0.5f; tRight.lowerLid = 0.5f;
                tLeft.y -= 4; tRight.y -= 4;
                startStep(250);
            } else if (step == 1) {
                // Bounce up with high smile
                tLeft.lowerLid = 0.5f; tRight.lowerLid = 0.5f;
                tLeft.y -= 8; tRight.y -= 8;
                startStep(120);
            } else if (step == 2) {
                tLeft.lowerLid = 0.5f; tRight.lowerLid = 0.5f;
                tLeft.y -= 4; tRight.y -= 4;
                startStep(120);
            } else if (step == 3) {
                tLeft.lowerLid = 0.5f; tRight.lowerLid = 0.5f;
                startStep(1800);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_LAUGH:
            if (step == 0) {
                // Closed laughing arcs (Image 1 row 4 col 7)
                tLeft.isClosedArc = true; tRight.isClosedArc = true;
                tLeft.y -= 6; tRight.y -= 6;
                startStep(150);
            } else if (step == 1) {
                tLeft.isClosedArc = true; tRight.isClosedArc = true;
                tLeft.y += 4; tRight.y += 4;
                startStep(120);
            } else if (step == 2) {
                tLeft.isClosedArc = true; tRight.isClosedArc = true;
                tLeft.y -= 6; tRight.y -= 6;
                startStep(120);
            } else if (step == 3) {
                tLeft.isClosedArc = true; tRight.isClosedArc = true;
                startStep(1200);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_LOVE:
            if (step == 0) {
                triggerBlink(140);
                startStep(140);
            } else if (step == 1) {
                // Heart Pupils! (Image 1 row 3 col 5 / Image 2 row 1 col 2)
                tLeft.isHeart = true; tRight.isHeart = true;
                tLeft.pupilSize = 12.0; tRight.pupilSize = 12.0;
                tLeft.w += 2; tRight.w += 2;
                startStep(2500);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_SHOCK:
            if (step == 0) {
                // Wide open staring eyes with small stunned pupil (Image 2 row 1 col 1)
                tLeft.w = 38; tRight.w = 38;
                tLeft.h = 40; tRight.h = 40;
                tLeft.pupilSize = 5.0; tRight.pupilSize = 5.0;
                startStep(2000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_SAD:
            if (step == 0) {
                // Inward droop lids + looking down (Image 1 row 2 col 3)
                tLeft.upperLid = 0.35f; tRight.upperLid = 0.35f;
                tLeft.lidAngle = 20.0f; tRight.lidAngle = -20.0f;
                tLeft.pupilY = 4.0f; tRight.pupilY = 4.0f;
                startStep(2500);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_ANGRY:
            if (step == 0) {
                // Fierce downward V-lids (Image 2 row 2 col 5)
                tLeft.upperLid = 0.38f; tRight.upperLid = 0.38f;
                tLeft.lidAngle = -24.0f; tRight.lidAngle = 24.0f;
                tLeft.pupilSize = 8.0f; tRight.pupilSize = 8.0f;
                startStep(2500);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_CURIOUS:
            if (step == 0) {
                // Asymmetrical: Left wide open, Right half-lidded (Image 1 row 2 col 2)
                tLeft.w = 36; tLeft.h = 38;
                tRight.upperLid = 0.40f;
                tLeft.pupilX = 3.0f; tRight.pupilX = 3.0f;
                startStep(2200);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_THINKING:
            if (step == 0) {
                // Both pupils looking up and left (Image 1 row 2 col 8)
                tLeft.pupilX = -6.0f; tRight.pupilX = -6.0f;
                tLeft.pupilY = -6.0f; tRight.pupilY = -6.0f;
                tRight.upperLid = 0.25f;
                startStep(2400);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_VIBE:
            if (step == 0) {
                tLeft.lowerLid = 0.3f; tRight.lowerLid = 0.3f;
                startStep(3000);
            } else {
                playFaceAnimation(ANIM_IDLE);
            }
            break;

        case ANIM_SLEEP:
            // Peaceful closed arc lines (Image 1 row 1 col 4 / Image 2 row 4 col 2)
            tLeft.isClosedArc = true;
            tRight.isClosedArc = true;
            startStep(500);
            break;

        case ANIM_WAKE:
            if (step == 0) {
                tLeft.upperLid = 0.5f; tRight.upperLid = 0.5f;
                startStep(300);
            } else if (step == 1) {
                triggerBlink(150);
                startStep(200);
            } else {
                playFaceAnimation(ANIM_HAPPY);
            }
            break;

        default:
            playFaceAnimation(ANIM_IDLE);
            break;
    }
}

// Public API
void setupFace() {
    setDefault(cLeft, defaultLeftX);
    setDefault(cRight, defaultRightX);
    playFaceAnimation(ANIM_IDLE);
    nextBlinkTime = millis() + random(2000, 5000);
    nextMicroMoveTime = millis() + random(1200, 3500);
}

void playFaceAnimation(FaceAnim anim) {
    currentAnim = anim;
    animStep = 0;
    isSequencePlaying = true;
    executeStep(currentAnim, animStep);
}

void setMusicState(bool isPlaying, float bpm) {
    if (isPlaying && !isMusicPlaying) {
        playFaceAnimation(ANIM_HAPPY);
    } else if (!isPlaying && isMusicPlaying) {
        playFaceAnimation(ANIM_SAD);
    }
    isMusicPlaying = isPlaying;
    if (bpm > 0) {
        currentMusicBPM = bpm;
    }
}

void updateFace() {
    unsigned long currentMillis = millis();

    // 1. Step Sequencer
    if (isSequencePlaying) {
        float t = 0.0;
        if (stepDuration > 0) {
            t = (float)(currentMillis - stepStartTime) / (float)stepDuration;
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
        // Natural Idle Micro-movements (Look around naturally)
        if (currentMillis >= nextMicroMoveTime) {
            sLeft = cLeft;
            sRight = cRight;
            setDefault(tLeft, defaultLeftX);
            setDefault(tRight, defaultRightX);

            float rx = (float)random(-5, 6);
            float ry = (float)random(-3, 4);
            tLeft.pupilX = rx; tRight.pupilX = rx;
            tLeft.pupilY = ry; tRight.pupilY = ry;

            stepStartTime = currentMillis;
            stepDuration = 220;
            isSequencePlaying = true;
            currentAnim = ANIM_IDLE;
            animStep = 0;

            nextMicroMoveTime = currentMillis + random(1800, 4500);
        }
    }

    // 2. Cartoon Blink Engine
    if (currentAnim != ANIM_SLEEP && !cLeft.isClosedArc && currentMillis >= nextBlinkTime) {
        float bDur = 160.0;
        if (random(0, 5) == 0) bDur = 130.0;
        else if (random(0, 10) == 0) bDur = 300.0; // Slow sleepy blink
        triggerBlink(bDur);
        nextBlinkTime = currentMillis + random(2500, 6000);
    }

    if (isBlinking) {
        float t = (float)(currentMillis - blinkStartTime) / currentBlinkDuration;
        if (t >= 1.0) {
            isBlinking = false;
            blinkFactor = 0.0;
        } else {
            if (t < 0.4) {
                // Rapid sweep down
                float normT = t / 0.4;
                blinkFactor = easeInOutCubic(normT);
            } else {
                // Smooth rise back up
                float normT = (t - 0.4) / 0.6;
                blinkFactor = 1.0 - easeInOutCubic(normT);
            }
        }
    }

    // 3. Music Vibing & Breathing
    float breathScale = 1.0;
    float vibeYOffset = 0.0;

    if (isMusicPlaying && currentMusicBPM > 0) {
        float beatMs = 60000.0 / currentMusicBPM;
        if (beatMs > 0) {
            float phase = (fmod(currentMillis, beatMs) / beatMs) * PI * 2.0;
            vibeYOffset = sin(phase) * -4.0; // Bounces to beat
            breathScale = 1.0 + (sin(phase) * 0.04);
        }
    } else {
        breathScale = 1.0 + (sin(currentMillis / 500.0) * 0.03);
        if (currentAnim == ANIM_SLEEP) {
            breathScale = 1.0 + (sin(currentMillis / 800.0) * 0.06);
        }
    }

    // 4. Render All Cartoon Layers
    displayClear();
    drawCartoonEye(cLeft, blinkFactor, breathScale, vibeYOffset);
    drawCartoonEye(cRight, blinkFactor, breathScale, vibeYOffset);
    drawAllEyebrows(currentAnim, vibeYOffset, currentMillis);
    drawCartoonMouth(currentAnim, vibeYOffset, currentMillis);
    displayUpdate();
}
