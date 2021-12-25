#include "bonus.h"

namespace bonus {

void init(json& bonusSysCnf, json& promotions, json& programs) {

}

CardInfo open(const char* qrOrCardid) {
	CardInfo ci;
	ci.type = CardInfo::BONUS;
	return ci;
}

double writeoff(const char* uid) {
	return 0;
}

void close(const char* uid) {

}

double getRate(int iProgram) {
	return 0;
}

double getCoef() {
	return 0;
}

}