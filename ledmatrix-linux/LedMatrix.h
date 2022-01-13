#ifndef _LEDMATRIX_H_
#define _LEDMATRIX_H_

#include "../mb-ascii-linux/MbAsciiMaster.h"
#include "../rgb332/rgb332.h"
#include "../font-linux/Font.h"

#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

class LedMatrix {
public:
    enum class Mode {
        LEFT = 0x00,
		CENTER = 0x01
    };

    int matrixWidth;
    int matrixHeight;

    LedMatrix(int deviceAddress, string driver = "/dev/tty0", int baudRate = B9600, int dirPin = -1);
    virtual ~LedMatrix();

    void setOpt(Mode mode, int font, int color);
    void writeString(const char* str);
	void writeString(const char* str, const char* encode);
	void writePrepareBlock(int iBlock, bool sw = true);
    void writeBlock(int x, int y, RGB332& bitmap);

	void switchBuffer();
    void clear();
	void reset();
    void setCursor(int x, int y);
	void setDefaultCursor(int x, int y);

    void setBright(float bright);

	void prepareFont(Font& font, int iFont, const char* encode);
	void prepareBlock(RGB332 bitmap, int iBlock);
	void prepareBlock(string pathToImg, int iBlock);

private:
	enum class WIA {
		PROJECT = 0x8001,
		SUB_64x32 = 0x01
	};

    enum class AddressMap {
		WIA = 0x01,
		VERSION = 0x03,
		LOR = 0x05,
		STATUS = 0x06,
		STRMODE = 0x10,
		CURSOR = 0x11,
		DEFCURSOR = 0x12,
		BLOCK = 0x20,
		BRIGHT = 0x30,
		LOAD_FONT = 0x100,
		LOAD_CHAR = 0x105,
		LOAD_BLOCK = 0x200,
		LOAD_PIXELS = 0x205,
		CHECK_FONTS = 0x400,
		CHECK_BLOCKS = 0x420
    };

    enum class Cmd {
		RESET = 0x00,
		SAVE = 0x08,
        CLEAR = 0x20,
		SWITCH_BUFFER = 0x21
    };

	const int _nBlocks = 32;
	const int _nFonts = 8;

    MbAsciiMaster* _mb;
    int _deviceAddress;
	int _toSmall;
	int _toMedium;
	int _toLarge;

    void _cmd(Cmd cmd, int timeout);
	void _loadFont(Font& font, int iFont, const char* encode);
	void _loadBlock(RGB332& bitmap, int iBlock);

	uint32_t _hash(Font& font);
	uint32_t _hash(RGB332 bitmap);

	uint8_t _toCP866(int utf, const char* encode);
	void _gliphToBitmap(RGB332 gliph, int* bitmap);

	void _assertFont(int iFont);
	void _assertBlock(int iBlock);
};

#endif
