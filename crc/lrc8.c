#include "crc.h"

#include <stdint.h>

static uint8_t _lrc8dv = LRC8_INIT;

uint8_t lrc8(uint8_t* data, uint32_t nData) {
    uint8_t crc = LRC8_INIT;

	while (nData--){
		crc += *data++;   /* Add buffer byte without carry */
	}

	crc = (uint8_t)(-((int8_t)crc));
	return crc;
}

void lrc8d_reset() {
	_lrc8dv = LRC8_INIT;
}

void lrc8d(uint8_t data) {
	_lrc8dv += data;
}

uint8_t lrc8d_get() {
	return (uint8_t)(-((int8_t)_lrc8dv));
}