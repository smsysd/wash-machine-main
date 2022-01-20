#include "button_driver.h"
#include "../extboard/ExtBoard.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace button_driver {

namespace {
	vector<Button> _buttons;
	void (*_onButtonPushed)(const Button& button);

	void _onExtboardButton(int iButton) {
		for (int i = 0; i < _buttons.size(); i++) {
			if (_buttons[i].type == Button::Type::EXTBOARD) {
				if (_buttons[i].index == iButton) {
					cout << "[INFO][BUTTON] button " << _buttons[i].id << " pushed" << endl;
					_onButtonPushed(_buttons[i]);
				}
			}
		}
	}
}

void init(json& buttons, void (*onButtonPushed)(const Button& button)) {
	for (int i = 0; i < buttons.size(); i++) {
		try {
			Button b;
			b.id = JParser::getf(buttons[i], "id", "");
			string type = JParser::getf(buttons[i], "type", "");
			if (type == "extboard") {
				b.type = Button::Type::EXTBOARD;
				b.index = JParser::getf(buttons[i], "index", "");
			} else
			if (type == "touch") {
				b.type = Button::Type::TOUCH;
				throw runtime_error("type 'touch' still not supported");
			} else {
				throw runtime_error("unkown type '" + type + "'");
			}
			string purpose = JParser::getf(buttons[i], "purpose", "");
			if (purpose == "program") {
				b.purpose = Button::Purpose::PROGRAM;
				b.prog = JParser::getf(buttons[i], "program", "");
				b.serviceProg = JParser::getf(buttons[i], "service-program", "");
			} else
			if (purpose == "end") {
				b.purpose = Button::Purpose::END;
			} else {
				throw runtime_error("unknown purpose '" + purpose + "'");
			}
			_buttons.push_back(b);
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' button: " + string(e.what()));
		}
	}

	_onButtonPushed = onButtonPushed;
	extboard::registerOnButtonPushedHandler(_onExtboardButton);
}

}