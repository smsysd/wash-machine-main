#include "bonus.h"
#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

namespace bonus {

namespace {
	vector<Program> _programs;
	vector<Promotion> _promotions;
	int _bonus = 0;
}

void init(json& bonusSysCnf, json& promotions, json& programs) {
	render::regVar(&_bonus, "bonus");
}

CardInfo open(const char* access) {
	CardInfo ci;
	ci.type = CardInfo::BONUS;
	return ci;
}

double writeoff(double desired) {
	return 0;
}

void close() {

}

double getRate(int iProgram) {
	return 0;
}

double getCoef() {
	return 1;
}

int getProgramFrame(int iProgram) {
	return -1;
}

}