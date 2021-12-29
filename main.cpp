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
using json = nlohmann::json;

enum class Mode {
	GIVE_MONEY,
	PROGRAM,
	SERVICE
};

Mode mode = Mode::GIVE_MONEY;
bonus::CardInfo card;
bool isCardRead = false;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(ButtonType type, int iButton);
void onCard(const char* uid);
void onQr(const char* qr);
void onServiceEnd();

int main(int argc, char const *argv[]) {

	init(onCashAppeared, onCashRunout, onButtonPushed, onCard, onServiceEnd);
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
	isCardRead = false;
	setGiveMoneyMode();
	mode = Mode::GIVE_MONEY;
}

void onButtonPushed(ButtonType type, int iButton) {
	if (mode == Mode::PROGRAM) {
		switch (type) {
		case button_driver::ButtonType::END:
			if (isCardRead) {
				isCardRead = false;
				accrueRemainBonuses(card.uid);
			}
			setGiveMoneyMode();
			mode = Mode::GIVE_MONEY;
			break;
		case button_driver::ButtonType::PROGRAM:
			setProgram(getProgramByButton(iButton));
			break;
		}
	}
}

void onCard(const char* cardid) {
	card = getCardInfo(cardid); 
	if (card.type == bonus::CardInfo::SERVICE) {
		setServiceMode(card.uid);
	} else
	if (card.type == bonus::CardInfo::BONUS) {
		if (writeOffBonuses(card.uid)) {
			isCardRead = true;
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