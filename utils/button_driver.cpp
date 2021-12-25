#include "button_driver.h"
#include "../extboard/ExtBoard.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace button_driver {

void init(json& buttons, void (*onButtonPushed)(ButtonType type, int iButton)) {
	
}

}