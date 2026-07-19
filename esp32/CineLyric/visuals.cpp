#include "visuals.h"
#include "display.h"
#include <math.h>

static float currentBpm = 120.0;
static float currentEnergy = 0.5;

static int visualIndex = 0;
const int numVisuals = 4;

static unsigned long lastBeatTime = 0;
static float beatPhase = 0.0;

void setVisualsData(float bpm, float energy) {
    currentBpm = bpm > 0 ? bpm : 120.0;
    currentEnergy = energy > 0 ? energy : 0.5;
}

void cycleVisuals() {
    visualIndex = (visualIndex + 1) % numVisuals;
}

void drawEQBars(float phase, float energy) {
    int numBars = 8;
    int barWidth = 128 / numBars;
    
    // Create some fake frequencies that bounce based on the beat phase and energy
    for (int i = 0; i < numBars; i++) {
        // Pseudo-random height based on bar index, energy, and beat phase
        float freqPhase = phase * (i + 1) * 0.5;
        float heightBase = (sin(freqPhase) * 0.5 + 0.5) * 64 * energy;
        // Bump it up strongly on the beat
        if (phase < 0.2) {
            heightBase += (0.2 - phase) * 5.0 * 64 * energy;
        }
        
        int barHeight = (int)heightBase;
        if (barHeight > 64) barHeight = 64;
        if (barHeight < 2) barHeight = 2; // min height
        
        int x = i * barWidth + 2;
        int y = 64 - barHeight;
        int w = barWidth - 4;
        
        u8g2.setDrawColor(1);
        u8g2.drawBox(x, y, w, barHeight);
    }
}

void drawPulse(float phase, float energy) {
    int centerX = 64;
    int centerY = 32;
    
    // Main pulsing circle
    float radiusMain = 10 + (1.0 - phase) * 30 * energy;
    if (radiusMain > 64) radiusMain = 64;
    
    u8g2.setDrawColor(1);
    u8g2.drawCircle(centerX, centerY, (int)radiusMain);
    
    // Secondary circle
    float radiusSec = 5 + (1.0 - fmod(phase + 0.5, 1.0)) * 20 * energy;
    u8g2.drawCircle(centerX, centerY, (int)radiusSec);
    
    // Center dot bouncing
    int centerR = 2 + energy * 10 * (1.0 - phase);
    u8g2.drawDisc(centerX, centerY, centerR);
}

void drawWaveform(float phase, float energy) {
    int centerY = 32;
    u8g2.setDrawColor(1);
    
    int lastX = 0;
    int lastY = centerY;
    
    for (int x = 0; x < 128; x += 2) {
        float timeOffset = millis() / 500.0;
        // Calculate y based on sine waves that modulate with beat phase and energy
        float yOffset = sin(x * 0.1 + timeOffset) * 20 * energy;
        yOffset += sin(x * 0.05 - timeOffset * 2) * 10 * energy * (1.0 - phase); // Beat adds high frequency noise/amplitude
        
        int y = centerY + (int)yOffset;
        if (y < 0) y = 0;
        if (y > 63) y = 63;
        
        if (x > 0) {
            u8g2.drawLine(lastX, lastY, x, y);
        }
        lastX = x;
        lastY = y;
    }
}

void drawParticles(float phase, float energy) {
    // A simple starfield/particle explosion effect that accelerates on beat
    static float particlesX[20];
    static float particlesY[20];
    static float particlesVX[20];
    static float particlesVY[20];
    static bool init = false;
    
    if (!init) {
        for (int i = 0; i < 20; i++) {
            particlesX[i] = 64;
            particlesY[i] = 32;
            float angle = random(0, 360) * PI / 180.0;
            float speed = random(5, 20) / 10.0;
            particlesVX[i] = cos(angle) * speed;
            particlesVY[i] = sin(angle) * speed;
        }
        init = true;
    }
    
    float speedMult = 1.0 + (1.0 - phase) * 3.0 * energy; // Accelerate on beat
    
    u8g2.setDrawColor(1);
    for (int i = 0; i < 20; i++) {
        particlesX[i] += particlesVX[i] * speedMult;
        particlesY[i] += particlesVY[i] * speedMult;
        
        // Reset if off screen
        if (particlesX[i] < 0 || particlesX[i] > 128 || particlesY[i] < 0 || particlesY[i] > 64) {
            particlesX[i] = 64;
            particlesY[i] = 32;
            float angle = random(0, 360) * PI / 180.0;
            float speed = random(5, 20) / 10.0;
            particlesVX[i] = cos(angle) * speed;
            particlesVY[i] = sin(angle) * speed;
        }
        
        u8g2.drawPixel((int)particlesX[i], (int)particlesY[i]);
    }
}

void updateVisuals() {
    displayClear();
    u8g2.setContrast(255);
    
    float beatInterval = 60000.0 / currentBpm;
    beatPhase = fmod((float)millis(), beatInterval) / beatInterval; // 0.0 to 1.0 over one beat
    
    switch (visualIndex) {
        case 0:
            drawEQBars(beatPhase, currentEnergy);
            break;
        case 1:
            drawPulse(beatPhase, currentEnergy);
            break;
        case 2:
            drawWaveform(beatPhase, currentEnergy);
            break;
        case 3:
            drawParticles(beatPhase, currentEnergy);
            break;
    }
    
    displayUpdate();
}
