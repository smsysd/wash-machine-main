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
};

void init(json& bonusSysCnf, json& promotions, json& programs);
CardInfo open(const char* access);
double writeoff(double desired);
void close(double acrue);
double getRate(int idProgram);
double getCoef();
int getProgramFrame(int idProgram);

}

#endif