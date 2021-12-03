#include "extboard/ExtBoard.h"
#include "general-tools/general_tools.h"
#include "jparser-linux/JParser.h"
#include "utils/utils.h"

#include <sstream>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;
using namespace utils;
using json = nlohmann::json;

enum class Mode {
	GIVE_MONEY,
	PROGRAM,
	SERVICE
};

Mode mode = Mode::GIVE_MONEY;

void onCashAppeared();
void onCashRunout();
void onButtonPushed(ButtonType type, int iButton);
void onCard(const char* uid);
void onQr(const char* qr);

int main(int argc, char const *argv[]) {
	init(onCashAppeared, onCashRunout, onButtonPushed, onCard, onQr);
	printLogo();
	setGiveMoneyMode();
	
	while (true) {

	}

	return 0;
}

void onCashAppeared() {
	setProgramMode();
	mode = Mode::PROGRAM;
	setProgram(0);
}

void onCashRunout() {
	setGiveMoneyMode();
	mode = Mode::GIVE_MONEY;
}

void onButtonPushed(ButtonType type, int iButton) {
	switch (type) {
	case ButtonType::END:
		if (mode == Mode::PROGRAM) {
			bonusEnd();
		}
		setGiveMoneyMode();
		break;
	case ButtonType::PROGRAM:
		if (mode == Mode::PROGRAM) {
			setProgram(getProgramByButton(iButton));
		}
		break;
	}
}

void onCard(const char* uid) {
	if (isServiceCard(uid)) {
		setServiceMode(uid);
		mode = Mode::SERVICE;
	} else {
		bonusBegin(uid);
	}
}

void onQr(const char* qr) {
	bonusBegin(qr);
}
