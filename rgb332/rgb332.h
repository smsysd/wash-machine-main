#ifndef RGB332_H_
#define RGB332_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RGB332_R_MASK       0xE0
#define RGB332_G_MASK       0x1C
#define RGB332_B_MASK       0x03

typedef struct RGB332 {
    uint32_t width;
    uint32_t height;
    uint8_t* data;
} RGB332;

RGB332 rgb332_create(uint32_t width, uint32_t height);
void rgb332_delete(RGB332 bitmap);
uint8_t rgb332_formColor(uint8_t r, uint8_t g, uint8_t b);
void rgb332_set(RGB332 bitmap, uint32_t x, uint32_t y, uint8_t color);
uint8_t rgb332_get(RGB332 bitmap, uint32_t x, uint32_t y);
uint8_t rgb332_mix(uint8_t color1, uint8_t color2, float balance);
uint8_t rgb332_maximize(uint8_t color, uint8_t threshold);
void rgb332_fill(RGB332 bitmap, uint8_t color);

uint8_t rgb332_extractR(uint8_t color);
uint8_t rgb332_extractG(uint8_t color);
uint8_t rgb332_extractB(uint8_t color);
void rgb332_injectR(uint8_t* color, uint8_t r);
void rgb332_injectG(uint8_t* color, uint8_t g);
void rgb332_injectB(uint8_t* color, uint8_t b);

uint32_t rgb332_toRGB888(uint8_t color);
uint8_t rgb332_toRGB111(uint8_t color);
uint8_t rgb332_fromRGB888(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif
