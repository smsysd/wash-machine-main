/* Author: Jodzik */
#ifndef BONUS_H_
#define BONUS_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"


#include <string>
#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace bonus {

struct CardInfo {
	enum Type {
		BONUS,
		SERVICE,
		UNKNOWN
	};
	Type type;
	char uid[64];
};

struct Program {
	int defFrame;
	int bonusFrame;
	double rate;
};

struct Promotion {
	enum Type {
		COEFFICIENT
	};
	Type type;
	double k;
	double sk;
	int priority;
	int time[2];
	bool weekdays[7];
	bool months[12];
};

void init(json& bonusSysCnf, json& promotions, json& programs);
CardInfo open(const char* access);
double writeoff(double desired);
void close();
double getRate(int iProgram);
double getCoef();
int getProgramFrame(int iProgram);

}

#endif