#ifndef FONT_ENGINE_H
#define FONT_ENGINE_H

#include <U8g2lib.h>

// Available fonts (assumes they are linked in U8g2)
#define FONT_HUGE u8g2_font_helvB18_te
#define FONT_LARGE u8g2_font_helvB14_te
#define FONT_MEDIUM u8g2_font_helvB10_te
#define FONT_SMALL u8g2_font_helvB08_tr

const uint8_t* getFontByStyle(const char* style);
const uint8_t* getDynamicFont(const char* text, int maxWidth);

#endif // FONT_ENGINE_H
