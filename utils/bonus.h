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

void init(json& bonusSysCnf, json& promotions, json& programs);
double writeoff(const char* uid);
void accrue(const char* uid, double nBonuses);
double getRate(int iProgram);
double getCoef();

}

#endif