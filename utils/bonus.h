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
	char uid[256];
};

void init(json& bonusSysCnf, json& promotions, json& programs);
CardInfo open(const char* qrOrCardid);
double writeoff(const char* uid);
void close(const char* uid);
double getRate(int iProgram);
double getCoef();

}

#endif