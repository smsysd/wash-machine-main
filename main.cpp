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
using json = nlohmann::json;

enum class Mode {
	GIVE_MONEY,
	PROGRAM,
	SERVICE
};

Mode mode = Mode::GIVE_MONEY;
CardInfo card;
bool isBonusBegin = false;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(Button& button);
void onCard(const char* uid);
void onQr(const char* qr);
void onServiceEnd();

int main(int argc, char const *argv[]) {

	init(onCashAppeared, onCashRunout, onButtonPushed, onQr, onCard, onServiceEnd);
	printLogoFrame();
	setGiveMoneyMode();
	
	while (true) {

	}

	return 0;
}

void onCashAppeared() {
	mode = Mode::PROGRAM;
	setProgram(0);
}

void onCashRunout() {
	isBonusBegin = false;
	setGiveMoneyMode();
	mode = Mode::GIVE_MONEY;
}

void onButtonPushed(Button& button) {
	if (mode == Mode::PROGRAM) {
		switch (button.type) {
		case Button::END:
			if (isBonusBegin) {
				isBonusBegin = false;
				accrueRemainBonusesAndClose();
			}
			setGiveMoneyMode();
			mode = Mode::GIVE_MONEY;
			break;
		case Button::PROGRAM:
			setProgram(button.prog);
			break;
		}
	} else
	if (mode == Mode::SERVICE) {
		switch (button.type) {
		case Button::END:
			setGiveMoneyMode();
			mode = Mode::GIVE_MONEY;
			break;
		case Button::PROGRAM:
			setServiceProgram(button.serviceProg);
			break;
		}
	}
}

void onQr(const char* qr) {
	if (isBonusBegin) {
		// TODO (if another qr ??)
		writeOffBonuses();
	} else {
		bool rc = startBonuses(card, qr);
		if (!rc) {
			printErrorFrame(ErrorFrame::BONUS_ERROR);
			return;
		}
		isBonusBegin = true;
		if (card.type == CardInfo::BONUS_ORG || card.type == CardInfo::BONUS_PERS) {
			rc = writeOffBonuses();
			if (!rc) {
				printErrorFrame(ErrorFrame::BONUS_ERROR);
				return;
			}
		} else
		if (card.type == CardInfo::SERVICE) {
			setServiceMode(card.id);
		} else {
			printErrorFrame(ErrorFrame::UNKNOWN_CARD);
		}
	}
}

void onCard(uint64_t cardid) {
	card = getCardInfo(cardid); 
	if (card.type == bonus::CardInfo::SERVICE) {
		setServiceMode(card.uid);
	} else
	if (card.type == bonus::CardInfo::BONUS) {
		if (writeOffBonuses(card.uid)) {
			isBonusBegin = true;
		}
	} else
	if (card.type == bonus::CardInfo::UNKNOWN) {
		printUnknownCardFrame();
	}
}

void onServiceEnd() {
	setGiveMoneyMode();
	mode = Mode::GIVE_MONEY;
}