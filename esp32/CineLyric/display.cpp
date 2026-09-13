#include "display.h"
#include "config.h"

// Initialize U8G2 object for SH1106 (Full frame buffer)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

void displaySetup() {
    u8g2.begin();
    u8g2.setContrast(255);
}

void displayClear() {
    u8g2.clearBuffer();
}

void displayUpdate() {
    u8g2.sendBuffer();
}

const uint8_t* fonts[] = {
    u8g2_font_logisoso20_tf,
    u8g2_font_logisoso16_tf,
    u8g2_font_ncenB14_tr,
    u8g2_font_ncenB10_tr,
    u8g2_font_helvB08_tr,
    u8g2_font_6x12_tf
};
const int numFonts = 6;

const uint8_t* autoSelectFont(const char* text, int maxWidth) {
    for (int i = 0; i < numFonts; i++) {
        u8g2.setFont(fonts[i]);
        if (u8g2.getUTF8Width(text) <= maxWidth) {
            return fonts[i];
        }
    }
    return fonts[numFonts - 1]; // Smallest fallback
}

void drawScrollingText(const char* text, int y, const uint8_t* font) {
    u8g2.setFont(font);
    int width = u8g2.getUTF8Width(text);
    int maxW = 120;
    
    if (width <= maxW) {
        int x = (128 - width) / 2;
        u8g2.drawUTF8(x, y, text);
        return;
    }
    
    long t = millis() / 25; // scroll speed
    int scrollOffset = t % (width + 64);
    int x = 64 - scrollOffset;
    u8g2.drawUTF8(x, y, text);
}

void drawWrappedText(const char* text, int y, int maxWidth, const uint8_t* font) {
    u8g2.setFont(font);
    int width = u8g2.getUTF8Width(text);
    
    if (width <= maxWidth) {
        int x = (128 - width) / 2;
        u8g2.drawUTF8(x, y, text);
        return;
    }
    
    int len = strlen(text);
    int splitPos = len / 2;
    int bestSplit = -1;
    
    for (int i = 0; i < len; i++) {
        if (text[i] == ' ') {
            if (bestSplit == -1 || abs(i - splitPos) < abs(bestSplit - splitPos)) {
                bestSplit = i;
            }
        }
    }
    
    if (bestSplit != -1 && bestSplit < 64 && (len - bestSplit) < 64) {
        char line1[64];
        char line2[64];
        strncpy(line1, text, bestSplit);
        line1[bestSplit] = '\0';
        strncpy(line2, text + bestSplit + 1, sizeof(line2) - 1);
        line2[sizeof(line2) - 1] = '\0';
        
        int w1 = u8g2.getUTF8Width(line1);
        int w2 = u8g2.getUTF8Width(line2);
        int h = u8g2.getMaxCharHeight();
        
        if (w1 > maxWidth || w2 > maxWidth) {
            drawScrollingText(text, y, font);
        } else {
            u8g2.drawUTF8((128 - w1) / 2, y - (h/2) - 1, line1);
            u8g2.drawUTF8((128 - w2) / 2, y + (h/2) + 1, line2);
        }
    } else {
        drawScrollingText(text, y, font);
    }
}

void drawLyrics(const char* text, int yOffset) {
    if (strlen(text) == 0) return;
    int maxW = 120;
    const uint8_t* bestFont = autoSelectFont(text, maxW);
    
    u8g2.setFont(bestFont);
    if (u8g2.getUTF8Width(text) > maxW) {
        // Even the smallest font is too big, attempt to wrap it with a legible font
        drawWrappedText(text, 36 + yOffset, maxW, u8g2_font_helvB08_tr);
    } else {
        int x = (128 - u8g2.getUTF8Width(text)) / 2;
        u8g2.drawUTF8(x, 36 + yOffset, text);
    }
}

void drawTitleArtist(const char* title, const char* artist) {
    int maxW = 120;
    
    if (strlen(title) > 0) {
        const uint8_t* titleFont = autoSelectFont(title, maxW);
        u8g2.setFont(titleFont);
        if (u8g2.getUTF8Width(title) > maxW) {
            drawScrollingText(title, 30, u8g2_font_ncenB10_tr);
        } else {
            int x = (128 - u8g2.getUTF8Width(title)) / 2;
            u8g2.drawUTF8(x, 30, title);
        }
    }
    
    if (strlen(artist) > 0) {
        u8g2.setFont(u8g2_font_6x12_tf);
        if (u8g2.getUTF8Width(artist) > maxW) {
            drawScrollingText(artist, 50, u8g2_font_6x12_tf);
        } else {
            int x = (128 - u8g2.getUTF8Width(artist)) / 2;
            u8g2.drawUTF8(x, 50, artist);
        }
    }
}

void drawCenteredText(const char* text, int y, const uint8_t* font) {
    u8g2.setFont(font);
    int width = u8g2.getUTF8Width(text);
    int x = (128 - width) / 2;
    if (x < 0) x = 0;
    u8g2.drawUTF8(x, y, text);
}

void drawText(const char* text, int x, int y, const uint8_t* font) {
    u8g2.setFont(font);
    u8g2.drawUTF8(x, y, text);
}
