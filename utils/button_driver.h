/* Author: Jodzik */
#ifndef BUTTON_DRIVER_H_
#define BUTTON_DRIVER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace button_driver {

enum class ButtonType {
	PROGRAM,
	END
};

void init(json& buttons, void (*onButtonPushed)(ButtonType type, int iButton));

}

#endif