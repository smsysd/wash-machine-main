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

enum class Result {
	OK,
	FAIL,
	NOT_FOUND,
	COND_NOT_MET,
	TR_ALREADY_OPEN
};

struct CardInfo {
	enum Type {
		BONUS,
		ONETIME,
		SERVICE,
		UNKNOWN
	};
	Type type;
	uint64_t id;
	double count;
	string owner;
	char access_str[256];
	uint64_t access_int;
};

void init(json& bonusSysCnf, json& promotions);

// No throwable below
// OK, FAIL, NOT_FOUND, TR_ALREADY_OPEN
Result open(CardInfo& card);

// OK, FAIL, NOT_FOUND
Result info(CardInfo& card, const char* access);
Result info(CardInfo& card, uint64_t access);

// OK, FAIL, NOT_FOUND, COND_NOT_MET (tr not open)
Result writeoff(double& count);

void close(double acrue);

// OK, FAIL, NOT_FOUND, COND_NOT_MET, if OK in card.count will be count of bonuses
Result onetime(CardInfo& card);

double getCoef();
bool ismultibonus();

}

#endif