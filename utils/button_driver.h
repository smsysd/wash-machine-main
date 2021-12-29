/* Author: Jodzik */
#ifndef BUTTON_DRIVER_H_
#define BUTTON_DRIVER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace button_driver {

struct Button {
	enum Type {
		PROGRAM,
		END
	};
	Type type;
	int id;
	int prog;
	int serviceProg;
};

void init(json& buttons, void (*onButtonPushed)(Button& button));

}

#endif