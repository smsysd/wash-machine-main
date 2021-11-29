/* Author: Jodzik */
#ifndef EXT_BOARD_H_
#define EXT_BOARD_H_

#include "../mspi-linux/Mspi.h"
#include "../json.h"
#include "../rgb332/rgb332.h"

#include <vector>
#include <string>
#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace ExtBoard {
	namespace {
		enum class LightCmd {
			WRITE0_BM = 0x02,
			WRITE1_BM = 0x01,
			WRITE0_LP = 0x04,
			WRITE1_LP = 0x03,
			WRITE0_IL = 0x06,
			WRITE1_IL = 0x05,
			DELAY = 0x10
		};

		enum class LightEffectType {
			MAIN = 0x01,
			SUB = 0x02
		};

		struct LightInstruction{
			LightCmd cmd;
			uint8_t data0;
			uint8_t data1;
			
		};

		struct LightEffect {
			int id;
			LightEffectType type;
			uint16_t delay;
			uint8_t nRepeats;
			LightInstruction instructions[1024];
		};
	}

	enum class DisplayStringMode {
		DEFAULT = 0x00,
		CENTER = 0x01
	};

	void init(json& extboard, json& performingSystem, json& buttons, json& display, json& leds, json& effects);
	
	/* Light control */
	void setLightEffect(int id);
	void resetLightEffect(LightEffectType type);

	/* Event handlers registration */
	void registerOnButtonPushedHandler(void (*handler)(int iButton));
	void registerOnEffectEndHandler(void (*handler)(int iEffect));
	void registerOnCardReadHandler(void (*handler)(const char* uid));
	void registerOnCashAddedHandler(void (*handler)(int nCash));
	void registerOnObjectCloserHandler(void (*handler)());

	/* Performer unit functions */
	void relievePressure();
	void setProgram(int iProg);
	void setRelaysState(int address, int states);

	/* Display functions */
	void clearDisplay();
	void printBlock(int x, int y, int iBlock);
	void print(int x, int y, int color, char* str);
	void printFrame(int iFrame);
	void loadChar(int font, int iChar, RGB332 bitmap);
	void loadBlock(int iBlock, RGB332 bitmap);

}

#endif