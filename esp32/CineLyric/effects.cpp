#include "effects.h"
#include <U8g2lib.h>
#include <math.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

void drawBackgroundEffect(const char* effectName, float beatPhase, float energy) {
    if (strcmp(effectName, "None") == 0) return;
    
    if (strcmp(effectName, "Burst") == 0) {
        // Particles exploding outwards
        static float px[15], py[15], pvx[15], pvy[15];
        static bool init = false;
        
        if (beatPhase < 0.1 && !init) {
            for (int i = 0; i < 15; i++) {
                px[i] = 64; py[i] = 32;
                float angle = random(0, 360) * PI / 180.0;
                float speed = random(10, 40) / 10.0;
                pvx[i] = cos(angle) * speed;
                pvy[i] = sin(angle) * speed;
            }
            init = true;
        } else if (beatPhase > 0.9) {
            init = false;
        }
        
        if (init) {
            u8g2.setDrawColor(1);
            for (int i = 0; i < 15; i++) {
                px[i] += pvx[i];
                py[i] += pvy[i];
                u8g2.drawPixel((int)px[i], (int)py[i]);
            }
        }
    } else if (strcmp(effectName, "Drift") == 0) {
        // Snow/drift falling down
        static float px[20], py[20];
        static bool init = false;
        
        if (!init) {
            for (int i = 0; i < 20; i++) {
                px[i] = random(0, 128);
                py[i] = random(0, 64);
            }
            init = true;
        }
        
        u8g2.setDrawColor(1);
        for (int i = 0; i < 20; i++) {
            py[i] += energy * 2.0;
            if (py[i] > 64) {
                py[i] = 0;
                px[i] = random(0, 128);
            }
            u8g2.drawPixel((int)px[i], (int)py[i]);
        }
    }
}

void applyCameraEffect(const char* effectName, float beatPhase, float energy, int& xOffset, int& yOffset) {
    xOffset = 0;
    yOffset = 0;
    
    if (strcmp(effectName, "Shake") == 0) {
        int maxShake = 1 + (int)(energy * 8);
        xOffset = random(-maxShake, maxShake + 1);
        yOffset = random(-maxShake, maxShake + 1);
    } else if (strcmp(effectName, "Pulse") == 0) {
        // A slight bounce on the Y axis
        yOffset = (int)(sin(beatPhase * PI) * 5 * energy);
    }
}
