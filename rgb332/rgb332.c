#include "rgb332.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifdef _cplusplus
extern "C" {
#endif

RGB332 rgb332_create(uint32_t width, uint32_t height) {
    RGB332 bitmap;
    bitmap.height = height;
    bitmap.width = width;
    bitmap.data = (uint8_t*)calloc(width * height, sizeof(uint8_t));
    
    return bitmap;
}

void rgb332_delete(RGB332 bitmap) {
    free(bitmap.data);
}

uint8_t rgb332_formColor(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t result;

    rgb332_injectR(&result, r);
    rgb332_injectG(&result, g);
    rgb332_injectB(&result, b);

    return result;
}

void rgb332_set(RGB332 bitmap, uint32_t x, uint32_t y, uint8_t color) {
    if (bitmap.data == NULL) {
        return;
    }
    if (x >= bitmap.width) {
        return;
    }
    if (y >= bitmap.height) {
        return;
    }

    bitmap.data[y * bitmap.width + x] = color;
}

uint8_t rgb332_get(RGB332 bitmap, uint32_t x, uint32_t y) {
    if (bitmap.data == NULL) {
        return 0;
    }
    if (x >= bitmap.width) {
        return 0;
    }
    if (y >= bitmap.height) {
        return 0;
    }

    return bitmap.data[y * bitmap.width + x];
}

uint8_t rgb332_mix(uint8_t color1, uint8_t color2, float balance) {
    uint8_t result;
    float ibalance;
    uint8_t r1;
    uint8_t g1;
    uint8_t b1;
    uint8_t r2;
    uint8_t g2;
    uint8_t b2;
    float rr;
    float rg;
    float rb;

    if (balance > 1) {
        balance = 1;
    }

    r1 = rgb332_extractR(color1);
    g1 = rgb332_extractG(color1);
    b1 = rgb332_extractB(color1);
    r2 = rgb332_extractR(color2);
    g2 = rgb332_extractG(color2);
    b2 = rgb332_extractB(color2);
    ibalance = 1 - balance;

    rr = (float)r1 * balance + (float)r2 * ibalance;
    rg = (float)g1 * balance + (float)g2 * ibalance;
    rb = (float)b1 * balance + (float)b2 * ibalance;


    rgb332_injectR(&result, (uint8_t)roundf(rr));
    rgb332_injectG(&result, (uint8_t)roundf(rg));
    rgb332_injectB(&result, (uint8_t)roundf(rb));

    return result;
}

uint8_t rgb332_maximize(uint8_t color, uint8_t threshold) {
    uint8_t result;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    r = rgb332_extractR(color);
    g = rgb332_extractG(color);
    b = rgb332_extractB(color);

    if (r >= threshold) {
        r = 0x07;
    } else {
        r = 0;
    }

    if (g >= threshold) {
        g = 0x07;
    } else {
        g = 0;
    }

    if (b >= (threshold + 1) / 2) {
        b = 0x03;
    } else {
        b = 0;
    }

    rgb332_injectR(&result, r);
    rgb332_injectG(&result, g);
    rgb332_injectB(&result, b);

    return result;
}

void rgb332_fill(RGB332 bitmap, uint8_t color) {
	uint32_t size;
	uint32_t i;

	if (bitmap.data = NULL) {
		return;
	}
	size = bitmap.width * bitmap.height;
	for (i = 0; i < size; i++) {
		bitmap.data[i] = color;
	}
}

uint8_t rgb332_extractR(uint8_t color) {
    return (color >> 5) & 0x07;
}

uint8_t rgb332_extractG(uint8_t color) {
    return (color >> 2) & 0x07;
}

uint8_t rgb332_extractB(uint8_t color) {
    return color & 0x03;
}

void rgb332_injectR(uint8_t* color, uint8_t r) {
    if (r > 0x07) {
        r = 0x07;
    }

    *color = *color & ~RGB332_R_MASK;
    *color = *color | (r << 5);
}

void rgb332_injectG(uint8_t* color, uint8_t g) {
    if (g > 0x07) {
        g = 0x07;
    }

    *color = *color & ~RGB332_G_MASK;
    *color = *color | (g << 2);
}

void rgb332_injectB(uint8_t* color, uint8_t b) {
    if (b > 0x03) {
        b = 0x03;
    }

    *color = *color & ~RGB332_B_MASK;
    *color = *color | b;
}

uint32_t rgb332_toRGB888(uint8_t color) {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint32_t result;

    r = rgb332_extractR(color);
    g = rgb332_extractG(color);
    b = rgb332_extractB(color);
    result = 0;
    result |= (r * 36) << 16;
    result |= (g * 36) << 8;
    result |= (b * 85);

    return result;
}

#ifdef _cplusplus
}
#endif
