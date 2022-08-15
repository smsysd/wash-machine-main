/* Author: Jodzik */
#ifndef UTILS_H_
#define UTILS_H_

#include "button_driver.h"
#include "render.h"
#include "bonus.h"
#include "../extboard/ExtBoard.h"

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
	SERVICE,
	WAIT
};

struct Program {
	int id;
	string name;
	string frame;
	string bonusFrame;
	int relayGroup;
	double rate;
	int freeUseTimeSec;
	int useTimeSec;
	double spendMoney;
	int effect;
	bool releivePressure;
	bool flap;
};


struct Session {
	enum Type {
		CLIENT,
		SERVICE,
		COLLECTION
	};
	enum EndType {
		PROGRAM,
		BUTTON,
		MONEY_RUNOUT,
		NEW_SESSION
	};
	Type type;
	EndType endType;
	double k;
	int k100;
	int rk100;
	time_t tBegin;
	time_t tEnd;
	uint64_t cardid;
	vector<Pay> pays;
	bool isBegin;
};

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(const bd::Button& button),
	void (*onQr)(const char* qr),
	void (*onCard)(uint64_t cardid),
	void (*onServiceEnd)());

void setGiveMoneyMode();
void setServiceMode();
void setProgram(int id);
void setServiceProgram(int id);
void setWaitMode();

/* Session for monitoring - setup bonus and session default values  */
void beginSession(Session::Type type, uint64_t id);
void dropSession(Session::EndType endType);

/* bonus macros */
void accrueRemainBonusesAndClose();
bool getLocalCardInfo(CardInfo& cardInfo, uint64_t cardid);

/* flags */
Mode cmode();
bool issession();
bool is_terminate();

/* extra control */
void addMoney(double nMoney, Payment::Type type);

}

#endif