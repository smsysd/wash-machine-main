#include "LedMatrix.h"
#include "../font-linux/Font.h"
#include "../rgb332/rgb332.h"
#include "../crc/crc.h"
#include "../BMP.h"

#include <stdint.h>
#include <stdio.h>
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
	_toSmall = 200 + (12000 / (baudRate * baudRate * 11) * 10);
	_toMedium = 400 + _toSmall * 2;
	_toLarge = 1000 + _toSmall * 10;

	int buf[10];
	cout << "assert WIA.." << endl;
	_mb->rread(_deviceAddress, (int)AddressMap::WIA, buf, 2, _toLarge);
	if ((WIA)buf[0] != WIA::PROJECT) {
		throw runtime_error("device by addr is not ledmatrix");
	}
	cout << "ledmatrix connected, identification.." << endl;
	switch ((WIA)buf[1]) {
	case WIA::SUB_64x32:
		matrixWidth = 64;
    	matrixHeight = 32;
		cout << "ledmatrix is 64x32" << endl;
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

void LedMatrix::reset() {
	_cmd(Cmd::RESET, _toLarge);
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
	buf[0] |= (color & 7) << 7;
	_mb->rwrite(_deviceAddress, (int)AddressMap::STRMODE, buf, 1, _toSmall);
}

void LedMatrix::writeString(const char* str) {
	char buf[128] = {0};
	strcpy(buf, str);
	_mb->str(_deviceAddress, buf, _toLarge + 10*strlen(str));
}

void LedMatrix::writeString(const char* str, const char* encode) {
	char outbuf[256];
	char* ip = (char*)str;
	char* op = outbuf;
	size_t icount = strlen(str);
	size_t ocount = 256;
	iconv_t cnv;

	if (icount >= 256) {
		throw runtime_error("too many");
	}
	cnv = iconv_open("CP866", encode);
	if (cnv == (iconv_t) - 1) {
		throw runtime_error("fail to begin convert");
	}

	size_t rc = iconv(cnv, &ip, &icount, &op, &ocount);
	if (rc != (size_t) - 1) {
		outbuf[256 - ocount] = '\0';
		iconv_close(cnv);
	} else {
		iconv_close(cnv);
		throw runtime_error("fail to convert");
	}

	_mb->str(_deviceAddress, outbuf, _toLarge + 10*icount);
}

void LedMatrix::writePrepareBlock(int iBlock, bool sw) {
	int buf[10];
	_assertBlock(iBlock);
	buf[0] = iBlock;
	if (sw) {
		buf[0] |= 0x8000;
	}
	_mb->rwrite(_deviceAddress, (int)AddressMap::BLOCK, buf, 1, _toLarge);
}

void LedMatrix::setBright(float bright) {
	int buf[10];
	buf[0] = (uint8_t)(bright * 255.0);
	_mb->rwrite(_deviceAddress, (int)AddressMap::BRIGHT, buf, 1, _toSmall);
}

void LedMatrix::prepareFont(Font& font, int iFont, const char* encode) {
	cout << "prepare font.." << endl;
	int buf[_nFonts*2];
	_assertFont(iFont);
	_mb->rread(_deviceAddress, (int)AddressMap::CHECK_FONTS, buf, _nFonts * 2, _toLarge);
	uint32_t rh = (buf[iFont*2] << 16) | buf[iFont*2 + 1];
	uint32_t ch = _hash(font);
	if (rh != ch) {
		cout << "received and calculated hash is different, r: " << rh << ", c: " << ch << endl;
		getchar();
		_loadFont(font, iFont, encode);
	}
}

void LedMatrix::prepareBlock(RGB332 bitmap, int iBlock) {
	int buf[4];
	_assertFont(iBlock);
	_mb->rread(_deviceAddress, (int)AddressMap::CHECK_BLOCKS + iBlock*2, buf, 2, _toLarge);
	uint32_t rh = (buf[0] << 16) | buf[1];
	uint32_t ch = _hash(bitmap);
	if (rh != ch) {
		cout << "received and calculated hash is different, r: " << rh << ", c: " << ch << endl;
		_loadBlock(bitmap, iBlock);
	}
}

void LedMatrix::prepareBlock(string pathToImg, int iBlock) {
	BMP bmp(pathToImg.c_str());
	int w = bmp.bmp_info_header.width;
	int h = bmp.bmp_info_header.height;
	if (bmp.bmp_info_header.bit_count != 24) {
		throw runtime_error("fail prepare block '" + pathToImg + "': image must be 24-bit encoded");
	}
	if (w > matrixWidth || h > matrixHeight) {
		throw runtime_error("fail prepare block '" + pathToImg + "': too big size");
	}
	RGB332 data = rgb332_create(w, h);
	for (int ih = 0; ih < h; ih++) {
		for (int iw = 0; iw < w; iw++) {
			int ip = ih * w * 3 + iw * 3;
			uint8_t b = bmp.data.at(ip);
			uint8_t g = bmp.data.at(ip + 1);
			uint8_t r = bmp.data.at(ip + 2);
			rgb332_set(data, iw, h - ih - 1, rgb332_fromRGB888(r, g, b));
		}
	}
	prepareBlock(data, iBlock);
}

void LedMatrix::_loadFont(Font& font, int iFont, const char* encode) {
	int buf[256];
	
	buf[0] = iFont;
	buf[1] = font.getMaxHeight();
	uint32_t h = _hash(font);
	buf[2] = h >> 16;
	buf[3] = (uint16_t)h;
	cout << "load font.." << endl;
	_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_FONT, buf, 4, _toLarge);

	vector<int> chars = font.getCharCodes();
	RGB332 bm;
	cout << "load symbols.." << endl;
	for (int i = 0; i < chars.size(); i++) {
		bm = font.getGliph(chars[i]);
		buf[0] = bm.width;
		buf[1] = _toCP866(chars[i], encode);
		_gliphToBitmap(bm, &buf[2]);
		cout << "width: " << buf[0] << ", index: " << buf[1] << endl;
		_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_CHAR, buf, 26, _toLarge);
	}
	_mb->cmd(_deviceAddress, (int)Cmd::SAVE, _toLarge);
}

void LedMatrix::_loadBlock(RGB332& bitmap, int iBlock) {
	int buf[2048];
	
	buf[0] = iBlock;
	buf[1] = bitmap.width | (bitmap.height << 8);
	cout << "load block, w: " << bitmap.width << ", h: " << bitmap.height << endl;
	uint32_t h = _hash(bitmap);
	buf[2] = h >> 16;
	buf[3] = (uint16_t)h;
	_mb->rwrite(_deviceAddress, (int)AddressMap::LOAD_BLOCK, buf, 4, _toLarge);

	int n = bitmap.width * bitmap.height;
	int j = 0;
	int k = 0;
	bool go = true;
	while (go) {
		buf[j] = 0;
		for (int i = 0; i <= 12; i += 3) {
			uint8_t c = rgb332_toRGB111(bitmap.data[k]);
			buf[j] |= c << i;
			k++;
			if (k >= n) {
				go = false;
				break;
			}
		}
		j++;
	}
	n = j;
	int cl;
	int bi = 0;
	while (true) {
		if (n > 63) {
			cl = 63;
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
	for (int i = 0; i < chars.size(); i++) {
		_gliphToBitmap(font.getGliph(chars[i]), buff);
		h += crc32((const uint8_t*)buff, 24*sizeof(int), 0x04C11DB7);
	}
	if (h == 0) {
		h = 1;
	}
	if (h == 0xFFFFFFFF) {
		h = 0xFFFFFFF0;
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

uint8_t LedMatrix::_toCP866(int utf, const char* encode) {
	char outbuf[10];
	char inbuf[10];
	char* ip = inbuf;
	char* op = outbuf;
	size_t icount = 2;
	size_t ocount = 10;
	iconv_t cnv;

	cnv = iconv_open("CP866", encode);
	if (cnv == (iconv_t) - 1) {
		throw runtime_error("fail convert");
	}

	inbuf[0] = (char)(utf & 0xFF);
	inbuf[1] = (char)(utf >> 8);
	inbuf[2] = 0;
	if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t) - 1) {
		outbuf[10 - ocount] = '\0';
	} else {
		iconv_close(cnv);
		throw runtime_error("fail to convert");
	}

	return outbuf[0];
}

void LedMatrix::_gliphToBitmap(RGB332 gliph, int* bitmap) {
	int B = 0;
	int b = 0;
	if (gliph.height > 24 || gliph.width > 16) {
		throw runtime_error("overflow");
	}
	memset(bitmap, 0, 24*sizeof(int));
	for (int h = 0; h < gliph.height; h++) {
		bitmap[h] = 0;
		for (int w = 0; w < gliph.width; w++) {
			if (rgb332_get(gliph, w, h) > 0x7F) {
				bitmap[h] |= 0x8000 >> w;
			}
		}
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
