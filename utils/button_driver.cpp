#include "button_driver.h"
#include "../extboard/ExtBoard.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../linux-ipc/ipc.h"

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

namespace button_driver {

namespace {
	vector<Button> _buttons;
	void (*_onButtonPushed)(const Button& button);
	IpcServer _ipcsrv;

	void _onExtboardButton(int iButton) {
		Button::Type type;
		int bm;
		int index;
		// cout << "[INFO][BUTTON] ibutton " << iButton << endl;
		if (iButton >= 72) {
			return;
		}
		if (iButton < 8) {
			type = Button::Type::EB;
			index = iButton;
			for (int i = 0; i < _buttons.size(); i++) {
				if (_buttons[i].type == type) {
					if (_buttons[i].index == index) {
						cout << "[INFO][BUTTON] button " << _buttons[i].id << " pushed" << endl;
						_onButtonPushed(_buttons[i]);
					}
				}
			}
		} else {
			type = Button::Type::BM;
			iButton -= 8;
			bm = iButton / 8;
			index = iButton % 8;
			for (int i = 0; i < _buttons.size(); i++) {
				if (_buttons[i].type == type) {
					if (_buttons[i].bm == bm) {
						if (_buttons[i].index == index) {
							cout << "[INFO][BUTTON] button " << _buttons[i].id << " pushed" << endl;
							_onButtonPushed(_buttons[i]);
						}
					}
				}
			}
		}
	}
}

void init(json& buttons, json& hwbuttons, void (*onButtonPushed)(const Button& button)) {
	for (int i = 0; i < buttons.size(); i++) {
		try {
			Button b;
			b.id = JParser::getf(buttons[i], "id", "");
			string type = JParser::getf(buttons[i], "type", "");
			if (type == "eb") {
				b.type = Button::Type::EB;
				b.index = JParser::getf(buttons[i], "index", "");
				if (b.index > 7 || b.index < 0) {
					throw runtime_error("'index' must be in [0:7]");
				}
			} else
			if (type == "bm") {
				b.type = Button::Type::BM;
				b.bm = JParser::getf(buttons[i], "bm", "");
				b.index = JParser::getf(buttons[i], "index", "");
				if (b.index > 7 || b.index < 0) {
					throw runtime_error("'index' must be in [0:7]");
				}
				if (b.bm > 7 || b.bm < 0) {
					throw runtime_error("'bm' must be in [0:7]");
				}
			} else
			if (type == "nwjs") {
				b.type = Button::Type::NWJS;
				b.name = JParser::getf(buttons[i], "name", "");
			} else {
				throw runtime_error("unkown type '" + type + "'");
			}

			string purpose = JParser::getf(buttons[i], "purpose", "");
			if (purpose == "program") {
				b.purpose = Button::Purpose::PROGRAM;
				try {
					b.prog = JParser::getf(buttons[i], "program", "");
				} catch (exception& e) {
					b.prog = -1;
				}
				try {
					b.serviceProg = JParser::getf(buttons[i], "service-program", "");
				} catch (exception& e) {
					b.serviceProg = -1;
				}
			} else
			if (purpose == "end") {
				b.purpose = Button::Purpose::END;
			} else {
				throw runtime_error("unknown purpose '" + purpose + "'");
			}
			_buttons.push_back(b);
			cout << "[INFO][BUTTON] add button " << b.id << ", type " << (int)b.type << ", purpose: " << (int)b.purpose << ", index " << b.index << ", bm " << b.bm << endl;
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' button: " + string(e.what()));
		}
	}

	_onButtonPushed = onButtonPushed;
	extboard::registerOnButtonPushedHandler(_onExtboardButton);
}

void _onNwjsButton(const char* n) {
	string name(n);
	Button::Type type = Button::Type::NWJS;
	for (int i = 0; i < _buttons.size(); i++) {
		if (_buttons[i].type == type) {
			if (_buttons[i].name == name) {
				cout << "[INFO][BUTTON] button " << _buttons[i].id << " pushed" << endl;
				_onButtonPushed(_buttons[i]);
			}
		}
	}
}

}