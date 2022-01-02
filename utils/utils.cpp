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
#include <time.h>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <dirent.h>
#include <errno.h>

using namespace std;
namespace bd = button_driver;

namespace utils {

Mode mode = Mode::INIT;

namespace {
	void (*_onCashAppeared)();
	void (*_onCashRunout)();
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
	vector<CardInfo> _cards;
	Session _session;
	int _currentProgram = -1;

	double _nMoney = 0;
	time_t _tOffServiceMode = 0;
	int _tServiceMode = 0;

	Program* _getProgram(vector<Program>& programs, int id) {
		for (int i = 0; i < programs.size(); i++) {
			if (programs[i].id == id) {
				return &programs[i];
			}
		}

		throw runtime_error("program with id '" + to_string(id) + "' not found");
	}

	void _putSession(json& sd) {
		DIR* dir = opendir("./statistics");
		if (dir) {
			closedir(dir);
		} else if (ENOENT == errno) {
			system("mkdir ./statistics");
		} else {
			_log->log(Logger::Type::ERROR, "MONITOR", "fail open statistics dir: " + to_string(errno));
			return;
		}
		DIR* dir2 = opendir("./statistics/sessions");
		if (dir2) {
			closedir(dir2);
		} else if (ENOENT == errno) {
			system("mkdir ./statistics/sessions");
		} else {
			_log->log(Logger::Type::ERROR, "MONITOR", "fail open session dir: " + to_string(errno));
			return;
		}
		ofstream f("./statistics/sessions/" + to_string(time(NULL)) + ".json", ios_base::out | ios_base::app);
		if (!f.is_open()) {
			_log->log(Logger::Type::ERROR, "MONITOR", "fail open session file");
			return;
		}
		f << sd.dump() << '\n';
		f.close();
	}

	void _onCashAdd(double nMoney) {
		cout << "money received: " << nMoney << endl;
		_moneyMutex.lock();
		if (_nMoney == 0) {
			_onCashAppeared();
		}
		if (_session.isBegin) {
			_nMoney += nMoney * _session.k;
			_session.depositedMoney += nMoney;
		}
		_moneyMutex.unlock();
	}

	Timer::Action _withdraw(timer_t) {
		if(!_session.isBegin) {
			return Timer::Action::CONTINUE;
		}
		_moneyMutex.lock();
		double oldv = _nMoney;
		if (mode == Mode::PROGRAM) {
			if (_programs[_currentProgram].useTimeSec > _programs[_currentProgram].freeUseTimeSec) {
				_nMoney -= _programs[_currentProgram].rate;
				if (_nMoney <= 0) {
					_nMoney = 0;
					if (oldv > 0) {
						_onCashRunout();
					}
				}
			}
			_session.totalSpent += oldv - _nMoney;
			_programs[_currentProgram].spendMoney += oldv - _nMoney;
			_programs[_currentProgram].useTimeSec++;

		} else
		if (mode == Mode::SERVICE) {
			_servicePrograms[_currentProgram].useTimeSec++;
		}
		_moneyMutex.unlock();
		return Timer::Action::CONTINUE;
	}

	ReturnCode _handler(uint16_t id, void* arg) {
		static double previousk = 0;

		if (mode == Mode::SERVICE && _tServiceMode > 0) {
			if (time(NULL) > _tOffServiceMode) {
				_onServiceEnd();
			}
		}

		double k = bonus::getCoef();
		if (previousk > 1 && k <= 1) {
			render::showFrame(render::SpecFrame::GIVE_MONEY);
		} else
		if (previousk <= 1 && k > 1) {
			render::showFrame(render::SpecFrame::GIVE_MONEY_BONUS);
		}
		return OK;
	}

}

Mode cmode() {
	return mode;
}

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(bd::Button& button),
	void (*onQr)(const char* qr),
	void (*onCard)(uint64_t cardid),
	void (*onServiceEnd)())
{
	_onCashAppeared = onCashAppeared;
	_onCashRunout = onCashRunout;
	_onServiceEnd = onServiceEnd;

	cout << "init general-tools.." << endl;
	general_tools_init();

	try {
		_log = new Logger("./log.txt");
	} catch (exception& e) {
		throw runtime_error("fail to create logger: " + string(e.what()));
	}

	// load necessary config
	cout << "load necessary config files.." << endl;
	try {
		_hwconfig = new JParser("./config/hwconfig.json");
		_config = new JParser("./config/config.json");
	} catch (exception& e) {
		throw runtime_error("fail to load necessary config files: " + string(e.what()));
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

	// get necessary config fields..
	cout << "get necessary config fields.." << endl;


	// get other config fields..
	cout << "get other config fields.." << endl;
	try {
		_tServiceMode = _config->get("service-time");
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "CONFIG", "no field 'service-time' - set as infinity");
		throw runtime_error("no field 'service-time' - set as infinity");
	}

	// extboard
	cout << "init extboard.." << endl;
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
		try {
			_effects = new JParser("./config/effects.json");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "UTILS", "fail to load effects: " + string(e.what()));
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
		bd::init(buttonsCnf, onButtonPushed);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to init button driver: " + string(e.what()));
		throw runtime_error("fail to init button driver: " + string(e.what()));
	}

	// render
	try {
		cout << "init render module.." << endl;
		_frames = new JParser("./config/frames.json");
		json& sf = _frames->get("spec-frames");
		json& go = _frames->get("general-option");
		json& bg = _frames->get("backgrounds");
		json& fonts = _frames->get("fonts");
		render::init(displayCnf, _frames->get("frames"), sf, go, bg, fonts);
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
		int rc = qrscaner_init(cdriver, width, height, onQr);
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
		bonus::init(_bonuscnf->get("bonus-sys"), _bonuscnf->get("promotions"));
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
			sp.frame = JParser::getf(jp, "frame", "program at [" + to_string(i) + "]");
			sp.bonusFrame = JParser::getf(jp, "bonus-frame", "program at [" + to_string(i) + "]");
			sp.relayGroup = JParser::getf(jp, "relay-group", "program at [" + to_string(i) + "]");
			sp.freeUseTimeSec = JParser::getf(jp, "free-use-time", "program at [" + to_string(i) + "]");
			sp.name = JParser::getf(jp, "name", "program at [" + to_string(i) + "]");
			sp.rate = JParser::getf(jp, "rate", "program at [" + to_string(i) + "]");
			sp.effect = JParser::getf(jp, "effect", "program at [" + to_string(i) + "]");
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
			sp.rate = 0;
			sp.effect = JParser::getf(jp, "effect", "program at [" + to_string(i) + "]");
			_servicePrograms.push_back(sp);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "UTILS INIT", "fail to fill programs: " + string(e.what()));
		throw runtime_error("fail to fill programs: " + string(e.what()));
	}

	// fill cards
	try {
		cout << "fill cards.." << endl;
		json& cards = _config->get("cards");
		for (int i = 0; i < cards.size(); i++) {
			json& jp = cards[i];
			CardInfo sp;
			sp.id = JParser::getf(jp, "id", "card at [" + to_string(i) + "]");
			string ct = JParser::getf(jp, "type", "card at [" + to_string(i) + "]");
			if (ct == "service") {
				sp.type = CardInfo::SERVICE;
			} else
			if (ct == "bonus") {
				sp.type = CardInfo::BONUS_PERS;
			} else {
				sp.type = CardInfo::UNKNOWN;
			}
			_cards.push_back(sp);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "LOCAL CARDS", "fail to fill cards: " + string(e.what()));
	}

	// extboard callbacks
	cout << "register exboard callbacks.." << endl;
	extboard::registerOnCashAddedHandler(_onCashAdd);
	extboard::registerOnCardReadHandler(onCard);

	cout << "register genereal handler.." << endl;
	callHandler(_handler, NULL, 500, 0);

	cout << "start withdraw timer.." << endl;
	_wdtimer = new Timer(1, 0, _withdraw);

	cout << "initialize complete." << endl;
}

void setGiveMoneyMode() {
	try {
		extboard::startLightEffect(extboard::SpecEffect::GIVE_MONEY_EFFECT, extboard::LightEffectType::MAIN);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to start effect: " + string(e.what()));
	}
	render::SpecFrame f = render::SpecFrame::GIVE_MONEY;
	if (bonus::getCoef() > 1) {
		f = render::SpecFrame::GIVE_MONEY_BONUS;
	}
	try {
		render::showFrame(f);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show give money frame: " + string(e.what()));
	}
	cout << "'give money' mode applied" << endl;
	mode = Mode::GIVE_MONEY;
}

void setServiceMode(uint64_t cardid) {
	try {
		extboard::startLightEffect(extboard::SpecEffect::SERVICE_EFFECT, extboard::LightEffectType::MAIN);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to start effect: " + string(e.what()));
	}
	cout << "'service' mode applied" << endl;
	_tOffServiceMode = time(NULL) + _tServiceMode;
	mode = Mode::SERVICE;
}

void setProgram(int id) {
	Program* p;
	try {
		p = _getProgram(_programs, id);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "INTERNAL", "fail to set program: " + string(e.what()));
		return;
	}

	try {
		extboard::startLightEffect(p->effect, extboard::LightEffectType::MAIN);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to start effect: " + string(e.what()));
	}

	try {
		extboard::setRelayGroup(p->relayGroup);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "EXTBOARD", "fail to set program");
		return;
	}

	try {
		if (_session.k > 1) {
			render::showFrame(p->bonusFrame);
		} else {
			render::showFrame(p->frame);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show frame: " + string(e.what()));
	}

	cout << "program '" << p->name << "' set" << endl;
	mode = Mode::PROGRAM;
}

void setServiceProgram(int id) {
	Program* p;
	try {
		p = _getProgram(_servicePrograms, id);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "INTERNAL", "fail to set service program: " + string(e.what()));
		return;
	}

	try {
		extboard::startLightEffect(p->effect, extboard::LightEffectType::MAIN);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to start effect: " + string(e.what()));
	}

	try {
		extboard::setRelayGroup(p->relayGroup);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to set service program");
		return;
	}

	try {
		render::showFrame(p->frame);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show frame: " + string(e.what()));
	}

	cout << "service program '" << p->name << "' set" << endl;
	if (mode != Mode::SERVICE) {
		_tOffServiceMode = time(NULL) + _tServiceMode;
		mode = Mode::SERVICE;
	}
}

void beginSession(Session::Type type, uint64_t id) {
	double k = bonus::getCoef();
	_session.k = k;
	_session.type = type;
	_session.cardid = id;
	_session.totalSpent = 0;
	_session.depositedMoney = 0;
	_session.writeoffBonuses = 0;
	_session.acrueBonuses = 0;
	
	// clear programs
	for (int i = 0; i < _programs.size(); i++) {
		_programs[i].useTimeSec = 0;
		_programs[i].spendMoney = 0;
	}

	for (int i = 0; i < _servicePrograms.size(); i++) {
		_servicePrograms[i].useTimeSec = 0;
	}

	_session.tBegin = time(NULL);
	_session.isBegin = true;
}

void dropSession() {
	// TODO
	if (!_session.isBegin) {
		return;
	}
	if (_session.type == Session::Type::CLIENT) {
		json sd;
		sd["type"] = "client";
		sd["total-spent"] = _session.totalSpent;
		sd["deposited-money"] = _session.depositedMoney;
		sd["acrue-bonuses"] = _session.acrueBonuses;
		sd["writeoff-bonuses"] = _session.writeoffBonuses;
		if (_session.cardid != 0) {
			sd["card"] = _session.cardid;
		}
		json pa;
		for (int i = 0; i < _programs.size(); i++) {
			json p;
			p["id"] = _programs[i].id;
			p["name"] = _programs[i].name;
			p["use-time"] = _programs[i].useTimeSec;
			p["spend-money"] = _programs[i].spendMoney;
			pa[i] = p;
		}
		sd["programs"] = pa;
		_putSession(sd);
	} else
	if (_session.type == Session::Type::SERVICE) {
		json sd;
		sd["type"] = "service";
		if (_session.cardid != 0) {
			sd["card"] = _session.cardid;
		}
		json pa;
		for (int i = 0; i < _servicePrograms.size(); i++) {
			json p;
			p["id"] = _servicePrograms[i].id;
			p["name"] = _servicePrograms[i].name;
			p["use-time"] = _servicePrograms[i].useTimeSec;
			pa[i] = p;
		}
		sd["programs"] = pa;
		_putSession(sd);
	} else {
		_log->log(Logger::Type::ERROR, "MONITOR", "not defined session type");
	}
	_session.isBegin =  false;
}

bool startBonuses(CardInfo& cardInfo, const char* qr) {
	try {
		cardInfo = bonus::open(qr);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "BONUS", "fail to open transaction: " + string(e.what()));
		return false;
	}

	return true;
}

bool writeOffBonuses() {
	try {
		double realWriteoff = bonus::writeoff();
		_nMoney += realWriteoff;
		if (_session.isBegin) {
			_session.writeoffBonuses += realWriteoff;
		}
		return true;
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "BONUS", "fail writeoff bonuses: " + string(e.what()));
	}
	return false;
}

void accrueRemainBonusesAndClose() {
	if (_session.isBegin) {
		_log->log(Logger::Type::ERROR, "BONUS", "fail close transaction with " + to_string(_nMoney) + " money: session not begin");
	}
	double acrue = _nMoney * _session.k;
	try {
		bonus::close(acrue);
		_session.acrueBonuses += acrue;
		_nMoney = 0;
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "BONUS", "fail close transaction with " + to_string(acrue) + " money: " + string(e.what()));
	}
}

bool getLocalCardInfo(CardInfo& cardInfo, uint64_t cardid) {
	for (int i = 0; i < _cards.size(); i++) {
		if (_cards[i].id == cardid) {
			cardInfo = _cards[i];
			return true;
		}
	}

	return false;
}

}