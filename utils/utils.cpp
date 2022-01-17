#include "utils.h"
#include "../extboard/ExtBoard.h"
#include "../general-tools/general_tools.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"
#include "../qrscaner-linux/qrscaner.h"
#include "../timer/Timer.h"
#include "../linux-stdsiga/stdsiga.h"
#include "render.h"
#include "button_driver.h"
#include "bonus.h"

#include <wiringPi.h>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <mutex>
#include <time.h>
#include <cmath>

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
		static int borehole = 0;
		static double previousk = 0;

		if (mode == Mode::SERVICE && _tServiceMode > 0) {
			if (time(NULL) > _tOffServiceMode) {
				_onServiceEnd();
			}
		}
		
		if (borehole % 30 == 0) {
			double k = bonus::getCoef();
			if (previousk > 1 && k <= 1) {
				previousk = k;
				render::showFrame(render::SpecFrame::GIVE_MONEY);
				cout << "[INFO][UTILS] 'give money' mode frame switched to default" << endl;
			} else
			if (previousk <= 1 && k > 1) {
				previousk = k;
				render::showFrame(render::SpecFrame::GIVE_MONEY_BONUS);
				cout << "[INFO][UTILS] 'give money' mode frame switched to bonus" << endl;
			}
		}

		borehole++;

		return OK;
	}

	void _softTerminate() {
		_log->log(Logger::Type::INFO, "SIG", "receive soft terminate signal, terminating..");
	}

}

Mode cmode() {
	return mode;
}

void init(
	void (*onCashAppeared)(),
	void (*onCashRunout)(),
	void (*onButtonPushed)(const bd::Button& button),
	void (*onQr)(const char* qr),
	void (*onCard)(uint64_t cardid),
	void (*onServiceEnd)())
{
	_onCashAppeared = onCashAppeared;
	_onCashRunout = onCashRunout;
	_onServiceEnd = onServiceEnd;

	cout << "init general-tools.." << endl;
	stdsiga_init(_softTerminate);
	general_tools_init();
	int wprc = wiringPiSetup();
	if (wprc != 0) {
		_log->log(Logger::Type::ERROR, "INIT", "fail setup wiringPi: " + to_string(wprc));
		throw runtime_error("fail setup wiringPi: " + to_string(wprc));
	}

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

	// get necessary config fields..
	cout << "get necessary config fields.." << endl;


	// get other config fields..
	cout << "get other config fields.." << endl;
	try {
		_tServiceMode = _config->get("service-time");
	} catch (exception& e) {
		_log->log(Logger::Type::INFO, "CONFIG", "no field 'service-time' - set as infinity");
		_tServiceMode = -1;
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
			sp.rate /= 60;
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
			sp.owner = JParser::getf(jp, "owner", "card at [" + to_string(i) + "]");
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

	// render
	try {
		cout << "init render module.." << endl;
		_frames = new JParser("./config/frames.json");
		json& display = _hwconfig->get("display");
		json& sf = _frames->get("spec-frames");
		json& go = _frames->get("general-option");
		json& bg = _frames->get("backgrounds");
		json& fonts = _frames->get("fonts");
		render::init(display, _frames->get("frames"), sf, go, bg, fonts);
		render::regVar(&_nMoney, L"money");
		render::regVar(&_session.k100, L"sbonus");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "RENDER", "fail to init render core: " + string(e.what()));
		throw runtime_error("fail to init render core: " + string(e.what()));
	}

	// extboard
	cout << "init extboard.." << endl;
	try {
		json& extBoardCnf = _hwconfig->get("ext-board");
		json& relaysGroups = _config->get("relays-groups");
		json& performingUnitsCnf = _hwconfig->get("performing-units");
		json& buttons = _hwconfig->get("buttons");
		json& rangeFinder = _hwconfig->get("range-finder");
		json& tempSens = _hwconfig->get("temp-sens");
		json& relIns = _hwconfig->get("releive-instructions");
		json& payment = _hwconfig->get("payment");
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
		json specef;
		if (_effects != nullptr) {
			try {
				effects = _effects->get("effects");
				specef = _effects->get("spec-effects");
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "CONFIG", "fail get load effects: " + string(e.what()));
			}
		}
		extboard::init(extBoardCnf, performingUnitsCnf, relaysGroups, payment, buttons, rangeFinder, tempSens, ledsCnf, effects, specef, relIns);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "EXTBOARD", "fail to init expander board: " + string(e.what()));
		throw runtime_error("fail to init expander board: " + string(e.what()));
	}

	// button
	try {
		cout << "init button driver.." << endl;
		json& buttons = _config->get("buttons");
		bd::init(buttons, onButtonPushed);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "INIT", "fail to init button driver: " + string(e.what()));
		throw runtime_error("fail to init button driver: " + string(e.what()));
	}

	// qr scaner
	try {
		cout << "init qr scanner.." << endl;
		json& qrscnf = _hwconfig->get("qr-scaner");
		bool en = JParser::getf(qrscnf, "enable", "qr-scaner");
		if (en) {
			string driver = JParser::getf(qrscnf, "driver", "qr-scaner");
			int width = JParser::getf(qrscnf, "width", "qr-scaner");
			int height = JParser::getf(qrscnf, "height", "qr-scaner");
			int bright = JParser::getf(qrscnf, "bright", "qr-scaner");
			int contrast = JParser::getf(qrscnf, "contrast", "qr-scaner");
			json sexpos = JParser::getf(qrscnf, "expos", "qr-scaner");
			json sfocus = JParser::getf(qrscnf, "focus", "qr-scaner");
			int readDelay = JParser::getf(qrscnf, "read-delay", "qr-scaner");
			bool equalb = JParser::getf(qrscnf, "eqal-blocking", "qr-scaner");
			int expos = QRSCANER_AUTO;
			int focus = QRSCANER_AUTO;
			if (sexpos.is_number()) {
				expos = sexpos;
			}
			if (sfocus.is_number()) {
				focus = sfocus;
			}
			int rc = qrscaner_init(driver.c_str(), width, height, readDelay, bright, expos, focus, contrast, equalb ? 1 : 0, onQr);
			if (rc == 0) {
				qrscaner_start();
			} else {
				throw runtime_error(to_string(rc));
			}
		} else {
			cout << "qr scaner disabled" << endl;
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "INIT", "fail to init qr-scaner: " + string(e.what()));
	}

	// bonus system
	try {
		cout << "init bonus system module.." << endl;
		_bonuscnf = new JParser("./config/bonus.json");
		bonus::init(_bonuscnf->get("bonus-sys"), _bonuscnf->get("promotions"));
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "INIT", "fail to init bonus system: " + string(e.what()));
	}

	// extboard callbacks
	cout << "register exboard callbacks.." << endl;
	extboard::registerOnMoneyAddedHandler(_onCashAdd);
	extboard::registerOnCardReadHandler(onCard);

	cout << "register genereal handler.." << endl;
	callHandler(_handler, NULL, 500, 0);

	cout << "start withdraw timer.." << endl;
	_wdtimer = new Timer(1, 0, _withdraw);

	cout << "initialize complete." << endl;
}

void setGiveMoneyMode() {
	bool isBonus = false;
	try {
		extboard::startLightEffect(extboard::SpecEffect::GIVE_MONEY_EFFECT, 0);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "fail to start effect: " + string(e.what()));
	}
	render::SpecFrame f = render::SpecFrame::GIVE_MONEY;
	if (bonus::getCoef() > 1) {
		f = render::SpecFrame::GIVE_MONEY_BONUS;
		isBonus = true;
	}
	try {
		render::showFrame(f);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show give money frame: " + string(e.what()));
	}
	if (isBonus) {
		cout << "'give money' mode applied with bonus frame" << endl;
	} else {
		cout << "'give money' mode applied" << endl;
	}
	mode = Mode::GIVE_MONEY;
}

void setServiceMode(uint64_t cardid) {
	try {
		extboard::startLightEffect(extboard::SpecEffect::SERVICE_EFFECT, 0);
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
		extboard::startLightEffect(p->effect, 0);
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
		extboard::startLightEffect(p->effect, 0);
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
	_session.k100 = round(k*100);
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