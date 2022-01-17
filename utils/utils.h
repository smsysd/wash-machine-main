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

enum class Mode {
	INIT,
	GIVE_MONEY,
	PROGRAM,
	SERVICE
};

struct Program {
	int id;
	string name;
	int frame;
	int bonusFrame;
	int relayGroup;
	double rate;
	int freeUseTimeSec;
	int useTimeSec;
	double spendMoney;
	int effect;
};

struct Session {
	enum Type {
		CLIENT,
		SERVICE
	};
	Type type;
	double k;
	int k100;
	time_t tBegin;
	time_t tEnd;
	uint64_t cardid;
	bool isBegin;
	double totalSpent;
	double depositedMoney;
	double writeoffBonuses;
	double acrueBonuses;
};

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(const bd::Button& button),
	void (*onQr)(const char* qr),
	void (*onCard)(uint64_t cardid),
	void (*onServiceEnd)());

void setGiveMoneyMode();
void setServiceMode(uint64_t cardid);
void setProgram(int id);
void setServiceProgram(int id);

/* Session for monitoring - setup bonus and session default values  */
void beginSession(Session::Type type, uint64_t id);
void dropSession();

bool startBonuses(CardInfo& cardInfo, const char* qr);
bool writeOffBonuses();
void accrueRemainBonusesAndClose();
bool getLocalCardInfo(CardInfo& cardInfo, uint64_t cardid);

Mode cmode();

}

#endif