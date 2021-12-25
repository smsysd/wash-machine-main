#include "LedMatrix.h"
#include "../font-linux/Font.h"
#include "../rgb332/rgb332.h"
#include "../crc/crc.h"

#include <stdint.h>
#include <vector>
#include <string>
#include <string.h>
#include <locale>
#include <codecvt>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <iconv.h>

using namespace std;

LedMatrix::LedMatrix(int deviceAddress, string driver, int baudRate, int dirPin) {
    _mb = new MbAsciiMaster(dirPin, driver.c_str(), baudRate);
    _deviceAddress = deviceAddress;
	_toSmall = 100 + (12000 / (baudRate * baudRate * 11) * 10);
	_toMedium = 200 + _toSmall * 1.5;
	_toLarge = 400 + _toSmall * 3;

	int buf[10];
	_mb->rread(_deviceAddress, (int)AddressMap::WIA, buf, 2, _toLarge);
	if ((WIA)buf[0] != WIA::PROJECT) {
		throw runtime_error("device by addr is not ledmatrix");
	}
	switch ((WIA)buf[1]) {
	case WIA::SUB_64x32:
		matrixWidth = 64;
    	matrixHeight = 32;	
		break;
	default: throw runtime_error("unknown ledmatrix type");
	}
}

LedMatrix::~LedMatrix() {
    delete _mb;
}

void LedMatrix::writeBlock(int x, int y, RGB332& bitmap) {

}

void LedMatrix::switchBuffer() {
	_cmd(Cmd::SWITCH_BUFFER, _toSmall);
}

void LedMatrix::clear() {
    _cmd(Cmd::CLEAR, _toMedium);
}

void LedMatrix::setCursor(int x, int y) {
	int buf[10];
	buf[0] = (uint8_t)x | (y << 8);
	_mb->rwrite(_deviceAddress, (int)AddressMap::CURSOR, buf, 1, _toSmall);
}

void LedMatrix::setDefaultCursor(int x, int y) {
	int buf[10];
	buf[0] = (uint8_t)x | (y << 8);
	_mb->rwrite(_deviceAddress, (int)AddressMap::DEFCURSOR, buf, 1, _toSmall);
}

void LedMatrix::setOpt(Mode mode, int font, int color) {
	int buf[10];
	buf[0] = (int)mode;
	buf[0] |= (font & 0x07) << 4;
	buf[0] |= color << 8;
	_mb->rwrite(_deviceAddress, (int)AddressMap::STRMODE, buf, 1, _toSmall);
}

void LedMatrix::writeString(char* str) {
	_mb->str(_deviceAddress, str, _toLarge);
}

void LedMatrix::writeString(string str) {
	char outbuf[512];
	char* ip = (char *) str.c_str();
	char* op = outbuf;
	size_t icount = str.length();
	size_t ocount = 512;
	iconv_t cnv;

	if (icount >= 512) {
		throw runtime_error("too many");
	}
	cnv = iconv_open("CP866", "UTF-8");
	if (cnv == (iconv_t) - 1) {
		throw runtime_error("fail convert");
	}

	if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t) - 1) {
		outbuf[512 - ocount] = '\0';
	} else {
		iconv_close(cnv);
		throw runtime_error("fail to convert");
	}

	_mb->str(_deviceAddress, outbuf, _toLarge);
}

void LedMatrix::writePrepareBlock(int iBlock) {
	int buf[10];
	buf[0] = iBlock;
	_mb->rwrite(_deviceAddress, (int)AddressMap::CURSOR, buf, 1, _toMedium);
}

void LedMatrix::setBright(float bright) {
	int buf[10];
	buf[0] = (uint8_t)(bright * 255.0);
	_mb->rwrite(_deviceAddress, (int)AddressMap::BRIGHT, buf, 1, _toSmall);
}

void LedMatrix::prepareFont(Font& font, int iFont) {
	int buf[_nFonts*2];
	_assertFont(iFont);
	_mb->rread(_deviceAddress, (int)AddressMap::CHECK_FONTS, buf, _nFonts * 2, _toLarge);
	uint32_t rh = (buf[iFont*2] << 8) | buf[iFont*2 + 1];
	uint32_t ch = _hash(font);
	if (rh != ch) {
		_loadFont(font, iFont);
	}
}

void LedMatrix::prepareBlock(RGB332 bitmap, int iBlock) {
	int buf[_nBlocks*2];
	_assertFont(iBlock);
	_mb->rread(_deviceAddress, (int)AddressMap::CHECK_FONTS, buf, _nBlocks * 2, _toLarge);
	uint32_t rh = (buf[iBlock*2] << 8) | buf[iBlock*2 + 1];
	uint32_t ch = _hash(bitmap);
	if (rh != ch) {
		_loadBlock(bitmap, iBlock);
	}
}

void LedMatrix::_loadFont(Font& font, int iFont) {
	int buf[256];
	
	buf[0] = iFont;
	buf[1] = font.getMaxHeight();
	uint32_t h = _hash(font);
	buf[2] = h >> 16;
	buf[3] = (uint16_t)h;
	_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_FONT, buf, 4, _toLarge);

	vector<int> chars = font.getCharCodes();
	RGB332 bm;
	int n;
	for (int i = 0; i < chars.size(); i++) {
		bm = font.getGliph(chars[i]);
		buf[0] = bm.width;
		buf[1] = _toCP866(chars[i]);
		n = _gliphToBitmap(bm, &buf[2], 254);
		_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_CHAR, buf, n + 2, _toLarge);
	}
	_mb->cmd(_deviceAddress, (int)Cmd::SAVE, _toLarge);
}

void LedMatrix::_loadBlock(RGB332& bitmap, int iBlock) {
	int buf[10000];
	
	buf[0] = iBlock;
	buf[1] = bitmap.width | (bitmap.height << 8);
	uint32_t h = _hash(bitmap);
	buf[2] = h >> 16;
	buf[3] = (uint16_t)h;
	_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_BLOCK, buf, 4, _toLarge);

	int n = bitmap.width * bitmap.height;
	bool mf;
	if (n % 2 == 0) {
		mf = false;
	} else {
		mf = true;
	}
	n /= 2;
	for (int i = 0; i < n; i++) {
		buf[i] = (bitmap.data[i*2] << 8) | bitmap.data[i*2 + 1];
	}
	if (mf) {
		buf[n] = (bitmap.data[n*2] << 8);
		n++;
	}
	int cl;
	int bi = 0;
	while (true) {
		if (n > 127) {
			cl = 127;
		} else {
			cl = n;
		}
		_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_PIXELS, &buf[bi], cl, _toLarge);
		n -= cl;
		bi += cl;
		if (n <= 0) {
			break;
		}
	}
	_mb->cmd(_deviceAddress, (int)Cmd::SAVE, _toLarge);
}

void LedMatrix::_cmd(Cmd cmd, int timeout) {
	_mb->cmd(_deviceAddress, (int)cmd, timeout);
}

uint32_t LedMatrix::_hash(Font& font) {
	vector<int> chars = font.getCharCodes();
	uint32_t h = 0;
	int buff[256];
	int n;
	for (int i = 0; i < chars.size(); i++) {
		n = _gliphToBitmap(font.getGliph(chars[i]), buff, 256);
		h += crc32((const uint8_t*)buff, n*sizeof(int), 0x04C11DB7);
	}
	if (h == 0) {
		h = 1;
	}

	return h;
}

uint32_t LedMatrix::_hash(RGB332 bitmap) {
	uint32_t h = crc32(bitmap.data, bitmap.width * bitmap.height, 0x04C11DB7);
	if (h == 0) {
		h = 1;
	}
	return h;
}

uint8_t LedMatrix::_toCP866(int utf) {
	char outbuf[10];
	char inbuf[10];
	char* ip = inbuf;
	char* op = outbuf;
	size_t icount = 2;
	size_t ocount = 10;
	iconv_t cnv;

	cnv = iconv_open("CP866", "UTF-8");
	if (cnv == (iconv_t) - 1) {
		throw runtime_error("fail convert");
	}

	inbuf[0] = (uint8_t)(utf >> 8);
	inbuf[1] = (uint8_t)(utf);
	inbuf[2] = 0;
	if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t) - 1) {
		outbuf[10 - ocount] = '\0';
	} else {
		iconv_close(cnv);
		throw runtime_error("fail to convert");
	}

	return outbuf[0];
}

int LedMatrix::_gliphToBitmap(RGB332 gliph, int* bitmap, int nBitmap) {
	int B = 0;
	int b = 0;
	bitmap[0] = 0;
	for (int h = 0; h < gliph.height; h++) {
		for (int w = 0; w < gliph.width; w++) {
			if (rgb332_get(gliph, w, h) > 0x7F) {
				bitmap[B] |= 0x8000 >> b;
			}
			b++;
			if (b > 15) {
				b = 0;
				B++;
				bitmap[B] = 0;
				if (B >= nBitmap) {
					throw runtime_error("overflow");
				}
			}
		}
	}
	if (b == 0) {
		return B;
	} else {
		return B + 1;
	}
}

void LedMatrix::_assertFont(int iFont) {
	if (iFont >= _nFonts) {
		throw runtime_error("incorrect font index");
	}
}

void LedMatrix::_assertBlock(int iBlock) {
	if (iBlock >= _nBlocks) {
		throw runtime_error("incorrect block index");
	}
}
