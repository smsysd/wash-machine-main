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
#include <mutex>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;
namespace bd = button_driver;

namespace utils {

namespace {
	void (*_onCashAppeared)();
	void (*_onCashRunout)();
	void (*_onButtonPushed)(bd::ButtonType type, int iButton);
	void (*_onCard)(const char* cardid);
	void (*_onServiceEnd)();

	JParser* _hwconfig = nullptr;
	JParser* _config = nullptr;
	JParser* _effects = nullptr;
	JParser* _bonuscnf = nullptr;
	JParser* _frames = nullptr;

	Logger* _log = nullptr;
	Timer* _wdtimer = nullptr;
	mutex _moneyMutex;

	vector<Program> _programs;
	vector<Program> _servicePrograms;
	int _currentProgram = -1;
	int _logoFrame = -1;
	int _unknownCardFrame = -1;

	double _nMoney = 0;

	void _onCashAdd(double nMoney) {
		cout << "money received: " << nMoney << endl;
		_moneyMutex.lock();
		if (_nMoney == 0) {
			_onCashAppeared();
		}
		_nMoney += nMoney;
		_moneyMutex.unlock();
	}

	Timer::Action _withdraw(timer_t) {
		_moneyMutex.lock();
		_moneyMutex.unlock();
		return Timer::Action::CONTINUE;
	}

	ReturnCode _handler(uint16_t id, void* arg) {
		return OK;
	}
}

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(bd::ButtonType type, int iButton),
	void (*onCard)(const char* cardid),
	void (*onServiceEnd)())
{
	_onCashAppeared = onCashAppeared;
	_onCashRunout = onCashRunout;
	_onButtonPushed = onButtonPushed;
	_onCard = onCard;
	_onServiceEnd = onServiceEnd;

	cout << "init general-tools.." << endl;
	general_tools_init();

	try {
		_log = new Logger("./log.txt");
	} catch (exception& e) {
		throw runtime_error("fail to create logger: " + string(e.what()));
	}

	try {
		cout << "load necessary config files.." << endl;
		_hwconfig = new JParser("./config/hwconfig.json");
		_config = new JParser("./config/config.json");
	} catch (exception& e) {
		throw runtime_error("fail to load necessary config files: " + string(e.what()));
	}

	try {
		_effects = new JParser("./config/effects.json");
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS", "fail to load effects: " + string(e.what()));
	}

	json buttonsCnf;
	json displayCnf;

	try {
		buttonsCnf = _hwconfig->get("buttons");
		displayCnf = _hwconfig->get("display");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "CONFIG", "fail to get config: " + string(e.what()));
		throw runtime_error("fail to get config: " + string(e.what()));
	}


	// extboard
	try {
		cout << "init extboard.." << endl;
		json& extBoardCnf = _hwconfig->get("ext-board");
		json& relaysGroups = _config->get("relays-groups");
		json& performingUnitsCnf = _hwconfig->get("performing-units");
		json ledsCnf;
		try {
			ledsCnf = _hwconfig->get("leds");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to load leds: " + string(e.what()));
		}
		json effects;
		if (_effects != nullptr) {
			try {
				effects = _effects->get("effects");
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "UTILS INIT", "fail get load effects: " + string(e.what()));
			}
		}
		extboard::init(extBoardCnf, performingUnitsCnf, relaysGroups, buttonsCnf, ledsCnf, effects);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to init expander board: " + string(e.what()));
		throw runtime_error("fail to init expander board: " + string(e.what()));
	}

	// button
	try {
		cout << "init button driver.." << endl;
		bd::init(buttonsCnf, _onButtonPushed);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to init button driver: " + string(e.what()));
		throw runtime_error("fail to init button driver: " + string(e.what()));
	}

	// render
	try {
		cout << "init render module.." << endl;
		_frames = new JParser("./config/frames.json");
		json& go = _frames->get("general-option");
		json& bg = _frames->get("backgrounds");
		json& fonts = _frames->get("fonts");
		try {
			_logoFrame = _config->get("logo-frame");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "CONFIG", "fail to get logo frame: " + string(e.what()));
		}
		render::init(displayCnf, _frames->get("frames"), go, bg, fonts);
		render::regVar(&_nMoney, L"money");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "RENDER", "fail to init render core: " + string(e.what()));
		// throw runtime_error("fail to init render core: " + string(e.what()));
	}

	// qr scaner
	try {
		cout << "init qr scanner.." << endl;
		json& qrscnf = _hwconfig->get("qr-scaner");
		string driver = JParser::getf(qrscnf, "driver", "qr-scaner");
		int width = JParser::getf(qrscnf, "width", "qr-scaner");
		int height = JParser::getf(qrscnf, "height", "qr-scaner");
		char cdriver[256] = {0};
		strcpy(cdriver, driver.c_str());
		int rc = qrscaner_init(cdriver, width, height, _onCard);
		if (rc == 0) {
			qrscaner_start();
		} else {
			throw runtime_error(to_string(rc));
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to init qr-scaner: " + string(e.what()));
	}

	// bonus system
	try {
		cout << "init bonus system module.." << endl;
		_bonuscnf = new JParser("./config/bonus.json");
		bonus::init(_bonuscnf->get("bonus-sys"), _bonuscnf->get("promotions"), _config->get("programs"));
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "UTILS INIT", "fail to init bonus system: " + string(e.what()));
	}

	// fill programs
	try {
		cout << "fill client programs.." << endl;
		json& programs = _config->get("programs");
		for (int i = 0; i < programs.size(); i++) {
			json& jp = programs[i];
			Program sp;
			sp.id = JParser::getf(jp, "id", "program at [" + to_string(i) + "]");
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
		cout << "fil service programs.." << endl;
		json& programs = _config->get("service-programs");
		for (int i = 0; i < programs.size(); i++) {
			json& jp = programs[i];
			Program sp;
			sp.id = JParser::getf(jp, "id", "program at [" + to_string(i) + "]");
			sp.frame = JParser::getf(jp, "frame", "program at [" + to_string(i) + "]");
			sp.relayGroup = JParser::getf(jp, "relay-group", "program at [" + to_string(i) + "]");
			sp.freeUseTimeSec = -1;
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
	cout << "register exboard callbacks.." << endl;
	extboard::registerOnCashAddedHandler(_onCashAdd);
	extboard::registerOnCardReadHandler(_onCard);

	cout << "register genereal handler.." << endl;
	callHandler(_handler, NULL, 500, 0);

	cout << "start withdraw timer.." << endl;
	_wdtimer = new Timer(1, 0, _withdraw);

	cout << "initialize complete." << endl;
}

void setGiveMoneyMode() {
	cout << "apply 'give money' mode.." << endl;
}

void setServiceMode(const char* uid) {
	cout << "apply 'service' mode.." << endl;
}

void setProgram(int iProg) {
	if (iProg >= _programs.size() || iProg < 0) {
		cout << "incorrect set program" << endl;
		return;
	}

	cout << "program '" << _programs[iProg].name << "' set" << endl;
}

void setServiceProgram(int iProg) {
	if (iProg >= _servicePrograms.size() || iProg < 0) {
		cout << "incorrect set service program" << endl;
		return;
	}

	cout << "service program '" << _servicePrograms[iProg].name << "' set" << endl;
}

int getProgramByButton(int iButton) {
	return -1;
}

int getServiceProgramByButton(int iButton) {
	return -1;
}

bool writeOffBonuses(const char* uid) {
	cout << "try write off bonuses.." << endl;
	return false;
}

void accrueRemainBonuses(const char* uid) {
	cout << "accrue bonuses.." << endl;
}

// additionaly check local storage service cards
CardInfo getCardInfo(const char* qrOrCardid) {
	CardInfo ci;
	ci.type = CardInfo::BONUS;
	return ci;
}

void printLogoFrame() {
	render::showFrame(_logoFrame);
}

void printUnknownCardFrame() {
	render::showFrame(_unknownCardFrame);
}

}