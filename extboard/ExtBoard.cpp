#include "ExtBoard.h"
#include "../mspi-linux/Mspi.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <time.h>

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
	
	enum class Addr {
		WIA = 0x00,
		VERSION = 0x03,
		LOR = 0x05,
		STATUS = 0x06,
		INTREASON = 0x07,
		RELAY_GROUPS = 0x10,
		DIRECT_SET = 0xD0,
		SET_GROUP = 0xD2,
		PERFORMERS_OPT = 0xE0,
		COUNTERS = 0x100,
		BUTTONS_OPT = 0x110,
		BUTTONS = 0x130,
		TEMPSENS_OPT = 0x140,
		TEMPSENS = 0x150,
		RFID_CARD = 0x160,
		RANGEFINDER_OPT = 0x170,
		RANGEFINDER = 0x171,
		FLAP = 0x172,
		EFFECT = 0x200,
		PERFORMERS_INFO = 0x1000,
		BUTTONS_INFO = 0x1060
	};

	enum class Cmd {
		RESET = 0x00,
		SAVE = 0x08,
		RELIEVE_PRESSURE = 0x10,
		OPEN_FLAP = 0x11,
		CLOSE_FLAP = 0x12,
		RESET_MAIN_EFFECT = 0x20,
		RESET_SUB_EFFECT = 0x21
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

	struct PerformerUnit {
		uint8_t addr;
		uint8_t normalStates;
		uint8_t dependencies[6];
	};

	struct RelaysGroup {
		uint8_t addr[4];
		uint8_t state[4];
	};

	Mspi* _mspi = nullptr;
	vector<LightEffect> _effects;
	int _giveMoneyEffect = -1;
	int _serviceEffect = -1;

	void _int(int reason) {

	}
}

void init(json& extboard, json& performingUnits, json& relaysGroups, json& buttons, json& leds, json& effects) {
	// get hw extboard config
	cout << "load hardware extboard config.." << endl;
	string driver = JParser::getf(extboard, "driver", "extboard");
	int speed = JParser::getf(extboard, "speed", "extboard");
	int csPin = JParser::getf(extboard, "cs-pin", "extboard");
	int intPin = JParser::getf(extboard, "int-pin", "extboard");

	// create mspi connection
	cout << "create MSPI connection.." << endl;
	_mspi = new Mspi(driver, speed, csPin, intPin, _int);
	uint8_t buf[1024];
	_mspi->read((int)Addr::WIA, buf, 3);
	_mspi->read((int)Addr::VERSION, &buf[3], 2);
	cout << "extboard connected, WIA_PRJ: " << (buf[0] << 8 | buf[1]) << ", WIA_SUB: " << buf[2] << ", VERSION: [" << buf[3] << ", " << buf[4] << "]" << endl;
	while (true) {
		_mspi->read((int)Addr::STATUS, buf, 1);
		if (buf[0] > 1) {
			break;
		}
	}

	cout << "load performing and buttons config.." << endl;

	cout << "compare options.." << endl;
	_mspi->read((int)Addr::RELAY_GROUPS, buf, )
}

/* Light control */
void startLightEffect(int id, LightEffectType type) {

}

void startLightEffect(SpecEffect effect, LightEffectType type) {
	int id = -1;
	switch (effect) {
	case SpecEffect::GIVE_MONEY_EFFECT: id = _giveMoneyEffect; break;
	case SpecEffect::SERVICE_EFFECT: id = _serviceEffect; break;
	}
	startLightEffect(id, type);
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