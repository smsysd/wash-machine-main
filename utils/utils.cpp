#include "utils.h"
#include "../extboard/ExtBoard.h"
#include "../perf/Perf.h"
#include "../general-tools/general_tools.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"
#include "../qrscaner-linux/qrscaner.h"
#include "../timer/Timer.h"
#include "../linux-stdsiga/stdsiga.h"
#include "../linux-ipc/ipc.h"
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
#include <sstream>
#include <iomanip>
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
	IpcServer _main_srv;
	bool _terminate = false;
	bool _normalwork = false;

	vector<Program> _programs;
	vector<Program> _servicePrograms;
	vector<CardInfo> _cards;
	Session _session;
	int _currentProgram = -1;

	double _nMoney = 0;
	time_t _tOffServiceMode = 0;
	int _tRemainFreeUseTime = 0;
	int _tServiceMode = 0;
	char _errort[32];
	int _tGiveMoneyMode = 0;
	int _tMaxGiveMoneyMode = 120;
	bool _money_enable = false;
	string _rfut_exceeded;

	string _paytype2str(Payment::Type type) {
		switch (type) {
		case Payment::Type::BONUS_EXT: return "BONUS_EXTERNAL"; break;
		case Payment::Type::BONUS_LOCAL: return "BONUS_LOCAL"; break;
		case Payment::Type::CASH: return "CASH"; break;
		case Payment::Type::COIN: return "COIN"; break;
		case Payment::Type::SERVICE: return "SERVICE"; break;
		case Payment::Type::STORED: return "STORED"; break;
		case Payment::Type::TERM: return "TERMINAL"; break;
		default: return "unknown"; break;
		}
	}

	string _mode2str(Mode mode) {
		switch (mode) {
		case Mode::GIVE_MONEY: return "GIVE_MONEY";
		case Mode::INIT: return "GIVE_MONEY";
		case Mode::PROGRAM: return "PROGRAM";
		case Mode::SERVICE: return "SERVICE";
		case Mode::WAIT: return "WAIT";
		default: return "unknown";
		}
	}
	
	string _setprec(double v) {
		v *= 100;
		v = floor(v + 0.5);
		return to_string(v / 100);
	}

	Program* _getProgram(vector<Program>& programs, int id) {
		for (int i = 0; i < programs.size(); i++) {
			if (programs[i].id == id) {
				return &programs[i];
			}
		}

		throw runtime_error("program with id '" + to_string(id) + "' not found");
	}

	int _getpi(vector<Program>& programs, int id) {
		for (int i = 0; i < programs.size(); i++) {
			if (programs[i].id == id) {
				return i;
			}
		}

		throw runtime_error("program with id '" + to_string(id) + "' not found");
	}

	void _putSession(json& sd, string name) {
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
		ofstream f("./statistics/sessions/" + name + ".json", ios_base::out);
		if (!f.is_open()) {
			_log->log(Logger::Type::ERROR, "MONITOR", "fail open session file");
			return;
		}
		f << sd.dump() << '\n';
		f.close();
	}

	void _onMoneyAdd(Pay pay) {
		if (pay.count == 0 || !_money_enable) {
			return;
		}
		_log->log(Logger::Type::INFO, "MONEY", "current money: " + to_string(_nMoney) + ", " + to_string(pay.count) + " money received from " + _paytype2str(pay.type), 6);
		_moneyMutex.lock();
		if (_nMoney == 0) {
			_onCashAppeared();
		}
		if (_session.isBegin) {
			_session.pays.push_back(pay);
			_nMoney += pay.count;
			if (pay.type == Payment::Type::CASH || pay.type == Payment::Type::TERM || pay.type == Payment::Type::COIN) {				
				if (_session.k > 1) {
					Pay bonuspay;
					bonuspay.type = Payment::Type::BONUS_LOCAL;
					bonuspay.count = pay.count * (_session.k - 1);
					_session.pays.push_back(pay);
					_nMoney += bonuspay.count;
				}	
			}
		} else {
			_log->log(Logger::Type::WARNING, "MONEY", "money received, but session is not begin");
			_nMoney += pay.count;
		}
		_moneyMutex.unlock();
	}

	void _onExtCashAdd(Pay pay) {
		_onMoneyAdd(pay);
	}

	Timer::Action _withdraw(timer_t) {
		// cout << "withdraw " << time(NULL) % 100 << endl;
		if(!_session.isBegin || !_normalwork) {
			return Timer::Action::CONTINUE;
		}
		_moneyMutex.lock();
		double oldv = _nMoney;
		if (mode == Mode::PROGRAM) {
			_programs[_currentProgram].useTimeSec++;
			// cout << "withdraw, program " << _currentProgram << ", use time " <<_programs[_currentProgram].useTimeSec << ", free use time " << _programs[_currentProgram].freeUseTimeSec << endl;
			if (_programs[_currentProgram].useTimeSec > _programs[_currentProgram].freeUseTimeSec) {
				// cout << "withdraw " << _programs[_currentProgram].rate << " money" << endl;
				_nMoney -= _programs[_currentProgram].rate;
				if (_nMoney > 0) {
					_programs[_currentProgram].spendMoney += oldv - _nMoney;
				}
				
				if (_nMoney <= 0) {
					_nMoney = 0;
					if (oldv > 0) {
						_onCashRunout();
					}
				}
			}

			_tRemainFreeUseTime = _programs[_currentProgram].freeUseTimeSec - _programs[_currentProgram].useTimeSec;
			if (_tRemainFreeUseTime < 0) {
				_tRemainFreeUseTime = 0;
			}
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
		if (perf::getState() == 0 && extboard::getState()) {
			if (!_normalwork) {
				extboard::payment_ctl(true);
				_normalwork = true;
				_log->log(Logger::Type::INFO, "MAIN", "NORMAL WORK");
				switch (mode) {
				case Mode::GIVE_MONEY: setGiveMoneyMode(); break;
				case Mode::PROGRAM: setProgram(_currentProgram); break;
				case Mode::SERVICE: setServiceMode(); setServiceProgram(_currentProgram); break;
				case Mode::WAIT: setWaitMode(); break;
				}
			}
		} else {
			if (_normalwork) {
				_normalwork = false;
				int perf_addr = perf::getState();
				memset(_errort, 0, sizeof(_errort));
				if (perf_addr > 0) {
					extboard::payment_ctl(false);
					sprintf(_errort, "PERF: %i", perf_addr);
				} else {
					sprintf(_errort, "EXTBOARD");
				}
				render::showFrame("error");
			}
			return OK;
		}

		// Off service mode
		if (mode == Mode::SERVICE && _tServiceMode > 0) {
			if (time(NULL) > _tOffServiceMode) {
				_onServiceEnd();
			}
		}
		
		if (mode == Mode::GIVE_MONEY) {
			_tGiveMoneyMode++;
			if (_tGiveMoneyMode > _tMaxGiveMoneyMode) {
				setWaitMode();
			}
		}
		// Check coef while inaction
		if (mode == Mode::GIVE_MONEY && borehole % 30 == 0) {
			double k = bonus::getCoef();
			if (previousk > 1 && k <= 1) {
				previousk = k;
				render::showFrame("give_money");
				_log->log(Logger::Type::INFO, "UTILS", "'give money' mode frame switched to default", 8);
			} else
			if (previousk <= 1 && k > 1) {
				previousk = k;
				render::showFrame("give_money_bonus");
				_log->log(Logger::Type::INFO, "UTILS", "'give money' mode frame switched to bonus", 8);
			}
		}

		borehole++;

		return OK;
	}

	void _softTerminate(int sock) {
		char buf[256] = {0};
		int rc;
		_log->log(Logger::Type::INFO, "SIG", "receive soft terminate signal", 5);
		if ((!_normalwork && _nMoney > 0) || (mode != Mode::GIVE_MONEY && mode != Mode::WAIT)) {
			rc = sprintf(buf, "%i\n", -1);
			write(sock, buf, rc);
			return;
		}
		rc = sprintf(buf, "%i\n", 0);
		write(sock, buf, rc);
		close(sock);
		render::showFrame("repair");
		sleep(1);
		_terminate = true;
	}

	void _money_restored() {
		_log->log(Logger::Type::INFO, "MONEY", "money restored", 7);
		_money_enable = true;
	}

	int _srv_handler_getbody(const char* req) {
		for (int i = 0; i < strlen(req); i++) {
			if (req[i] == ' ') {
				return i + 1;
			}
		}
		return -1;
	}

	void _srv_handler(int sock) {
		// static int con_cnt = 0;
		// con_cnt++;
		// cout << "con_cnt: " << con_cnt << endl;
		char buf[256] = {0};
		int rc = read(sock, buf, sizeof(buf)-1);
		if (rc > 0) {
			if (strstr(buf, "gv") == buf) {
				rc = _srv_handler_getbody(buf);
				if (rc > 0) {
					char* body = &buf[rc];
					int len = strlen(body);
					if (len > 0) {
						if (body[len-1] == '\n') {
							len--;
						}
						body[len] = 0;
						if (strcmp(body, "money") == 0) {
							memset(buf, 0, sizeof(buf));
							rc = sprintf(buf, "%i", (int)(_nMoney+0.99));
							write(sock, buf, rc);
						} else
						if (strcmp(body, "bonus") == 0) {
							memset(buf, 0, sizeof(buf));
							rc = sprintf(buf, "%i", (int)(bonus::getCoef()*100));
							write(sock, buf, rc);
						} else
						if (strcmp(body, "bonus_add") == 0) {
							memset(buf, 0, sizeof(buf));
							rc = sprintf(buf, "%i", (int)(bonus::getCoef()*100)-100);
							write(sock, buf, rc);
						} else
						if (strcmp(body, "rfut") == 0) {
							if (_currentProgram >= 0 && mode == Mode::PROGRAM) {
								memset(buf, 0, sizeof(buf));
								int temp = _programs[_currentProgram].freeUseTimeSec - _programs[_currentProgram].useTimeSec;
								if (temp > 0) {
									rc = sprintf(buf, "%i", (int)temp);
								} else {
									rc = sprintf(buf, "%s", _rfut_exceeded.c_str());
								}
								write(sock, buf, rc);
							}
						} else
						if (strcmp(body, "error") == 0) {
							memset(buf, 0, sizeof(buf));
							rc = sprintf(buf, "%s", _errort);
							write(sock, buf, rc);
						}
					}
				}
			} else
			if (strstr(buf, "poll") == buf) {
				memset(buf, 0, sizeof(buf));
				rc = sprintf(buf, "%i", 0);
				write(sock, buf, rc);
			} else
			if (strstr(buf, "exit") == buf) {
				_softTerminate(sock);
			} else
			if (strstr(buf, "mode") == buf) {
				memset(buf, 0, sizeof(buf));
				rc = sprintf(buf, "%s\n", _mode2str(mode).c_str());
				write(sock, buf, rc);
			} else
			if (strstr(buf, "state") == buf) {
				memset(buf, 0, sizeof(buf));
				string state;
				if (_normalwork) {
					state = "NORMAL_WORK";
				} else {
					state = "ERROR: " + string(_errort);
				}
				rc = sprintf(buf, "mode: %s\nstate: %s\nsession: %s\n", _mode2str(mode).c_str(), state.c_str(), _session.isBegin ? "BEGIN" : "NOTHING");
				write(sock, buf, rc);
			} else
			if (strstr(buf, "reset") == buf) {
				_nMoney = 0;
				mode = Mode::GIVE_MONEY;
				_session.isBegin = false;
			}
		}
	}
}

Mode cmode() {
	return mode;
}

bool issession() {
	return _session.isBegin;
}

bool is_terminate() {
	return _terminate;
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

	cout << "[INFO][UTILS] init general-tools.." << endl;
	DIR* dir = opendir("./statistics");
	if (dir) {
		closedir(dir);
	} else if (ENOENT == errno) {
		system("mkdir ./statistics");
	} else {
		throw runtime_error("[ERROR][LOGGER] fail to open statistics dir: " + to_string(errno));
		return;
	}
	try {
		_log = new Logger("./statistics/log.txt");
		_log->log(Logger::Type::INFO, "START", "");
	} catch (exception& e) {
		throw runtime_error("[ERROR][LOGGER] fail to create logger: " + string(e.what()));
	}

	general_tools_init();
	int wprc = wiringPiSetup();
	if (wprc != 0) {
		_log->log(Logger::Type::ERROR, "GEN", "fail setup wiringPi: " + to_string(wprc));
		exit(-1);
	}

	// load necessary config
	cout << "[INFO][UTILS] load necessary config.." << endl;
	try {
		_hwconfig = new JParser("./config/hwconfig.json");
		_config = new JParser("./config/config.json");
		_tServiceMode = _config->get("service-time");
		_tMaxGiveMoneyMode = _config->get("give-money-time");
		_rfut_exceeded = _config->get("rfut_exceeded");
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "CONFIG", "fail to load necessary config files: " + string(e.what()));
		exit(-1);
	}

	try {
		int loglevel = _config->get("log-level");
		_log->setLogLevel(loglevel);
	} catch (exception& e) {

	}

	// fill programs
	try {
		cout << "[INFO][UTILS] fill client programs.." << endl;
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
			sp.releivePressure = JParser::getf(jp, "releive", "program at [" + to_string(i) + "]");
			sp.flap = JParser::getf(jp, "flap", "program at [" + to_string(i) + "]");
			_programs.push_back(sp);
		}
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "CONFIG", "fail to fill programs: " + string(e.what()));
		exit(-1);
	}
	
	try {
		cout << "[INFO][UTILS] fil service programs.." << endl;
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
		_log->log(Logger::Type::ERROR, "CONFIG", "fail to fill programs: " + string(e.what()));
		exit(-1);
	}

	// fill cards
	try {
		cout << "[INFO][UTILS] fill cards.." << endl;
		json& cards = _config->get("cards");
		for (int i = 0; i < cards.size(); i++) {
			try {
				json& jp = cards[i];
				CardInfo sp;
				sp.id = JParser::getf(jp, "id", "card at [" + to_string(i) + "]");
				string ct = JParser::getf(jp, "type", "card at [" + to_string(i) + "]");
				sp.owner = JParser::getf(jp, "owner", "card at [" + to_string(i) + "]");
				if (ct == "service") {
					sp.type = CardInfo::SERVICE;
				} else
				if (ct == "bonus") {
					sp.type = CardInfo::BONUS;
					sp.count = JParser::getf(jp, "bonus", "card at [" + to_string(i) + "]");
				} else
				if (ct == "collector") {
					sp.type = CardInfo::COLLECTOR;
				} else {
					sp.type = CardInfo::UNKNOWN;
				}
				_cards.push_back(sp);
				cout << "[INFO][UTILS] add card " << sp.id << " with type " << sp.type << endl;
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "CONFIG", "fail to fill card " + to_string(i) + ": " + string(e.what()));
			}
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "CONFIG", "fail to fill cards: " + string(e.what()));
	}


	// render
	try {
		cout << "[INFO][UTILS] init render module.." << endl;
		json& display = _hwconfig->get("display");
		render::init(display, _log);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "RENDER", "fail to init render core: " + string(e.what()));
		exit(-2);
	}

	// extboard
	cout << "[INFO][UTILS] init extboard.." << endl;
	try {
		json& extBoardCnf = _hwconfig->get("ext-board");
		json& payment = _hwconfig->get("payment");
		json ledsCnf;
		try {
			ledsCnf = _hwconfig->get("leds");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "CONFIG", "fail to load leds: " + string(e.what()));
		}
		try {
			_effects = new JParser("./config/effects.json");
		} catch (exception& e) {
			_log->log(Logger::Type::WARNING, "CONFIG", "fail to load effects: " + string(e.what()));
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
		extboard::init(extBoardCnf, payment, ledsCnf, effects, specef, _log);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "EXTBOARD", "fail to init expander board: " + string(e.what()));
		exit(-3);
	}

	// performers init
	cout << "[INFO][UTILS] init perf unit.." << endl;
	try {
		json& perfGen = _hwconfig->get("performing-gen");
		json& relaysGroups = _config->get("relays-groups");
		json& performingUnitsCnf = _hwconfig->get("performing-units");
		json& relIns = _hwconfig->get("releive-instructions");
		json& payment = _hwconfig->get("payment");
		perf::init(perfGen, performingUnitsCnf, relaysGroups, relIns, _log);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "PERF", "fail to init performing units: " + string(e.what()));
		exit(-3);
	}

	// button
	try {
		cout << "[INFO][UTILS] init button driver.." << endl;
		json& buttons = _config->get("buttons");
		bd::init(buttons, onButtonPushed);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "BUTTON", "fail to init button driver: " + string(e.what()));
		exit(-4);
	}

	// qr scaner
	try {
		cout << "[INFO][UTILS] init qr scanner.." << endl;
		json& qrscnf = _hwconfig->get("qr-scaner");
		bool en = JParser::getf(qrscnf, "enable", "qr-scaner");
		if (en) {
			string driver = JParser::getf(qrscnf, "driver", "qr-scaner");
			int width = JParser::getf(qrscnf, "width", "qr-scaner");
			int height = JParser::getf(qrscnf, "height", "qr-scaner");
			int bright = JParser::getf(qrscnf, "bright", "qr-scaner");
			int contrast = JParser::getf(qrscnf, "contrast", "qr-scaner");
			int reinitPeriod = JParser::getf(qrscnf, "reinit-period", "qr-scaner");
			json sexpos = JParser::getf(qrscnf, "expos", "qr-scaner");
			json sfocus = JParser::getf(qrscnf, "focus", "qr-scaner");
			bool equalb = JParser::getf(qrscnf, "equal-blocking", "qr-scaner");
			int expos = QRSCANER_AUTO;
			int focus = QRSCANER_AUTO;
			if (sexpos.is_number()) {
				expos = sexpos;
			}
			if (sfocus.is_number()) {
				focus = sfocus;
			}
			int rc = qrscaner_init(driver.c_str(), width, height, bright, expos, focus, contrast, equalb ? 1 : 0, reinitPeriod, onQr);
			if (rc == 0) {
				qrscaner_start();
			} else {
				throw runtime_error(to_string(rc));
			}
		} else {
			cout << "[INFO][UTILS] qr scaner disabled" << endl;
		}
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "QRSCANER", "fail to init qr-scaner: " + string(e.what()));
	}

	// bonus system
	try {
		cout << "[INFO][UTILS] init bonus system module.." << endl;
		_bonuscnf = new JParser("./config/bonus.json");
		bonus::init(_bonuscnf->get("bonus-sys"), _bonuscnf->get("promotions"));
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "BONUS", "fail to init bonus system: " + string(e.what()));
	}

	// extboard callbacks
	cout << "[INFO][UTILS] register exboard callbacks.." << endl;
	extboard::registerOnMoneyAddedHandler(_onExtCashAdd);
	extboard::registerOnCardReadHandler(onCard);

	usleep(100000);
	extboard::restoreMoney(_money_restored);

	// init main ipc server
	int rc = ipc_server(&_main_srv, "./main.ipc", 1, _srv_handler);
	if (rc < 0) {
		_log->log(Logger::Type::ERROR, "UTILS", "fail init main ipc server");
		exit(-1);
	}

	cout << "[INFO][UTILS] register genereal handler.." << endl;
	callHandler(_handler, NULL, 500, 0);

	cout << "[INFO][UTILS] start withdraw timer.." << endl;
	_wdtimer = new Timer(1, 0, _withdraw);

	cout << "[INFO][UTILS] initialize complete." << endl;
	_normalwork = true;
	extboard::payment_ctl(true);
	_log->log(Logger::Type::INFO, "MAIN", "NORMAL WORK");
}

void setGiveMoneyMode() {
	if (_terminate) {
		return;
	}
	bool isBonus = false;
	extboard::startLightEffect(extboard::SpecEffect::GIVE_MONEY_EFFECT, 0);
	extboard::flap(true);
	perf::setRelayGroup(0);
	perf::relievePressure();
	if (bonus::getCoef() > 1) {
		isBonus = true;
		render::showFrame("give_money_bonus");
	} else {
		render::showFrame("give_money");
	}

	if (isBonus) {
		_log->log(Logger::Type::INFO, "MODE", "'give money' mode applied with bonus frame", 8);
	} else {
		_log->log(Logger::Type::INFO, "MODE", "'give money' mode applied", 8);
	}
	mode = Mode::GIVE_MONEY;
	_tGiveMoneyMode = 0;
}

void setWaitMode() {
	perf::setRelayGroup(0);
	extboard::startLightEffect(extboard::SpecEffect::WAIT, 0);
	extboard::flap(false);
	render::showFrame("wait");

	_log->log(Logger::Type::INFO, "MODE", "'wait' mode applied", 8);
	mode = Mode::WAIT;
}

void setServiceMode() {
	if (_terminate) {
		return;
	}
	perf::setRelayGroup(0);
	extboard::flap(false);
	render::showFrame("service");

	extboard::startLightEffect(extboard::SpecEffect::SERVICE_EFFECT, 0);
	_log->log(Logger::Type::INFO, "MODE", "'service' mode applied", 8);
	_tOffServiceMode = time(NULL) + _tServiceMode;
	mode = Mode::SERVICE;
}

void setProgram(int id) {
	int previous = _currentProgram;
	if (_terminate || !_normalwork) {
		return;
	}
	if (id < 0) {
		return;
	}
	if (!_session.isBegin) {
		_log->log(Logger::Type::WARNING, "INTERNAL", "call 'setProgram' without session", 8);
		return;
	}
	Program* p;
	try {
		p = _getProgram(_programs, id);
		_currentProgram = _getpi(_programs, id);
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "INTERNAL", "fail to set program: " + string(e.what()));
		return;
	}

	extboard::startLightEffect(p->effect, 0);
	extboard::flap(p->flap);
	perf::setRelayGroup(p->relayGroup);
	if (p->releivePressure && _currentProgram != previous) {
		perf::relievePressure();
	}
	_tRemainFreeUseTime = _programs[_currentProgram].freeUseTimeSec - _programs[_currentProgram].useTimeSec;
	if (_tRemainFreeUseTime < 0) {
		_tRemainFreeUseTime = 0;
	}

	if (_session.k > 1) {
		render::showFrame(p->bonusFrame);
	} else {
		render::showFrame(p->frame);
	}

	_log->log(Logger::Type::INFO, "PROGRAM", "program '" + p->name + "' set", 8);
	mode = Mode::PROGRAM;
}

void setServiceProgram(int id) {
	if (_terminate) {
		return;
	}
	if (id < 0) {
		return;
	}
	Program* p;
	try {
		p = _getProgram(_servicePrograms, id);
		_currentProgram = _getpi(_servicePrograms, id);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "INTERNAL", "fail to set service program: " + string(e.what()));
		return;
	}

	extboard::startLightEffect(p->effect, 0);
	perf::setRelayGroup(p->relayGroup);
	render::showFrame(p->frame);

	cout << "[INFO][UTILS] service program '" << p->name << "' set" << endl;
	_log->log(Logger::Type::INFO, "PROGRAM", "service program '" + p->name + "' set", 8);
	if (mode != Mode::SERVICE) {
		_tOffServiceMode = time(NULL) + _tServiceMode;
		mode = Mode::SERVICE;
	}
}

void beginSession(Session::Type type, uint64_t id) {
	if (_terminate) {
		return;
	}
	if (_session.isBegin) {
		_log->log(Logger::Type::WARNING, "MONITOR", "try begin session when previous session not end");
		dropSession(Session::EndType::NEW_SESSION);
	}
	double k = bonus::getCoef();
	_session.k = k;
	_session.k100 = round(k*100);
	_session.rk100 = _session.k100 - 100;
	_session.type = type;
	_session.cardid = id;
	
	// clear programs
	for (int i = 0; i < _programs.size(); i++) {
		_programs[i].useTimeSec = 0;
		_programs[i].spendMoney = 0;
	}

	for (int i = 0; i < _servicePrograms.size(); i++) {
		_servicePrograms[i].useTimeSec = 0;
	}
	_session.pays.clear();
	_session.tBegin = time(NULL);
	json sd;
	_putSession(sd, to_string(_session.tBegin));
	if (type == Session::Type::CLIENT) {
		_log->log(Logger::Type::INFO, "MONITOR", "client session begin, k: '" + to_string(_session.k), 8);
	} else
	if (type == Session::Type::SERVICE) {
		_log->log(Logger::Type::INFO, "MONITOR", "service session begin", 8);
	} else
	if (type == Session::Type::COLLECTION) {
		_log->log(Logger::Type::INFO, "MONITOR", "collector session begin", 8);
	}
	_session.isBegin = true;
}

void dropSession(Session::EndType endType) {
	if (!_session.isBegin) {
		return;
	}
	if (_session.type == Session::Type::CLIENT) {
		extboard::dropMoney();
		json sd;
		sd["type"] = "client";
		switch (endType)
		{
		case Session::EndType::BUTTON: sd["end-type"] = "button"; break;
		case Session::EndType::MONEY_RUNOUT: sd["end-type"] = "money-runout"; break;
		case Session::EndType::NEW_SESSION: sd["end-type"] = "new-session"; break;
		default: sd["end-type"] = "none"; break;
		}
		
		if (_session.cardid != 0) {
			sd["card"] = _session.cardid;
		}

		json pays;
		for (int i = 0; i < _session.pays.size(); i++) {
			json p;
			p["count"] = _session.pays[i].count;
			switch (_session.pays[i].type)
			{
			case Payment::Type::CASH: p["type"] = "cash";  break;
			case Payment::Type::TERM: p["type"] = "term";  break;
			case Payment::Type::COIN: p["type"] = "coin";  break;
			case Payment::Type::STORED: p["type"] = "stored";  break;
			case Payment::Type::SERVICE: p["type"] = "service";  break;
			case Payment::Type::BONUS_EXT: p["type"] = "bonus-ext";  break;
			case Payment::Type::BONUS_LOCAL: p["type"] = "bonus-local";  break;
			default: p["type"] = "unknown"; break;
			}
			pays[i] = p;
		}
		sd["pays"] = pays;

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
		_putSession(sd, to_string(_session.tBegin));
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
		_putSession(sd, to_string(_session.tBegin));
	} else
	if (_session.type == Session::Type::COLLECTION) {
		json sd;
		sd["type"] = "collection";
		if (_session.cardid != 0) {
			sd["card"] = _session.cardid;
		}
		_putSession(sd, to_string(_session.tBegin));
	} else {
		_log->log(Logger::Type::ERROR, "MONITOR", "not defined session type");
	}
	_nMoney = 0;
	qrscaner_clear();
	_session.isBegin =  false;
}

void accrueRemainBonusesAndClose() {
	double acrue = _nMoney;
	if (_session.isBegin) {
		acrue = _nMoney / _session.k;
	} else {
		_log->log(Logger::Type::WARNING, "BONUS", "close transaction with " + to_string(_nMoney) + " money, but session not begin");
	}

	try {
		bonus::close(acrue);
		_nMoney = 0;
	} catch (exception& e) {
		_log->log(Logger::Type::ERROR, "BONUS", "fail close transaction with " + to_string(acrue) + " money: " + string(e.what()));
	}
}

bool getLocalCardInfo(CardInfo& cardInfo, uint64_t cardid) {
	for (int i = 0; i < _cards.size(); i++) {
		cout << "\t[GET_LOCAL_CARD] compare " << _cards[i].id << " with " << cardid << endl;
		if (_cards[i].id == cardid) {
			cout << "\t[GET_LOCAL_CARD] EQUAL" << endl; 
			cardInfo = _cards[i];
			return true;
		}
	}

	return false;
}

void addMoney(double nMoney, Payment::Type type) {
	Pay pay;
	pay.count = nMoney;
	pay.type = type;
	_onMoneyAdd(pay);
}

}