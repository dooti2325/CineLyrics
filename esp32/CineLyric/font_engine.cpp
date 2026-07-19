#include "font_engine.h"
#include <string.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

const uint8_t* getFontByStyle(const char* style) {
    if (strcmp(style, "Huge") == 0) return FONT_HUGE;
    if (strcmp(style, "Large") == 0) return FONT_LARGE;
    if (strcmp(style, "Medium") == 0) return FONT_MEDIUM;
    if (strcmp(style, "Small") == 0) return FONT_SMALL;
    
    return FONT_MEDIUM;
}

const uint8_t* getDynamicFont(const char* text, int maxWidth) {
    u8g2.setFont(FONT_HUGE);
    if (u8g2.getUTF8Width(text) <= maxWidth) return FONT_HUGE;
    
    u8g2.setFont(FONT_LARGE);
    if (u8g2.getUTF8Width(text) <= maxWidth) return FONT_LARGE;
    
    u8g2.setFont(FONT_MEDIUM);
    if (u8g2.getUTF8Width(text) <= maxWidth) return FONT_MEDIUM;
    
    return FONT_SMALL;
}
