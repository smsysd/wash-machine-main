#include "crc.h"

#include <stdint.h>

uint32_t crc32(const uint8_t* data, uint32_t nData, uint32_t polynome) {
	uint32_t crc = -1;
	uint32_t rp = 0;

	for (int i = 0; i < 32; i++) {
		if (polynome & (1 << i)) {
			rp |= 1 << (31 - i);
		}
	}

	while(nData--) {
		crc = crc ^ *data++;
		for(int bit = 0; bit < 8; bit++ ) {
			if(crc & 1) {
				crc = (crc >> 1) ^ rp;
			} else {
				crc = (crc >> 1);
			}
		}
	}
	return ~crc;
}