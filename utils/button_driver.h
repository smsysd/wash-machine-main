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
	enum class Purpose {
		PROGRAM,
		END
	};
	enum class Type {
		EXTBOARD,
		TOUCH
	};
	Purpose purpose;
	Type type;
	int id;
	int index;
	int prog;
	int serviceProg;
};

void init(json& buttons, void (*onButtonPushed)(const Button& button));

}

#endif