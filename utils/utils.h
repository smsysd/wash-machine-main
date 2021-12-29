/* Author: Jodzik */
#ifndef UTILS_H_
#define UTILS_H_

#include "button_driver.h"
#include "render.h"
#include "bonus.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <time.h>
#include <chrono>

using namespace std;
using namespace std::chrono;
namespace bd = button_driver;
using CardInfo = bonus::CardInfo;

namespace utils {

struct Program {
	int id;
	string name;
	int frame;
	int relayGroup;
	double rate;
	int freeUseTimeSec;
	int remainFreeUseTimeSec;
	time_point<steady_clock> tBegin;
	steady_clock::duration duration;
};

enum class ErrorFrame {
	UNKNOWN_CARD,
	BONUS_ERROR,
	INTERNAL_ERROR
};

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(bd::Button& button),
	void (*onQr)(const char* qr),
	void (*onCard)(uint64_t cardid),
	void (*onServiceEnd)());

void setGiveMoneyMode();
void setServiceMode(uint64_t cardid);
void setProgram(int id);
void setServiceProgram(int id);

bool startBonuses(CardInfo& cardInfo, const char* qr);
bool writeOffBonuses();
void accrueRemainBonusesAndClose();
bool getLocalCardInfo(CardInfo& cardInfo, uint64_t cardid);

void printLogoFrame();
void printErrorFrame(ErrorFrame ef);

}

#endif