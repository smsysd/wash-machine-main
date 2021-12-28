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
	Type _type;
	string _cert;
}

void init(json& bonusSysCnf, json& promotions, json& programs) {
	// link with render
	render::regVar(&_bonus, L"bonus");
	
	// general init
	string bt = JParser::getf(bonusSysCnf, "type", "bonus-system");
	if (bt == "aquanter") {
		_type = Type::AQUANTER;
		_cert = JParser::getf(bonusSysCnf, "certificate", "bonus-system");

	} else
	if (bt == "gazoil") {
		_type = Type::GAZOIL;
		throw runtime_error("bonus system '" + bt + "' still not supported");
	} else {
		throw runtime_error("unknown bonus system type '" + bt + "'");
	}

	// fill programs
	for (int i = 0; i < programs.size(); i++) {
		json& jp = programs[i];
		Program sp;
		sp.id = JParser::getf(jp, "id", "program at [" + to_string(i) + "]");
		sp.name = JParser::getf(jp, "name", "program at [" + to_string(i) + "]");
		sp.defFrame = JParser::getf(jp, "frame", "program at [" + to_string(i) + "]");
		sp.bonusFrame = JParser::getf(jp, "bonus-frame", "program at [" + to_string(i) + "]");

		_programs.push_back(sp);
	}

	// fill promotions
	for (int i = 0; i < promotions.size(); i++) {
		json& jp = promotions[i];
		Promotion sp;
		string pt = JParser::getf(jp, "type", "promotion " + to_string(i));
		if (pt == "coefficient") {
			sp.type = Promotion::COEFFICIENT;
			sp.k = JParser::getf(jp, "k", "promotion " + to_string(i));
			sp.sk = JParser::getf(jp, "sumk", "promotion " + to_string(i));
			sp.priority = JParser::getf(jp, "priority", "promotion " + to_string(i));
			if (jp["time"].is_string()) {

			} else {
				
			}
		} else {
			throw runtime_error("unknown type '" + pt + "' for " + to_string(i) + " promotion");
		}
	}
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