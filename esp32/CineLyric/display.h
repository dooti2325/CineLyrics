#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

extern bool isDisplayInverted;

void displaySetup();
void displayClear();
void displayUpdate();

const uint8_t* autoSelectFont(const char* text, int maxWidth);
void drawScrollingText(const char* text, int y, const uint8_t* font);
void drawWrappedText(const char* text, int y, int maxWidth, const uint8_t* font);
void drawLyrics(const char* text, int yOffset = 0);
void drawTitleArtist(const char* title, const char* artist);
void drawCenteredText(const char* text, int y, const uint8_t* font);
void drawText(const char* text, int x, int y, const uint8_t* font);

#endif // DISPLAY_H
