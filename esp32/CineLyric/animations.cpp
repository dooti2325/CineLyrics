#include "animations.h"
#include "display.h"
#include "font_engine.h"
#include "effects.h"
#include <string.h>
#include <math.h>

static AnimPacket currentPacket;

static unsigned long frameCount = 0;
static unsigned long animStartTime = 0;

static bool animOverridden = false;
static int animIndex = 0;
const char* const animList[] = {"fade", "slide_left", "slide_up", "typewriter", "karaoke", "shake", "wave", "glitch"};
const int numAnims = 8;

void cycleAnimation() {
    animIndex = (animIndex + 1) % numAnims;
    strncpy(currentPacket.animType, animList[animIndex], sizeof(currentPacket.animType) - 1);
    currentPacket.animType[sizeof(currentPacket.animType) - 1] = '\0';
    animOverridden = true;
    animStartTime = millis(); // restart animation with new style
}

void setAnimationState(const AnimPacket& pkt) {
    if (strcmp(currentPacket.title, pkt.title) != 0) {
        animOverridden = false; // Reset override on new song
    }
    
    // Copy the struct
    currentPacket = pkt;
    
    if (!animOverridden) {
        // Sync index
        for (int i = 0; i < numAnims; i++) {
            if (strcmp(animList[i], currentPacket.animType) == 0) {
                animIndex = i;
                break;
            }
        }
    } else {
        // Restore overridden anim
        strncpy(currentPacket.animType, animList[animIndex], sizeof(currentPacket.animType) - 1);
        currentPacket.animType[sizeof(currentPacket.animType) - 1] = '\0';
    }
    
    frameCount = 0;
    animStartTime = millis();
}

void drawFade() {
    // Simulate fade by lowering contrast initially, then maxing it
    // U8g2 contrast on SH1106 ranges 0-255.
    long elapsed = millis() - animStartTime;
    uint8_t contrast = 255;
    if (elapsed < 500) {
        contrast = (elapsed * 255) / 500;
    }
    u8g2.setContrast(contrast);
    drawLyrics(currentPacket.lyric);
}

void drawSlideLeft() {
    long elapsed = millis() - animStartTime;
    int offset = 128 - (elapsed * 128) / 500; // Slide in over 500ms
    if (offset < 0) offset = 0;
    
    int width = u8g2.getUTF8Width(currentPacket.lyric);
    int x = (128 - width) / 2 + offset;
    drawText(currentPacket.lyric, x, 36, getFontByStyle(currentPacket.font));
}

void drawSlideUp() {
    long elapsed = millis() - animStartTime;
    int offset = 64 - (elapsed * 64) / 500;
    if (offset < 0) offset = 0;
    
    drawLyrics(currentPacket.lyric, offset);
}

void drawTypewriter() {
    long elapsed = millis() - animStartTime;
    int len = strlen(currentPacket.lyric);
    int charsToShow = (elapsed * len) / 1000; // Type out over 1s
    if (charsToShow > len) charsToShow = len;
    
    char temp[128];
    strncpy(temp, currentPacket.lyric, charsToShow);
    temp[charsToShow] = '\0';
    
    drawLyrics(temp);
}

void drawKaraoke() {
    long elapsed = millis() - animStartTime;
    
    int numWords = 1;
    for (int i = 0; currentPacket.lyric[i] != '\0'; i++) {
        if (currentPacket.lyric[i] == ' ') numWords++;
    }
    
    int timePerWord = currentPacket.duration / numWords;
    if (timePerWord <= 0) timePerWord = 1;
    
    int activeIndex = elapsed / timePerWord;
    if (activeIndex >= numWords) activeIndex = numWords - 1;
    
    char formatted[256] = "";
    char temp[128];
    strncpy(temp, currentPacket.lyric, sizeof(temp));
    
    char* token = strtok(temp, " ");
    int currentIndex = 0;
    
    while (token != NULL) {
        if (currentIndex == activeIndex) {
            strcat(formatted, "[");
            strcat(formatted, token);
            strcat(formatted, "]");
        } else {
            strcat(formatted, token);
        }
        strcat(formatted, " ");
        token = strtok(NULL, " ");
        currentIndex++;
    }
    
    drawLyrics(formatted);
}

void drawShake() {
    int maxShake = 1 + (int)(currentPacket.energy * 6);
    int offsetX = random(-maxShake, maxShake + 1);
    int offsetY = random(-maxShake, maxShake + 1);
    
    int width = u8g2.getUTF8Width(currentPacket.lyric);
    int x = (128 - width) / 2 + offsetX;
    drawText(currentPacket.lyric, x, 36 + offsetY, getFontByStyle(currentPacket.font));
}

void drawWave() {
    long elapsed = millis() - animStartTime;
    const uint8_t* font = getFontByStyle(currentPacket.font);
    u8g2.setFont(font);
    
    int len = strlen(currentPacket.lyric);
    int startX = (128 - u8g2.getUTF8Width(currentPacket.lyric)) / 2;
    
    int currentX = startX;
    char singleChar[2] = {0, 0};
    
    for (int i = 0; i < len; i++) {
        singleChar[0] = currentPacket.lyric[i];
        int offsetY = sin((elapsed / 100.0) + i) * 5;
        u8g2.drawUTF8(currentX, 36 + offsetY, singleChar);
        currentX += u8g2.getUTF8Width(singleChar);
    }
}

void drawGlitch() {
    drawLyrics(currentPacket.lyric);
    
    // Add random lines/rects for glitch effect scaled by energy
    int glitchProbability = (int)(currentPacket.energy * 10);
    if (random(0, 10) < glitchProbability) {
        int y = random(10, 50);
        int h = random(1, 5);
        u8g2.setDrawColor(2); // XOR mode
        u8g2.drawBox(0, y, 128, h);
        u8g2.setDrawColor(1);
    }
}

void updateAnimation() {
    displayClear();
    
    if (currentPacket.bpm > 0) {
        float beatInterval = 60000.0 / currentPacket.bpm;
        float beatPhase = fmod((float)millis(), beatInterval) / beatInterval;
        
        // Apply camera effect (currently only calculate, not globally applied to u8g2 as there's no native global offset without manual translation)
        int camX = 0, camY = 0;
        if (currentPacket.shake) {
            applyCameraEffect("Shake", beatPhase, currentPacket.beatStrength, camX, camY);
        } else {
            applyCameraEffect(currentPacket.secondary, beatPhase, currentPacket.beatStrength, camX, camY);
        }
        
        u8g2.setDrawColor(1); // Ensure default drawing color is lit

        // Background Effects
        drawBackgroundEffect(currentPacket.particles, beatPhase, currentPacket.beatStrength);
    } else {
        u8g2.setDrawColor(1);
    }
    
    if (strcmp(currentPacket.animType, "fade") != 0) {
        u8g2.setContrast(255);
    }
    
    // Show Title/Artist if no lyrics or at beginning of song
    if (strlen(currentPacket.lyric) == 0 || strcmp(currentPacket.lyric, "No lyrics found") == 0 || strcmp(currentPacket.lyric, "♪") == 0) {
        drawTitleArtist(currentPacket.title, currentPacket.artist);
    } else {
        if (strcmp(currentPacket.animType, "fade") == 0) drawFade();
        else if (strcmp(currentPacket.animType, "slide_left") == 0) drawSlideLeft();
        else if (strcmp(currentPacket.animType, "slide_up") == 0) drawSlideUp();
        else if (strcmp(currentPacket.animType, "typewriter") == 0) drawTypewriter();
        else if (strcmp(currentPacket.animType, "karaoke") == 0) drawKaraoke();
        else if (strcmp(currentPacket.animType, "shake") == 0) drawShake();
        else if (strcmp(currentPacket.animType, "wave") == 0) drawWave();
        else if (strcmp(currentPacket.animType, "glitch") == 0) drawGlitch();
        else drawLyrics(currentPacket.lyric); // Default
    }
    
    displayUpdate();
    frameCount++;
}
