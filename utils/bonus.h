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

enum Type {
	AQUANTER,
	GAZOIL
};

struct CardInfo {
	enum Type {
		BONUS_PERS,
		BONUS_ORG,
		SERVICE,
		UNKNOWN
	};
	Type type;
	uint64_t id;
	double count;
	string owner;
};

void init(json& bonusSysCnf, json& promotions);
CardInfo open(const char* access);
double writeoff();
void close(double acrue);
double getCoef();

}

#endif