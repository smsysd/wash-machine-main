#ifndef _CRC_H_
#define _CRC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* CRC8 RefIn: false, RefOut: false*/
#define CRC8_INIT           0x00
#define CRC8_POLINOME       0x07

/* LRC8 */
#define LRC8_INIT           0x00

/* CRC16 polinome: 0x8005 */
#define CRC16_INIT          0xFFFF

uint8_t crc8(uint8_t* data, uint32_t nData);

uint8_t lrc8(uint8_t* data, uint32_t nData);
void lrc8d_reset();
void lrc8d(uint8_t data);
uint8_t lrc8d_get();

uint16_t crc16(uint8_t* data, uint32_t nData);

void crc32_init(uint32_t polynome, uint32_t initial, uint32_t xorOut);
uint32_t crc32(const uint8_t* data, uint32_t nData, uint32_t polynome);

#ifdef __cplusplus
}
#endif

#endif