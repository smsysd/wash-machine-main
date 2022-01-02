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

	void init(json& extboard, json& performingUnits, json& relaysGroups, json& buttons, json& leds, json& effects);
	
	/* Light control */
	void startLightEffect(int id, LightEffectType type);
	void startLightEffect(SpecEffect effect, LightEffectType type);
	void resetLightEffect(LightEffectType type);

	/* Performing functions */
	void relievePressure();
	void setRelayGroup(int iGroup);
	void setRelaysState(int address, int states);

	/* Event handlers registration */
	void registerOnButtonPushedHandler(void (*handler)(int iButton));
	void registerOnEffectEndHandler(void (*handler)(int iEffect));
	void registerOnCardReadHandler(void (*handler)(uint64_t cardid));
	void registerOnCashAddedHandler(void (*handler)(double nCash));
	void registerOnObjectCloserHandler(void (*handler)());
}

#endif