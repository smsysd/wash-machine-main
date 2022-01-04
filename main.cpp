#include "extboard/ExtBoard.h"
#include "general-tools/general_tools.h"
#include "jparser-linux/JParser.h"
#include "utils/utils.h"
#include "utils/button_driver.h"
#include "utils/render.h"
#include "utils/bonus.h"

#include <sstream>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <ctime>

using namespace std;
using namespace utils;
using namespace button_driver;
using namespace bonus;
using namespace render;
using json = nlohmann::json;

const int tErrorFrame = 4;
const int tLogoFrame = 3;

CardInfo bonusCard;
CardInfo rfidCard;
bool isBonusBegin = false;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(Button& button);
void onCard(uint64_t cardid);
void onQr(const char* qr);
void onServiceEnd();

int main(int argc, char const *argv[]) {

	init(onCashAppeared, onCashRunout, onButtonPushed, onQr, onCard, onServiceEnd);
	showTempFrame(SpecFrame::LOGO, tLogoFrame);
	setGiveMoneyMode();
	
	while (true) {

	}

	return 0;
}

void onCashAppeared() {
	setProgram(0);
}

void onCashRunout() {
	if (isBonusBegin) {
		isBonusBegin = false;
		accrueRemainBonusesAndClose();
	}
	setGiveMoneyMode();
}

void onButtonPushed(Button& button) {
	if (cmode() == Mode::PROGRAM) {
		switch (button.type) {
		case Button::END:
			if (isBonusBegin) {
				isBonusBegin = false;
				accrueRemainBonusesAndClose();
			}
			setGiveMoneyMode();
			break;
		case Button::PROGRAM:
			setProgram(button.prog);
			break;
		}
	} else
	if (cmode() == Mode::SERVICE) {
		switch (button.type) {
		case Button::END:
			setGiveMoneyMode();
			break;
		case Button::PROGRAM:
			setServiceProgram(button.serviceProg);
			break;
		}
	}
}

void onQr(const char* qr) {
	if (isBonusBegin) {
		// TODO (if another qr ?? (if old client ??))
		writeOffBonuses();
	} else {
		bool rc = startBonuses(bonusCard, qr);
		if (!rc) {
			showTempFrame(SpecFrame::BONUS_ERROR, tErrorFrame);
			return;
		}
		isBonusBegin = true;
		if (bonusCard.type == CardInfo::BONUS_ORG || bonusCard.type == CardInfo::BONUS_PERS) {
			rc = writeOffBonuses();
			if (!rc) {
				showTempFrame(SpecFrame::BONUS_ERROR, tErrorFrame);
				return;
			}
		} else
		if (bonusCard.type == CardInfo::SERVICE) {
			setServiceMode(bonusCard.id);
		} else {
			showTempFrame(SpecFrame::UNKNOWN_CARD, tErrorFrame);
		}
	}
}

void onCard(uint64_t cardid) {
	bool rc = getLocalCardInfo(rfidCard, cardid);
	if (!rc) {
		showTempFrame(SpecFrame::UNKNOWN_CARD, tErrorFrame);
		return;
	}

	if (rfidCard.type == CardInfo::SERVICE) {
		setServiceMode(rfidCard.id);
	} else
	if (rfidCard.type == CardInfo::BONUS_ORG || rfidCard.type == CardInfo::BONUS_PERS) {
		// may be rfid bonus card
	} else
	if (rfidCard.type == CardInfo::UNKNOWN) {
		showTempFrame(SpecFrame::UNKNOWN_CARD, tErrorFrame);
	}
}

void onServiceEnd() {
	setGiveMoneyMode();
}