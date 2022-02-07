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
using json = nlohmann::json;

const int tErrorFrame = 4;
const int tLogoFrame = 3;

CardInfo card;
bool isBonusBegin = false;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(const Button& button);
void onCard(uint64_t cardid);
void onQr(const char* qr);
void onServiceEnd();

int main(int argc, char const *argv[]) {
	init(onCashAppeared, onCashRunout, onButtonPushed, onQr, onCard, onServiceEnd);
	render::showTempFrame(render::SpecFrame::LOGO, tLogoFrame);
	setGiveMoneyMode();
	
	while (true) {
		sleep(5);
	}

	return 0;
}

void onCashAppeared() {
	if (isBonusBegin) {
		if (card.id != 0) {
			beginSession(Session::Type::CLIENT, card.id);
		} else {
			beginSession(Session::Type::CLIENT, 0);
		}
	} else {
		beginSession(Session::Type::CLIENT, 0);
	}
	setProgram(0);
}

void onCashRunout() {
	if (isBonusBegin) {
		isBonusBegin = false;
		accrueRemainBonusesAndClose();
	}
	card.id = 0;
	dropSession();
	setGiveMoneyMode();
}

void onButtonPushed(const Button& button) {
	if (cmode() == Mode::PROGRAM) {
		switch (button.purpose) {
		case Button::Purpose::END:
			if (isBonusBegin) {
				isBonusBegin = false;
				accrueRemainBonusesAndClose();
			}
			card.id = 0;
			dropSession();
			setGiveMoneyMode();
			break;
		case Button::Purpose::PROGRAM:
			setProgram(button.prog);
			break;
		}
	} else
	if (cmode() == Mode::SERVICE) {
		switch (button.purpose) {
		case Button::Purpose::END:
			setGiveMoneyMode();
			break;
		case Button::Purpose::PROGRAM:
			setServiceProgram(button.serviceProg);
			break;
		}
	}
}

void _handleCard() {
	bool rc;
	if (card.type == CardInfo::SERVICE) {
		dropSession();
		if (cmode() != Mode::SERVICE) {
			beginSession(Session::Type::SERVICE, card.id);
			setServiceMode();
		} else {
			setGiveMoneyMode();
		}
	} else
	if (card.type == CardInfo::BONUS) {
		if (isBonusBegin) {
			if (bonus::ismultibonus()) {
				if (card.count <= 0) {
					render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
					return;
				}
				rc = writeOffBonuses();
				if (!rc) {
					render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
				}
			}
		} else {
			if (card.count <= 0) {
				render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
				return;
			}
			rc = writeOffBonuses();
			if (!rc) {
				render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
			} else {
				isBonusBegin = true;
			}
		}
	} else
	if (card.type == CardInfo::ONETIME) {
		addMoney(card.count);
	} else {
		render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
	}
}

void onQr(const char* qr) {
	if (isBonusBegin && !bonus::ismultibonus()) {
		cout << "[INFO][MAIN] multibonus is disable" << endl;
		return;
	}
	bool rc = bonus::open(card, qr);
	if (rc) {
		if (card.type == bonus::CardInfo::UNKNOWN) {
			render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
		} else
		if (card.type == bonus::CardInfo::NOT_MET) {
			render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
		} else {
			_handleCard();
		}
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}
}

void onCard(uint64_t cardid) {
	bool rc = getLocalCardInfo(card, cardid);
	if (rc) {
		if (card.type == CardInfo::SERVICE) {
			dropSession();
			if (cmode() != Mode::SERVICE) {
				beginSession(Session::Type::SERVICE, card.id);
				setServiceMode();
			} else {
				setGiveMoneyMode();
			}
		} else
		if (card.type == CardInfo::BONUS) {
			addMoney(card.count);
		} else {
			render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
		}
		return;
	}

	rc = bonus::open(card, cardid);
	if (rc) {
		if (card.type != bonus::CardInfo::UNKNOWN) {
			_handleCard();
		} else {
			render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
		}
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}
}

void onServiceEnd() {
	dropSession();
	setGiveMoneyMode();
}