/* Author: Jodzik */
#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <string>
#include <vector>

using namespace std;
namespace bd = button_driver;
using ButtonType = bd::ButtonType;

namespace utils {

struct Program {
	int id;
	string name;
	int frame;
	int relayGroup;
	double rate;
	int freeUseTimeSec;
	int remainFreeUseTimeSec;
};

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(ButtonType type, int iButton),
	void (*onCard)(const char* uid),
	void (*onQr)(const char* qr));

void printLogo();
void setGiveMoneyMode();
void setProgramMode();
void setServiceMode(const char* uid);
void setProgram(int iProg);
int getProgramByButton(int iButton);
void bonusEnd();
void bonusBegin(const char* uid);
bool isServiceCard(const char* uid);

}

#endif