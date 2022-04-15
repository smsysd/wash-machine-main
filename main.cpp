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

CardInfo curCard;
bool isBonusBegin = false;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(const Button& button);
void onCard(uint64_t cardid);
void onQr(const char* qr);
void onServiceEnd();
void stop(Session::EndType endType);

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
		beginSession(Session::Type::CLIENT, curCard.id);
	} else {
		beginSession(Session::Type::CLIENT, 0);
	}
	setProgram(0);
}

void onCashRunout() {
	stop(Session::EndType::MONEY_RUNOUT);
}

void onButtonPushed(const Button& button) {
	if (cmode() == Mode::PROGRAM) {
		switch (button.purpose) {
		case Button::Purpose::END:
			stop(Session::EndType::BUTTON);
			break;
		case Button::Purpose::PROGRAM:
			setProgram(button.prog);
			break;
		}
	} else
	if (cmode() == Mode::SERVICE) {
		switch (button.purpose) {
		case Button::Purpose::END:
			stop(Session::EndType::NEW_SESSION);
			break;
		case Button::Purpose::PROGRAM:
			setServiceProgram(button.serviceProg);
			break;
		}
	} else if (cmode() == Mode::WAIT) {
		setGiveMoneyMode();
	}
}

void writeoffBonus() {
	double count = 0;
	bonus::Result rc = bonus::writeoff(count);
	if (rc == bonus::Result::OK) {
		if (count <= 0) {
			render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
			return;
		}
		addMoney(count, Payment::Type::BONUS_EXT);
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}	
}

void beginBonus(bonus::CardInfo& card) {
	bonus::Result rc = bonus::open(card);
	if (rc == bonus::Result::OK) {
		isBonusBegin = true;
		curCard = card;
		writeoffBonus();
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}
}

void _handleCard(bonus::CardInfo& card, bool local = false) {
	bonus::Result rc;
	if (card.type == CardInfo::SERVICE) {
		if (cmode() == Mode::SERVICE) {
			stop(Session::EndType::PROGRAM);
		} else
		if (cmode() == Mode::GIVE_MONEY) {
			beginSession(Session::Type::SERVICE, card.id);
			setServiceMode();
		} else
		if (cmode() == Mode::PROGRAM) {
			stop(Session::EndType::NEW_SESSION);
			beginSession(Session::Type::SERVICE, card.id);
			setServiceMode();
		}
	} else
	if (card.type == CardInfo::BONUS) {
		if (local) {
			addMoney(card.count, Payment::Type::SERVICE);
			return;
		}
		if (card.count <= 0) {
			render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
			return;
		}
		if (isBonusBegin) {
			if (curCard.id == card.id) {
				if (bonus::ismultibonus()) {
					writeoffBonus();
				}
			} else {
				stop(Session::EndType::PROGRAM);
				beginBonus(card);
			}
		} else {
			beginBonus(card);
		}
	} else
	if (card.type == CardInfo::ONETIME) {
		if (isBonusBegin && !bonus::ismultibonus()) {
			return;
		}
		rc = bonus::onetime(card);
		if (rc == bonus::Result::OK) {
			addMoney(card.count, Payment::Type::BONUS_EXT);
		} else
		if (rc == bonus::Result::COND_NOT_MET) {
			render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
		} else {
			render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
		}
	} else
	if (card.type == CardInfo::COLLECTOR) {
		beginSession(Session::Type::COLLECTION, card.id);
		dropSession(Session::EndType::BUTTON);
	} else {
		render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
	}
}

void onQr(const char* qr) {
	bonus::CardInfo newcard;
	render::showFrame(render::SpecFrame::SERVER_CONNECT);
	bonus::Result rc = bonus::info(newcard, qr);
	if (rc == bonus::Result::OK) {
		_handleCard(newcard);
	} else
	if (rc == bonus::Result::COND_NOT_MET) {
		render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
	} else
	if (rc == bonus::Result::NOT_FOUND) {
		render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}
}

void onCard(uint64_t cardid) {
	bonus::CardInfo newcard;
	bool rcb = getLocalCardInfo(newcard, cardid);
	if (rcb) {
		_handleCard(newcard, true);
		return;
	}
	render::showFrame(render::SpecFrame::SERVER_CONNECT);
	bonus::Result rc = bonus::info(newcard, cardid);
	if (rc == bonus::Result::OK) {
		_handleCard(newcard);
	} else
	if (rc == bonus::Result::COND_NOT_MET) {
		render::showTempFrame(render::SpecFrame::NOMONEY, tErrorFrame);
	} else
	if (rc == bonus::Result::NOT_FOUND) {
		render::showTempFrame(render::SpecFrame::UNKNOWN_CARD, tErrorFrame);
	} else {
		render::showTempFrame(render::SpecFrame::BONUS_ERROR, tErrorFrame);
	}
}

void onServiceEnd() {
	stop(Session::EndType::PROGRAM);
}

void stop(Session::EndType endType) {
	if (isBonusBegin) {
		accrueRemainBonusesAndClose();
		isBonusBegin = false;
	}
	dropSession(endType);
	setGiveMoneyMode();
}