#include "crc.h"

#include <stdint.h>


uint8_t crc8(uint8_t* data, uint32_t nData) {
    uint8_t crc = CRC8_INIT;
    uint32_t i;
    uint8_t j;

    for (i = 0; i < nData; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ CRC8_POLINOME);
            else
                crc <<= 1;
        }
    }
    return crc;
}
