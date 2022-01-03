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

namespace extboard {

	enum class LightEffectType {
		MAIN = 0x01,
		SUB = 0x02
	};

	enum class SpecEffect {
		GIVE_MONEY_EFFECT,
		SERVICE_EFFECT
	};

	void init(json& extboard, json& performingUnits, json& relaysGroups, json& buttons, json& rangeFinder, json& tempSens, json& leds, json& effects, json& releiveInstructions);
	
	/* Light control */
	void startLightEffect(int id, LightEffectType type);
	void startLightEffect(SpecEffect effect, LightEffectType type);
	void resetLightEffect(LightEffectType type);

	/* Performing functions */
	void relievePressure();
	void setRelayGroup(int iGroup);
	void setRelaysState(int address, int states);

	/* Event handlers registration */
	void registerOnButtonPushedHandler(void (*handler)(int id));
	void registerOnCardReadHandler(void (*handler)(uint64_t cardid));
	void registerOnMoneyAddedHandler(void (*handler)(double nMoney));
	void registerOnObjectCloserHandler(void (*handler)(bool state));
}

#endif