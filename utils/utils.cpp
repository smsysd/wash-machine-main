#include "utils.h"
#include "../extboard/ExtBoard.h"
#include "../general-tools/general_tools.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"
#include "../qrscaner-linux/qrscaner.h"
#include "../timer/Timer.h"
#include "render.h"
#include "button_driver.h"
#include "bonus.h"

#include <sstream>
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;
namespace bd = button_driver;
using ButtonType = bd::ButtonType;

namespace utils {

namespace {
	void (*_onCashAppeared)();
	void (*_onCashRunout)();
	void (*_onButtonPushed)(ButtonType type, int iButton);
	void (*_onCard)(const char* cardid);

	JParser* _hwconfig = nullptr;
	JParser* _config = nullptr;
	JParser* _effectscnf = nullptr;
	JParser* _bonuscnf = nullptr;
	JParser* _framescnf = nullptr;

	Logger* _log = nullptr;
	Timer* _wdtimer = nullptr;

	vector<Program> _programs;
	vector<Program> _servicePrograms;
	int _currentProgram = -1;
	int _logoFrame = -1;
	int _unknownCardFrame = -1;

	double _nMoney = 0;


}

void init(void (*onCashAppeared)(), void (*onCashRunout)(), void (*onButtonPushed)(ButtonType type, int iButton), void (*onCard)(const char* cardid)) {
	_onCashAppeared = onCashAppeared;
	_onCashRunout = onCashRunout;
	_onButtonPushed = onButtonPushed;
	_onCard = onCard;

	try {
		_log = new Logger("./log.txt");
	} catch (exception& e) {
		throw runtime_error("fail to create logger: " + string(e.what()));
	}

	try {
		_hwconfig = new JParser("./config/hwconfig.json");
		_config = new JParser("./config/config.json");
	} catch (exception& e) {
		throw runtime_error("fail to load necessary config files: " + string(e.what()));
	}

	try {
		_effectscnf = new JParser("./config/effects.json");
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS", "fail to load effects: " + string(e.what()));
	}

	json buttonsCnf;
	json displayCnf;

	try {
		buttonsCnf = _hwconfig->get("buttons");
		displayCnf = _hwconfig->get("display");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to get config: " + string(e.what()));
		throw runtime_error("fail to get config: " + string(e.what()));
	}


	// extboard
	try {
		json& extBoardCnf = _hwconfig->get("ext-board");
		json& relaysGroups = _config->get("relays-groups");
		json& performingUnitsCnf = _hwconfig->get("performing-units");
		json ledsCnf;
		try {
			ledsCnf = _hwconfig->get("leds");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to load leds: " + string(e.what()));
		}
		json effectsCnf;
		if (_effectscnf != nullptr) {
			try {
				effectsCnf = _effectscnf->get("effects");
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "UTILS INIT", "fail get load effects: " + string(e.what()));
			}
		}
		ExtBoard::init(extBoardCnf, performingUnitsCnf, relaysGroups, buttonsCnf, displayCnf, ledsCnf, effectsCnf);
		_log->log(Logger::Type::INFO, "UTILS INIT", "expander board init");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to init expander board: " + string(e.what()));
		throw runtime_error("fail to init expander board: " + string(e.what()));
	}

	// button
	try {
		bd::init(buttonsCnf, _onButtonPushed);
		_log->log(Logger::Type::INFO, "UTILS INIT", "button driver init");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to init button driver: " + string(e.what()));
		throw runtime_error("fail to init button driver: " + string(e.what()));
	}

	// render
	try {
		_framescnf = new JParser("./config/frames.json");
		try {
			_logoFrame = _config->get("logo-frame");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "CONFIG", "fail to get logo frame: " + string(e.what()));
		}
		render::init(displayCnf, _framescnf->get("frames"));
		_log->log(Logger::Type::INFO, "UTILS INIT", "render core init");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "RENDER", "fail to init render core: " + string(e.what()));
		throw runtime_error("fail to init render core: " + string(e.what()));
	}

	// qr scaner
	try {
		json& qrscnf = _hwconfig->get("qr-scaner");
		string driver = JParser::getf(qrscnf, "driver", "qr-scaner");
		int width = JParser::getf(qrscnf, "width", "qr-scaner");
		int height = JParser::getf(qrscnf, "height", "qr-scaner");
		char cdriver[256] = {0};
		strcpy(cdriver, driver.c_str());
		int rc = qrscaner_init(cdriver, width, height, _onCard);
		if (rc == 0) {
			_log->log(Logger::Type::INFO, "UTILS INIT", "qr-scaner init");
		} else {
			throw runtime_error(to_string(rc));
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to init qr-scaner: " + string(e.what()));
	}

	// bonus system
	try {
		_bonuscnf = new JParser("./config/bonus.json");
		bonus::init(_bonuscnf->get("bonus-sys"), _bonuscnf->get("promotions"), _config->get("programs"));
		_log->log(Logger::Type::INFO, "INIT", "bonus system init");
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to init bonus system: " + string(e.what()));
	}

	// fill programs
	try {
		json& programs = _config->get("programs");
		for (int i = 0; i < programs.size(); i++) {
			json& jp = programs[i];
			Program sp;
			sp.id = JParser::getf(jp, "id", "program at [" + to_string(i) + "]");
			sp.frame = JParser::getf(jp, "frame", "program at [" + to_string(i) + "]");
			sp.relayGroup = JParser::getf(jp, "relay-group", "program at [" + to_string(i) + "]");
			sp.freeUseTimeSec = JParser::getf(jp, "free-use-time", "program at [" + to_string(i) + "]");
			sp.name = JParser::getf(jp, "name", "program at [" + to_string(i) + "]");
			sp.remainFreeUseTimeSec = sp.freeUseTimeSec;
			sp.rate = 0;
			_programs.push_back(sp);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to fill programs: " + string(e.what()));
		throw runtime_error("fail to fill programs: " + string(e.what()));
	}
	
	try {
		json& programs = _config->get("service-programs");
		for (int i = 0; i < programs.size(); i++) {
			json& jp = programs[i];
			Program sp;
			sp.id = JParser::getf(jp, "id", "program at [" + to_string(i) + "]");
			sp.frame = JParser::getf(jp, "frame", "program at [" + to_string(i) + "]");
			sp.relayGroup = JParser::getf(jp, "relay-group", "program at [" + to_string(i) + "]");
			sp.freeUseTimeSec = JParser::getf(jp, "free-use-time", "program at [" + to_string(i) + "]");
			sp.name = JParser::getf(jp, "name", "program at [" + to_string(i) + "]");
			sp.remainFreeUseTimeSec = sp.freeUseTimeSec;
			sp.rate = 0;
			_servicePrograms.push_back(sp);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to fill programs: " + string(e.what()));
		throw runtime_error("fail to fill programs: " + string(e.what()));
	}


	// extboard callbacks
	ExtBoard::registerOnCashAddedHandler(_onCashAdd);
	ExtBoard::registerOnCardReadHandler(_onCard);

	_wdtimer = new Timer(1, 0, _withdraw);
}

void setGiveMoneyMode() {

}

void setProgramMode() {

}

void setServiceMode(const char* uid) {

}

void setProgram(int iProg) {

}

int getProgramByButton(int iButton) {

}

bool writeOffBonuses(const char* uid) {

}

void accrueRemainBonuses(const char* uid) {

}

// additionaly check local storage service cards
CardInfo getCardInfo(const char* qrOrCardid) {

}

int getProgramByButton(int iButton) {

}

int getProgramFrame(int iProgram) {

}

void printLogoFrame() {
	render::showFrame(_logoFrame);
}

void printUnknownCardFrame() {
	render::showFrame(_unknownCardFrame);
}

namespace {

void _onCashAdd(double nMoney) {
	if (_nMoney == 0) {
		_onCashAppeared();
	}
	_nMoney += nMoney;
}

Timer::Action _withdraw(timer_t) {
	return Timer::Action::CONTINUE;
}

}
}