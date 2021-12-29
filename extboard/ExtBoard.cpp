#include "ExtBoard.h"
#include "../mspi-linux/Mspi.h"

using namespace std;
using json = nlohmann::json;

namespace extboard {

namespace {
	enum class LightCmd {
		WRITE0_BM = 0x02,
		WRITE1_BM = 0x01,
		WRITE0_LP = 0x04,
		WRITE1_LP = 0x03,
		WRITE0_IL = 0x06,
		WRITE1_IL = 0x05,
		DELAY = 0x0A,
		NODELAY = 0x0B
	};

	struct LightInstruction{
		uint8_t cmd;
		uint8_t data;
	};

	struct LightEffect {
		int id;
		uint16_t delay;
		uint8_t nRepeats;
		LightInstruction instructions[508];
	};
}

void init(json& extboard, json& performingUnits, json& relaysGroups, json& buttons, json& leds, json& effects) {

}

/* Light control */
void startLightEffect(int id, LightEffectType type) {

}

void resetLightEffect(LightEffectType type) {

}

/* Performing functions */
void relievePressure() {

}

void setRelayGroup(int iGroup) {

}

void setRelaysState(int address, int states) {

}

/* Event handlers registration */
void registerOnButtonPushedHandler(void (*handler)(int iButton)) {

}

void registerOnEffectEndHandler(void (*handler)(int iEffect)) {

}

void registerOnCardReadHandler(void (*handler)(uint64_t cardid)) {

}

void registerOnCashAddedHandler(void (*handler)(double nCash)) {

}

void registerOnObjectCloserHandler(void (*handler)()) {

}

}