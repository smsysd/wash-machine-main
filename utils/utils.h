/* Author: Jodzik */
#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <time.h>
#include <chrono>

using namespace std;
using namespace std::chrono;
namespace bd = button_driver;
using ButtonType = bd::ButtonType;
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
	duration<long> duration;
};

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(ButtonType type, int iButton),
	void (*onCard)(const char* cardid),
	void (*onServiceEnd)());

void setGiveMoneyMode();
void setServiceMode(const char* uid);
void setProgram(int iProg);

bool writeOffBonuses(const char* uid);
void accrueRemainBonuses(const char* uid);

CardInfo getCardInfo(const char* qrOrCardid); // additionaly check local storage service cards
int getProgramByButton(int iButton);
int getProgramFrame(int iProgram);

void printLogoFrame();
void printUnknownCardFrame();

}

#endif