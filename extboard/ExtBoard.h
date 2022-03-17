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

	enum class SpecEffect {
		GIVE_MONEY_EFFECT,
		SERVICE_EFFECT,
		WAIT
	};
	enum class ErrorType {
		NONE,
		DISCONNECT_DEV,
		INTERNAL
	};

	void init(json& extboard, json& performingUnits, json& relaysGroups, json& payment, json& buttons, json& rangeFinder, json& tempSens, json& leds, json& effects, json& specEffects, json& releiveInstructions);
	
	/* Light control no exceptions */
	void startLightEffect(int id, int index);
	void startLightEffect(SpecEffect effect, int index);
	void resetLightEffect(int index);

	/* Performing functions no exceptions */
	void relievePressure();
	void setRelayGroup(int iGroup);
	void setRelaysState(int address, int states);

	/* Other control functions no exceptions */
	void flap(bool state);

	/* Event handlers registration */
	void registerOnButtonPushedHandler(void (*handler)(int iButton));
	void registerOnCardReadHandler(void (*handler)(uint64_t cardid));
	void registerOnMoneyAddedHandler(void (*handler)(double nMoney));
	void registerOnObjectCloserHandler(void (*handler)(bool state));
	void registerOnErrorHandler(void (*handler)(ErrorType et, string text));

	// * * * DEPRICATED * * *
	// up cash from temp file, it was created when error initialize and read money before reset not null
	// void up_cash();
}

#endif