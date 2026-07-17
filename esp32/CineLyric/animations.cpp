#include "animations.h"
#include "display.h"
#include <string.h>
#include <math.h>

static char currentLyric[128] = "";
static char nextLyric[128] = "";
static char currentAnim[32] = "fade";
static char currentTitle[128] = "";
static char currentArtist[128] = "";
static float currentBpm = 120.0;
static float currentEnergy = 0.5;
static int currentDur = 5000;

static unsigned long frameCount = 0;
static unsigned long animStartTime = 0;

void setAnimationState(const char* text, const char* nextText, const char* animType, const char* title, const char* artist, float bpm, float energy, int dur) {
    strncpy(currentLyric, text, sizeof(currentLyric) - 1);
    currentLyric[sizeof(currentLyric) - 1] = '\0';
    
    if (nextText) {
        strncpy(nextLyric, nextText, sizeof(nextLyric) - 1);
        nextLyric[sizeof(nextLyric) - 1] = '\0';
    } else {
        nextLyric[0] = '\0';
    }
    
    strncpy(currentAnim, animType, sizeof(currentAnim) - 1);
    currentAnim[sizeof(currentAnim) - 1] = '\0';
    
    if (title) {
        strncpy(currentTitle, title, sizeof(currentTitle) - 1);
        currentTitle[sizeof(currentTitle) - 1] = '\0';
    }
    if (artist) {
        strncpy(currentArtist, artist, sizeof(currentArtist) - 1);
        currentArtist[sizeof(currentArtist) - 1] = '\0';
    }
    
    currentBpm = bpm;
    currentEnergy = energy;
    currentDur = dur;
    
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
    drawLyrics(currentLyric);
}

void drawSlideLeft() {
    long elapsed = millis() - animStartTime;
    int offset = 128 - (elapsed * 128) / 500; // Slide in over 500ms
    if (offset < 0) offset = 0;
    
    u8g2.setFont(u8g2_font_helvB08_tr);
    int width = u8g2.getUTF8Width(currentLyric);
    int x = (128 - width) / 2 + offset;
    drawText(currentLyric, x, 36, u8g2_font_helvB08_tr);
}

void drawSlideUp() {
    long elapsed = millis() - animStartTime;
    int offset = 64 - (elapsed * 64) / 500;
    if (offset < 0) offset = 0;
    
    drawLyrics(currentLyric, offset);
}

void drawTypewriter() {
    long elapsed = millis() - animStartTime;
    int len = strlen(currentLyric);
    int charsToShow = (elapsed * len) / 1000; // Type out over 1s
    if (charsToShow > len) charsToShow = len;
    
    char temp[128];
    strncpy(temp, currentLyric, charsToShow);
    temp[charsToShow] = '\0';
    
    drawLyrics(temp);
}

void drawKaraoke() {
    long elapsed = millis() - animStartTime;
    
    int numWords = 1;
    for (int i = 0; currentLyric[i] != '\0'; i++) {
        if (currentLyric[i] == ' ') numWords++;
    }
    
    int timePerWord = currentDur / numWords;
    if (timePerWord <= 0) timePerWord = 1;
    
    int activeIndex = elapsed / timePerWord;
    if (activeIndex >= numWords) activeIndex = numWords - 1;
    
    char formatted[256] = "";
    char temp[128];
    strncpy(temp, currentLyric, sizeof(temp));
    
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
    int maxShake = 1 + (int)(currentEnergy * 6);
    int offsetX = random(-maxShake, maxShake + 1);
    int offsetY = random(-maxShake, maxShake + 1);
    
    u8g2.setFont(u8g2_font_helvB08_tr);
    int width = u8g2.getUTF8Width(currentLyric);
    int x = (128 - width) / 2 + offsetX;
    drawText(currentLyric, x, 36 + offsetY, u8g2_font_helvB08_tr);
}

void drawWave() {
    long elapsed = millis() - animStartTime;
    u8g2.setFont(u8g2_font_helvB08_tr);
    
    int len = strlen(currentLyric);
    int startX = (128 - u8g2.getUTF8Width(currentLyric)) / 2;
    
    int currentX = startX;
    char singleChar[2] = {0, 0};
    
    for (int i = 0; i < len; i++) {
        singleChar[0] = currentLyric[i];
        int offsetY = sin((elapsed / 100.0) + i) * 5;
        u8g2.drawUTF8(currentX, 36 + offsetY, singleChar);
        currentX += u8g2.getUTF8Width(singleChar);
    }
}

void drawGlitch() {
    drawLyrics(currentLyric);
    
    // Add random lines/rects for glitch effect scaled by energy
    int glitchProbability = (int)(currentEnergy * 10);
    if (random(0, 10) < glitchProbability) {
        int y = random(10, 50);
        int h = random(1, 5);
        u8g2.setDrawColor(2); // XOR mode
        u8g2.drawBox(0, y, 128, h);
        u8g2.setDrawColor(1);
    }
}

void drawBeatBackground() {
    if (currentBpm <= 0) return;
    float beatInterval = 60000.0 / currentBpm;
    float beatPhase = fmod((float)millis(), beatInterval) / beatInterval;
    
    // Pulse effect
    int intensity = (1.0 - beatPhase) * 10.0 * currentEnergy; 
    
    if (intensity > 0) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, 0, intensity, intensity);
        u8g2.drawBox(128 - intensity, 0, intensity, intensity);
        u8g2.drawBox(0, 64 - intensity, intensity, intensity);
        u8g2.drawBox(128 - intensity, 64 - intensity, intensity, intensity);
    }
}

void updateAnimation() {
    displayClear();
    
    // Reset contrast if not fading
    if (strcmp(currentAnim, "fade") != 0) {
        u8g2.setContrast(255);
    }
    
    // Show Title/Artist if no lyrics or at beginning of song
    if (strlen(currentLyric) == 0 || strcmp(currentLyric, "No lyrics found") == 0 || strcmp(currentLyric, "♪") == 0) {
        drawTitleArtist(currentTitle, currentArtist);
    } else {
        drawBeatBackground();

        if (strcmp(currentAnim, "fade") == 0) drawFade();
        else if (strcmp(currentAnim, "slide_left") == 0) drawSlideLeft();
        else if (strcmp(currentAnim, "slide_up") == 0) drawSlideUp();
        else if (strcmp(currentAnim, "typewriter") == 0) drawTypewriter();
        else if (strcmp(currentAnim, "karaoke") == 0) drawKaraoke();
        else if (strcmp(currentAnim, "shake") == 0) drawShake();
        else if (strcmp(currentAnim, "wave") == 0) drawWave();
        else if (strcmp(currentAnim, "glitch") == 0) drawGlitch();
        else drawLyrics(currentLyric); // Default
    }
    
    displayUpdate();
    frameCount++;
}
