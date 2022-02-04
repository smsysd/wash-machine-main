#include "ExtBoard.h"
#include "../mspi-linux/Mspi.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../general-tools/general_tools.h"

#include <wiringPi.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <mutex>

using namespace std;
using json = nlohmann::json;

namespace extboard {

namespace {

	enum class Status {
		ERROR = 0x00,
		NOT_INIT = 0x01,
		INIT_SELF = 0x02,
		LOAD_OPT = 0x03,
		INIT_EXTDEV = 0x05,
		WORK = 0x06
	};
	enum class LOR {
		ST_MASK = 0x03,
		ST_OKC = 0x02,
		ST_OK = 0x01,
		ST_ERROR = 0x00,
		OP_NONE = 0x00,
		OP_RELEIVE = 0x01 << 2,
		OP_RSTEFFECT = 0x02 << 2,
		OP_DIRPERFWR = 0x03 << 2,
		OP_RESET = 0x04 << 2,
		OP_SETRELGR = 0x05 << 2,
		OP_MASK = 0xFC
	};

	enum class ErrorCode {
		NONE = 0x00,
		NO_PERFORMER = 0x02,
		NO_BUTTON_MODULE = 0x03,
		INTERNAL = 0x06
	};

	enum class LightCmd {
		WRITE_BM = 0x01,
		WRITE_LP = 0x03,
		WRITE_IL = 0x05,
		DELAY = 0x07,
		END = 0x00,
		NODELAY_BIT = 0x80
	};
	
	enum class Addr {
		WIA = 0x00,
		VERSION = 0x03,
		LOR = 0x05,
		STATUS = 0x06,
		INTREASON = 0x07,
		RELAY_GROUPS = 0x10,
		DIRECT_SET = 0xD0,
		PERFORMERS_OPT = 0xE0,
		PAYMENT = 0x100,
		PAYMENT_OPT = 0x106,
		BUTTONS_OPT = 0x110,
		BUTTONS = 0x130,
		TEMPSENS_OPT = 0x140,
		TEMPSENS = 0x150,
		RFID_CARD = 0x160,
		RANGEFINDER_OPT = 0x170,
		RANGEFINDER = 0x171,
		FLAP = 0x172,
		EFFECT = 0x200,
		RELEIVE_INS = 0x800,
		PERFORMERS_INFO = 0x1000,
		BUTTONS_INFO = 0x1060,
		ERROR_DESC = 0x1100
	};

	enum class Cmd {
		RESET = 0x00,
		END_OPT = 0x0A,
		RELIEVE_PRESSURE = 0x10,
		FLAP = 0x12,
		RESET_EFFECT = 0x20,
		APPLAY_RGROUP = 0x30
	};

	enum class IntReason {
		BOOTLOADER = 0x01,
		WAIT_OPT = 0x02,
		ERROR = 0x03,
		BUTTON = 0x04,
		CARD = 0x05,
		MONEY = 0x06,
		OBJ_CLOSER = 0x07,
		OBJ_WASTE = 0x08,
		EFFECTEND = 0x09,
		LOR = 0x0A
	};

	struct LightInstruction {
		uint8_t cmd;
		uint8_t data[4];
	};

	struct LightEffect {
		int id;
		int delay;
		int nRepeats;
		bool needReset;
		int nResetInstructions;
		int nTotalInstructions;
		LightInstruction instructions[256];
		uint8_t data[1288];
	};

	struct Payment {
		enum Mode {
			NOT_USE = 0x00,
			COUNTER = 0x01,
			BILL_CODE = 0x02
		};
		Mode mode;
		double rate;
		bool usedbillcodes[32];
		double billcodes[32];
		int maxbillcode;
	};

	struct PerformerUnit {
		int addr;
		int normalStates;
		int dependencies[6];

		bool operator!=(const PerformerUnit& rv) {
			if (addr != rv.addr) {
				return true;
			}
			if (normalStates != rv.normalStates) {
				return true;
			}
			for (int i = 0; i < 6; i++) {
				if (dependencies[i] != rv.dependencies[i]) {
					return true;
				}
			}

			return false;
		}
	};

	struct RelaysGroup {
		int id;
		int addr[4];
		int state[4];
		
		bool operator!=(const RelaysGroup& rv) {
			for (int i = 0; i < 4; i++) {
				if (addr[i] != rv.addr[i]) {
					return true;
				}
				if (state[i] != rv.state[i]) {
					return true;
				}
			}
			return false;
		}
	};

	struct ButtonModule {
		int addr;
		int activeLevels;
		int io;

		bool operator!=(const ButtonModule& rv) {
			if (addr != rv.addr) {
				return true;
			}
			if (activeLevels != rv.activeLevels) {
				return true;
			}
			if (io != rv.io) {
				return true;
			}
			return false;
		}
	};

	struct Led {
		enum Type {
			LEDPANEL,
			BUTTONMODULE,
			EXTBOARD
		};
		int id;
		Type type;
		int addr;
		int i;
		bool state;
	};

	enum class HandleType {
		START_EFFECT = 1,	// data: effect_id effect_index
		RESET_EFFECT,		// data: effect_index
		RELEIVE_PRESSURE,	// data: none
		SET_RELAY_GROUP,	// data: group_id 
		DIRECT_SET_RELAYS,	// data: addr, states
		FLAP				// data: state
	};

	struct Handle {
		HandleType type;
		uint8_t data[8];
	};

	Mspi* _mspi = nullptr;
	vector<Led> _leds;
	vector<LightEffect> _effects;
	vector<PerformerUnit> _performersu;
	vector<RelaysGroup> _rgroups;
	vector<ButtonModule> _buttonms;
	ButtonModule _buttoneb;
	uint8_t _buttonms_io[8] = {0};
	Payment _cash;
	Payment _coin;
	Payment _terminal;
	uint8_t _releiveInstructions[128];
	int _nReleiveInstructions;
	int _closerRange = -1;
	uint64_t _tempSens1Addr = 0;
	uint64_t _tempSens2Addr = 0;
	int _giveMoneyEffect = -1;
	int _serviceEffect = -1;
	bool _isInit = false;
	pthread_t _thread_id;
	Fifo _operations;
	int _currentRelayGroupId = -1;
	int _nrstPin = -1;
	uint8_t _lor = (uint8_t)LOR::OP_NONE | (uint8_t)LOR::ST_OK;
	mutex _mutex;

	void (*_onButton)(int iButton);
	void (*_onCard)(uint64_t id);
	void (*_onMoney)(double nMoney);
	void (*_onCloser)(bool state);
	void (*_onError)(ErrorType et, string text);

	uint64_t _collectn(uint8_t* src, int nb) {
		uint64_t val = 0;
		for (int i = 0; i < nb; i++) {
			val |= src[i] << ((nb - i - 1) * 8);
		}
		return val;
	}

	void _distribn(uint8_t* dst, uint64_t val, int nb) {
		for (int i = 0; i < nb; i++) {
			dst[i] = (uint8_t) (val >> ((nb - i -1) * 8));
		}
	}

	int _get_bmi(uint8_t addr) {
		for (int i = 0; i < _buttonms.size(); i++) {
			if (_buttonms[i].addr == addr) {
				return i;
			}
		}
		throw runtime_error("button module with '" + to_string(addr) + "' address not found");
	}

	int _get_rgi(int id) {
		for (int i = 0; i < _rgroups.size(); i++) {
			if (_rgroups[i].id == id) {
				return i;
			}
		}
		throw runtime_error("relay group with '" + to_string(id) + "' id not found");
	}

	template<typename T>
	bool _is_contain(vector<T> vec, T val) {
		for (int i = 0; i < vec.size(); i++) {
			if (vec[i] == val) {
				return true;
			}
		}

		return false;
	}

	Led* _getLed(int id) {
		for (int i = 0; i < _leds.size(); i++) {
			if (_leds[i].id == id) {
				return &_leds[i];
			}
		}
		throw runtime_error("led '" + to_string(id) + "' not found");
	}

	LightEffect* _getEffect(int id) {
		for (int i = 0; i < _effects.size(); i++) {
			if (_effects[i].id == id) {
				return &_effects[i];
			}
		}
		throw runtime_error("effect '" + to_string(id) + "' not found");
	}

	void _appendLightIns(vector<LightInstruction>& iv, json& ins) {
		vector<Led> leds;
		bool setf = false;
		bool resetf = false;
		json set;
		json reset;
		try {
			set = ins["set"];
			if (set != nullptr) {
				setf = true;
			}
		} catch (exception& e) {}
		try {
			reset = ins["reset"];
			if (reset != nullptr) {
				resetf = true;
			}
		} catch (exception& e) {}
		if (setf) {
			if (set.is_number_integer()) {
				int id = set;
				Led* led = _getLed(id);
				led->state = true;
				leds.push_back(*led);
			} else
			if (set.is_array()) {
				for (int i = 0; i < set.size(); i++) {
					int id = set[i];
					Led* led = _getLed(id);
					led->state = true;
					leds.push_back(*led);
				}
			}
		}
		if (resetf) {
			if (reset.is_number_integer()) {
				int id = reset;
				Led* led = _getLed(id);
				led->state = false;
				leds.push_back(*led);
			} else
			if (reset.is_array()) {
				for (int i = 0; i < reset.size(); i++) {
					int id = reset[i];
					Led* led = _getLed(id);
					led->state = false;
					leds.push_back(*led);
				}
			}
		}

		if (!setf && !resetf) {
			int delay = ins["delay"];
			if (delay > 65535) {
				delay = 65535;
			}
			LightInstruction li;
			li.cmd = (uint8_t)LightCmd::DELAY;
			li.data[0] = delay >> 8;
			li.data[1] = delay & 0xFF;
			iv.push_back(li);
		} else {
			vector<Led> lp;
			vector<Led> eb;
			vector<Led> bm;
			// sort leds by commands (on extboard, on button module, on ledpanel)
			for (int i = 0; i < leds.size(); i++) {
				if (leds[i].type == Led::LEDPANEL) {
					lp.push_back(leds[i]);
				} else
				if (leds[i].type == Led::EXTBOARD) {
					eb.push_back(leds[i]);
				} else
				if (leds[i].type == Led::BUTTONMODULE) {
					bm.push_back(leds[i]);
				} else {
					throw runtime_error("undefined led type");
				}
			}

			// create instructions
			vector<LightInstruction> aiv;
			// insert set/reset bits in lp instructions
			if (lp.size() > 0) {
				LightInstruction li;
				li.cmd = (uint8_t)LightCmd::WRITE_LP;
				int setbits = 0;
				int resetbits = 0;
				for (int i = 0; i < lp.size(); i++) {
					if (lp[i].state) {
						setbits |= 1 << lp[i].i;
					} else {
						resetbits |= 1 << lp[i].i;
					}
				}
				li.data[0] = resetbits >> 8;
				li.data[1] = resetbits & 0xFF;
				li.data[2] = setbits >> 8;
				li.data[3] = setbits & 0xFF;				
				aiv.push_back(li);
			}
			// insert set/reset bits in eb instructions
			if (eb.size() > 0) {
				LightInstruction li;
				li.cmd = (uint8_t)LightCmd::WRITE_IL;
				int setbits = 0;
				int resetbits = 0;
				for (int i = 0; i < eb.size(); i++) {
					if (eb[i].state) {
						setbits |= 1 << eb[i].i;
					} else {
						resetbits |= 1 << eb[i].i;
					}
				} 
				li.data[0] = resetbits;
				li.data[1] = setbits;
				aiv.push_back(li);
			}

			// sort leds by bm index
			vector<vector<Led>> bms(8);
			for (int i = 0; i < bm.size(); i++) {
				uint8_t bmi = _get_bmi(bm[i].addr);
				bms[bmi].push_back(bm[i]);
			}
			
			// handle all bms
			for (int i = 0; i < 8; i++) {
				if (bms[i].size() > 0) {
					LightInstruction li;
					li.cmd = (uint8_t)LightCmd::WRITE_BM;
					int setbits = 0;
					int resetbits = 0;
					// insert set/reset bits in bm instructions
					for (int j = 0; j < bms[i].size(); j++) {
						if (bms[i][j].state) {
							setbits |= 1 << bms[i][j].i;
						} else {
							resetbits |= 1 << bms[i][j].i;
						}
					}
					li.data[0] = i;
					li.data[1] = resetbits;
					li.data[2] = setbits;
					aiv.push_back(li);
				}
			}

			if (aiv.size() > 32) {
				throw runtime_error("too many once time instructions");
			}

			if (aiv.size() > 1) {
				for (int i = 0; i < aiv.size() - 1; i++) {
					aiv[i].cmd |= (uint8_t)LightCmd::NODELAY_BIT;
				}
			}

			for (int i = 0; i < aiv.size(); i++) {
				iv.push_back(aiv[i]);
			}
		}
	}

	void _loadPayment(json& src, Payment& dst, string name) {
		string mode = JParser::getf(src, "mode", "payment " + name);
		if (mode == "not-use") {
			dst.mode = Payment::NOT_USE;
		} else
		if (mode == "counter") {
			dst.mode = Payment::COUNTER;
			dst.rate = JParser::getf(src, "rate", "payment " + name);
		} else
		if (mode == "bill-code") {
			dst.mode = Payment::BILL_CODE;
			json& bcs = JParser::getf(src, "bill-codes", "payment " + name);
			dst.maxbillcode = 0;
			for (int i = 0; i < 32; i++) {
				dst.usedbillcodes[i] = false;
			}
			for (int i = 0; i < bcs.size(); i++) {
				try {
					int bc = bcs[i][0];
					int bn = bcs[i][1];
					if (bc < 1 || bc > 31) {
						throw runtime_error("incorrect bill code, must be [1:31]");
					}
					dst.usedbillcodes[bc] = true;
					dst.billcodes[bc] = bn;
					if (bc > dst.maxbillcode) {
						dst.maxbillcode = bc;
					}
				} catch (exception& e) {
					throw runtime_error("fail load '" + to_string(i) + "' bill code for " + name + ": " + string(e.what()));
				}
			}
		} else {
			throw runtime_error("unknown payment mode '" + mode + "' for cash");
		}
	}

	double _injectMoney(Payment& pm, uint16_t val) {
		if (pm.mode == Payment::NOT_USE) {
			return 0;
		} else
		if (pm.mode == Payment::COUNTER) {
			return (double)val * pm.rate;
		} else
		if (pm.mode == Payment::BILL_CODE) {
			if (val > pm.maxbillcode) {
				cout << "[WARNING][EXTBOARD] received " << val << " bill code number, it is overflow, set max bill code: " << pm.maxbillcode << endl;
				return pm.billcodes[pm.maxbillcode];
			}
			if (!pm.usedbillcodes[val]) {
				int nextbc = pm.maxbillcode;
				for (int i = val; i < 32; i++) {
					if (pm.usedbillcodes[i]) {
						nextbc = i;
					}
				}
				cout << "[WARNING][EXTBOARD] received " << val << " bill code number, it is not used, set next bill code: " << nextbc << endl;
				return pm.billcodes[nextbc];
			}
			return pm.billcodes[val];
		} else {
			return 0;
		}
	}

	double _injectMoney(uint8_t* buf) {
		uint16_t ncash = _collectn(buf, 2);
		uint16_t nterm = _collectn(&buf[2], 2);
		uint16_t ncoin = _collectn(&buf[4], 2);
		double money = 0;
		if (ncash > 0) {
			money += _injectMoney(_cash, ncash);
		}
		if (nterm > 0) {
			money += _injectMoney(_terminal, nterm);
		}
		if (ncoin > 0) {
			money += _injectMoney(_coin, ncoin);
		}
		return money;
	}

	void _put_cash(double nMoney) {
		ofstream f("./.cash", ios_base::app);
		f << nMoney << endl;
		f.close();
	}

	void _getErrorAndThrow() {
		uint8_t buf[128];
		memset(buf, 0, sizeof(buf));
		
		try {
			_mspi->read((int)Addr::ERROR_DESC, buf, 64);
		} catch (exception& e) {
			throw runtime_error("FRED");
		}
		
		if (buf[0] == (uint8_t)ErrorCode::INTERNAL) {
			throw runtime_error((char*)&buf[1]);
		} else
		if (buf[0] == (uint8_t)ErrorCode::NONE) {
			throw runtime_error("NONE");
		} else
		if (buf[0] == (uint8_t)ErrorCode::NO_BUTTON_MODULE) {
			throw runtime_error("NO BM " + to_string((int)buf[1]));
		} else
		if (buf[0] == (uint8_t)ErrorCode::NO_PERFORMER) {
			throw runtime_error("NO PERF " + to_string((int)buf[1]));
		}

		throw runtime_error("UNKNOWN");
	}

	void _connect () {
		uint8_t buf[8];
		int tries = 0;
		int timeout;

		try {
			if (_nrstPin > 0) {
				pinMode(_nrstPin, OUTPUT);
				digitalWrite(_nrstPin, 0);
				usleep(10000);
				digitalWrite(_nrstPin, 1);
			} else {
				_mspi->cmd((int)Cmd::RESET, 0);
			}
			sleep(1); // bootloader
			_mspi->read((int)Addr::WIA, buf, 3);
			_mspi->read((int)Addr::VERSION, &buf[3], 2);
		} catch (exception& e) {
			sleep(2);
			tries++;
			if (tries >= 10) {
				throw runtime_error("fail to connect");
			}
		}
		cout << "extboard connected, WIA_PRJ: " << (int)(buf[0] << 8 | buf[1]) << ", WIA_SUB: " << (int)buf[2] << ", VERSION: [" << (int)buf[3] << ", " << (int)buf[4] << "]" << endl;
		timeout = 0;
		while (true) {
			usleep(100000);
			_mspi->read((int)Addr::STATUS, buf, 1);
			if (buf[0] == (uint8_t)Status::LOAD_OPT) {
				break;
			}
			timeout++;
			if (timeout > 100) {
				throw runtime_error("too long self init");
			}
		}
	}

	void _uploadAllOptions() {
		uint8_t buf[256];
		memset(buf, 0, 32);
		for (int i = 0; i < _performersu.size(); i++) {
			buf[i*8] = _performersu[i].addr;
			buf[i*8 + 1] = _performersu[i].normalStates;
			buf[i*8 + 2] = _performersu[i].dependencies[0];
			buf[i*8 + 3] = _performersu[i].dependencies[1];
			buf[i*8 + 4] = _performersu[i].dependencies[2];
			buf[i*8 + 5] = _performersu[i].dependencies[3];
			buf[i*8 + 6] = _performersu[i].dependencies[4];
			buf[i*8 + 7] = _performersu[i].dependencies[5];
		}
		_mspi->write((int)Addr::PERFORMERS_OPT, buf, 32);

		memset(buf, 0, 192);
		for (int i = 0; i < _rgroups.size(); i++) {
			for (int j = 0; j < 4; j++) {
				buf[i*8 + j*2] = _rgroups[i].addr[j];
				buf[i*8 + j*2 + 1] = _rgroups[i].state[j];
			}
		}
		_mspi->write((int)Addr::RELAY_GROUPS, buf, 192);

		memset(buf, 0, 26);
		buf[0] = _buttoneb.io;
		buf[1] = _buttoneb.activeLevels;
		for (int i = 0; i < _buttonms.size(); i++) {
			buf[2 + i*3] = _buttonms[i].addr;
			buf[2 + i*3 + 1] = _buttonms[i].io;
			buf[2 + i*3 + 2] = _buttonms[i].activeLevels;
		}
		_mspi->write((int)Addr::BUTTONS_OPT, buf, 26);

		memset(buf, 0, 1);
		buf[0] = _closerRange;
		_mspi->write((int)Addr::RANGEFINDER_OPT, buf, 1);

		memset(buf, 0, 16);
		_distribn(buf, _tempSens1Addr, 8);
		_distribn(&buf[8], _tempSens2Addr, 8);
		_mspi->write((int)Addr::TEMPSENS_OPT, buf, 16);

		memset(buf, 0, _nReleiveInstructions * 2);
		memcpy(buf, _releiveInstructions, _nReleiveInstructions * 2);
		_mspi->write((int)Addr::RELEIVE_INS, buf, _nReleiveInstructions * 2);
	}

	void _initExtdev() {
		uint8_t buf[4];
		_mspi->cmd((int)Cmd::END_OPT, 0);
		int timeout = 0;
		while (true) {
			usleep(250000);
			_mspi->read((int)Addr::STATUS, buf, 1);
			if (buf[0] == (uint8_t)Status::WORK) {
				break;
			} else
			if (buf[0] == (uint8_t)Status::INIT_EXTDEV) {
				timeout++;
				if (timeout > 400) { // ~20 seconds
					throw ("TIMEOUT");
				}
			} else if (buf[0] == (uint8_t)Status::ERROR) {
				_getErrorAndThrow();
			} else {
				throw runtime_error("INC STATUS " + to_string((int)buf[0]));
			}
		}
	}

	uint8_t _getLor() {
		uint8_t temp;
		temp =_lor;
		_lor = (uint8_t)LOR::OP_NONE | (uint8_t)LOR::ST_OK;
		return temp;
	}

	void _int(int reason) {
		static int suspicion = 0;
		cout << "[INFO][EXTBOARD]|INT| " << reason << endl;
		_mutex.lock();
		uint8_t buf[32] = {0};
		if (reason == (int)IntReason::ERROR) {
			if (!_isInit) {
				return;
			}
			uint8_t buf[128];
			memset(buf, 0, sizeof(buf));
			try {
				_isInit = false;
				_mspi->disableInt();
				_mspi->read((int)Addr::ERROR_DESC, buf, 64);
				suspicion = 0;
				if (buf[0] == (uint8_t)ErrorCode::INTERNAL) {
					_mutex.unlock();
					_onError(ErrorType::INTERNAL, string((char*)&buf[1]));
					return;
				} else
				if (buf[0] == (uint8_t)ErrorCode::NONE) {
					_mutex.unlock();
					_onError(ErrorType::INTERNAL, "NONE");
					return;
				} else
				if (buf[0] == (uint8_t)ErrorCode::NO_BUTTON_MODULE) {
					_mutex.unlock();
					_onError(ErrorType::DISCONNECT_DEV, "NO BM " + to_string((int)buf[1]));
					return;
				} else
				if (buf[0] == (uint8_t)ErrorCode::NO_PERFORMER) {
					_mutex.unlock();
					_onError(ErrorType::DISCONNECT_DEV, "NO PERF " + to_string((int)buf[1]));
					return;
				} else {
					_mutex.unlock();
					_onError(ErrorType::INTERNAL, "UNKNOWN");
					return;
				}

			} catch (exception& e) {
				suspicion++;
			}
		} else
		if (reason == (int)IntReason::WAIT_OPT) {
			_isInit = false;
			_mspi->disableInt();
		} else
		if (reason == (int)IntReason::LOR) {
			if (!_isInit) {
				return;
			}
			try {
				_mspi->read((int)Addr::LOR, buf, 1);
				cout << "[INFO][ETBOARD] LOR " << (int)buf[0] << endl;
				_lor = buf[0];
			} catch (exception& e) {
				cout << "[WARNING][EXTBOARD]|INT| fail to read LOR: " << e.what() << endl;
				suspicion++;
			}
		} else
		if (reason == (int)IntReason::BUTTON && _onButton != nullptr) {
			if (!_isInit) {
				return;
			}
			try {
				_mspi->read((int)Addr::BUTTONS, buf, 9);
				suspicion = 0;
				buf[0] &= _buttoneb.io; // this mean used mask - not io
				for (int i = 0; i < 8; i++) {
					if (buf[0] & (1 << i)) {
						_onButton(i);
					}
				}
				for (int i = 0; i < 8; i++) {
					if (i < _buttonms.size()) {
						buf[i+1] &= ~_buttonms[i].io;
						for (int j = 0; j < 8; j++) {
							if (buf[i+1] & (1 << j)) {
								_onButton(8 + i*8 + j);
							}
						}
					}
				}
			} catch (exception& e) {
				cout << "[WARNING][EXTBOARD]|INT| fail to read buttons: " << e.what() << endl;
				suspicion++;
			}
		} else
		if (reason == (int)IntReason::CARD && _onCard != nullptr) {
			if (!_isInit) {
				return;
			}
			try {
				_mspi->read((int)Addr::RFID_CARD, buf, 8);
				suspicion = 0;
				uint64_t cid = _collectn(buf, 8);
				_onCard(cid);
			} catch (exception& e) {
				cout << "fail to read card: " << e.what() << endl;
				suspicion++;
			}
		} else
		if (reason == (int)IntReason::MONEY && _onMoney != nullptr) {
			if (!_isInit) {
				return;
			}
			try {
				_mspi->read((int)Addr::PAYMENT, buf, 6);
				suspicion = 0;
				double money = _injectMoney(buf);
				if (money > 0) {
					_onMoney(money);
				}
			} catch (exception& e) {
				cout << "fail to read money: " << e.what() << endl;
				suspicion++;
			}
		} else
		if (reason == (int)IntReason::OBJ_CLOSER && _onCloser != nullptr) {
			if (!_isInit) {
				return;
			}
			_onCloser(true);
		} else
		if (reason == (int)IntReason::OBJ_WASTE && _onCloser != nullptr) {
			if (!_isInit) {
				return;
			}
			_onCloser(false);
		}
		_mutex.unlock();
		if (suspicion > 10) {
			_onError(ErrorType::DISCONNECT_DEV, "EXTBOARD");
		}
	}

	void* _handler(void* arg) {
		uint8_t buf[2048];
		int suspicion = 0;
		int borehole = 0;
		bool hpos;
		int tries;
		while (true) {
			_mutex.lock();
			tries = 0;
			while (!_isInit) {
				try {
					cout << "[WARNING][EXTBOARD] extboard was reset during operations - reinit.." << endl;
					hpos = false;
					_connect();
					_uploadAllOptions();
					hpos = true;
					usleep(50000);
					_initExtdev();
					_isInit = true;
					_onError(ErrorType::NONE, "");
					_mspi->enableInt();
					suspicion = 0;
					if (_currentRelayGroupId >= 0) {
						usleep(100000);
						setRelayGroup(_currentRelayGroupId);
					}
				} catch (exception& e) {
					tries++;
					sleep(2);
					cout << "[ERROR][EXTBOARD] fail to reinit: " << e.what() << endl;
					if (tries > 3) {
						if (hpos) {
							_onError(ErrorType::DISCONNECT_DEV, string(e.what()));
						} else {
							_onError(ErrorType::DISCONNECT_DEV, "EXTBOARD");
						}
					}
				}
			}
			try {
				Handle* h = (Handle*)fifo_get(&_operations);
				if (h != nullptr) {
					cout << "[INFO][EXTBOARD] handling " << (int)h->type << ", total fifo size " << _operations.nElements << endl;
				}
				if (h == nullptr) {
					usleep(10000);
				} else
				if (h->type == HandleType::DIRECT_SET_RELAYS) {
					buf[0] = h->data[0];
					buf[1] = h->data[1];
					_mspi->write((int)Addr::DIRECT_SET, buf, 2);
					fifo_pop(&_operations);
					suspicion = 0;
				} else
				if (h->type == HandleType::RELEIVE_PRESSURE) {
					_mspi->cmd((int)Cmd::RELIEVE_PRESSURE, 0);
					_mutex.unlock();
					int timeout = 0;
					while (true) {
						usleep(100000);
						if (_getLor() == ((uint8_t)LOR::OP_RELEIVE | (uint8_t)LOR::ST_OK)) {
							break;
						}
						timeout++;
						if (timeout > 200) {
							suspicion++;
							break;
						}
					}
					fifo_pop(&_operations);
					suspicion = 0;
				} else
				if (h->type == HandleType::RESET_EFFECT) {
					_mspi->cmd((int)Cmd::RESET_EFFECT, h->data[0]);
					while (true) {
						usleep(20000);
						_mspi->read((int)Addr::LOR, buf, 1);
						if (buf[0] != (uint8_t)OKC) {
							break;
						}
					}
					fifo_pop(&_operations);
					suspicion = 0;
				} else
				if (h->type == HandleType::SET_RELAY_GROUP) {
					int rgi;
					try {
						rgi = _get_rgi(h->data[0]);
					} catch (exception& e) {
						fifo_pop(&_operations);
						throw runtime_error(e.what());
					}
					_mspi->cmd((int)Cmd::APPLAY_RGROUP, rgi);
					_mutex.unlock();
					int timeout = 0;
					while (true) {
						usleep(100000);
						if (_getLor() == ((uint8_t)LOR::OP_SETRELGR | (uint8_t)LOR::ST_OK)) {
							break;
						}
						timeout++;
						if (timeout > 100) {
							suspicion++;
							break;
						}
					}
					fifo_pop(&_operations);
					suspicion = 0;
				} else
				if (h->type == HandleType::FLAP) {
					_mspi->cmd((int)Cmd::FLAP, h->data[0]);
					fifo_pop(&_operations);
					suspicion = 0;
				} else
				if (h->type == HandleType::START_EFFECT) {
					fifo_pop(&_operations);
					try {
						LightEffect* ef = _getEffect(h->data[0]);
						ef->data[0] = h->data[1];
						_mspi->write((int)Addr::EFFECT, ef->data, 8 + ef->nTotalInstructions*5);
						suspicion = 0;
					} catch (exception& e) {
						cout << "[WARNING][EXTBOARD]|HANDLER| fail to start effect: " << e.what() << endl;
					}
				} else {
					fifo_pop(&_operations);
					cout << "[WARNING][EXTBOARD]|HANDLER| undefined handle: " << (int)h->type << endl;
				}
			} catch (exception& e) {
				cout << "[WARNING][EXTBOARD]|HANDLER| fail handle: " << e.what() << endl;
				suspicion++;
				usleep(100000);
			}
			_mutex.unlock();
			if (suspicion > 5) {
				suspicion = 0;
				_isInit = false;
				_mspi->disableInt();
				_onError(ErrorType::DISCONNECT_DEV, "EXTBOARD");
			}
			if (borehole % 1000 == 0) {
				// add repetive handles
				if (_currentRelayGroupId >= 0 && _operations.nElements == 0) {
					setRelayGroup(_currentRelayGroupId);
				}
			}
			usleep(20000);
			borehole++;
		}
	}
}

void init(json& extboard, json& performingUnits, json& relaysGroups, json& payment, json& buttons, json& rangeFinder, json& tempSens, json& leds, json& effects, json& specEffects, json& releiveInstructions) {
	// get hw extboard config
	cout << "[INFO][EXTBOARD] load hardware extboard config.." << endl;
	string driver = JParser::getf(extboard, "driver", "extboard");
	int speed = JParser::getf(extboard, "speed", "extboard");
	int csPin = JParser::getf(extboard, "cs-pin", "extboard");
	int intPin = JParser::getf(extboard, "int-pin", "extboard");
	_nrstPin = JParser::getf(extboard, "nrst-pin", "extboard");

	cout << "[INFO][EXTBOARD] load performing units config.." << endl;
	for (int i = 0; i < performingUnits.size(); i++) {
		try {
			PerformerUnit pu;
			pu.addr = JParser::getf(performingUnits[i], "address", "");
			pu.normalStates = JParser::getf(performingUnits[i], "normal-states", "");
			json& dep = JParser::getf(performingUnits[i], "dependencies", "");
			if (dep.size() != 6) {
				throw runtime_error("dependencies must be specified for all 6 relays");
			}
			for (int j = 0; j < 6; j++) {
				pu.dependencies[j] = dep[j];
			}
			_performersu.push_back(pu);
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' performer unit: " + string(e.what()));
		}
	}
	int rbuff[256] = {0};
	for (int i = 0; i < _performersu.size(); i++) {
		rbuff[_performersu[i].addr]++;
	}
	for (int i = 1; i < 256; i++) {
		if (rbuff[i] > 1) {
			throw runtime_error("address '" + to_string(i) + "' of performer unit was repeated");
		}
	}

	cout << "[INFO][EXTBOARD] load relays groups.." << endl;
	for (int i = 0; i < relaysGroups.size(); i++) {
		try {
			RelaysGroup rg = {0};
			rg.id = JParser::getf(relaysGroups[i], "id", "relay-group");
			json& ps = JParser::getf(relaysGroups[i], "performers", "relay-group");
			if (ps.size() > 4) {
				throw runtime_error("performers may be 4 or less");
			}
			for (int j = 0; j < ps.size(); j++) {
				try {
					rg.addr[j] = JParser::getf(ps[j], "address", "performer");
					rg.state[j] = JParser::getf(ps[j], "state", "performer");
				} catch (exception& e) {
					throw runtime_error("fail load '" + to_string(j) + "' performer: " + string(e.what()));
				}
			}
			_rgroups.push_back(rg);
		} catch (exception& e) {
			throw runtime_error("fail load '" + to_string(i) + "' relay group: " + string(e.what()));
		}
	}
	
	cout << "[INFO][EXTBOARD] load payment hardware config.." << endl;
	json& cash = JParser::getf(payment, "cash", "payment");
	_loadPayment(cash, _cash, "cash");
	json& coin = JParser::getf(payment, "coin", "payment");
	_loadPayment(coin, _coin, "coin");
	json& terminal = JParser::getf(payment, "terminal", "payment");
	_loadPayment(terminal, _terminal, "terminal");

	cout << "[INFO][EXTBOARD] load buttons hardware config.." << endl;
	json& bseb = JParser::getf(buttons, "extboard", "buttons");
	_buttoneb.addr = 0;
	_buttoneb.activeLevels = JParser::getf(bseb, "active-levels", "buttons extborad");
	_buttoneb.io = JParser::getf(bseb, "used-mask", "buttons extborad");
	json& bsms = JParser::getf(buttons, "modules", "buttons");
	if (bsms.size() > 8) {
		throw runtime_error("buttons modules must be 8 or less");
	}
	for (int i = 0; i < bsms.size(); i++) {
		try {
			ButtonModule bm;
			bm.addr = JParser::getf(bsms[i], "address", "");
			bm.activeLevels = JParser::getf(bsms[i], "active-levels", "");
			bm.io = JParser::getf(bsms[i], "io-mask", "");
			_buttonms.push_back(bm);
		} catch (exception& e) {
			throw runtime_error("fail load '" + to_string(i) + "' buttons module: " + string(e.what()));
		}
	}
	memset(rbuff, 0, 256);
	for (int i = 0; i < _buttonms.size(); i++) {
		rbuff[_buttonms[i].addr]++;
	}
	for (int i = 1; i < 256; i++) {
		if (rbuff[i] > 1) {
			throw runtime_error("address '" + to_string(i) + "' of buttons module was repeated");
		}
	}

	cout << "[INFO][EXTBOARD] load range-finder hardware config.." << endl;
	_closerRange = JParser::getf(rangeFinder, "closer-range", "range-finder");

	cout << "[INFO][EXTBOARD] load temperature sensors hardware config.." << endl;
	_tempSens1Addr = JParser::getf(tempSens[0], "address", "temp-sens '0'");
	_tempSens2Addr = JParser::getf(tempSens[1], "address", "temp-sens '1'");

	cout << "[INFO][EXTBOARD] load releive instructions.." << endl;
	memset(_releiveInstructions, 0, sizeof(_releiveInstructions));
	_nReleiveInstructions = 0;
	for (int i = 0; i < releiveInstructions.size(); i++) {
		try {
			bool success = false;
			json& ins = releiveInstructions[i];
			try {
				json& set = ins["set"];
				uint8_t addr = set[0];
				uint8_t states = set[1];	
				_releiveInstructions[i*2] = addr;
				_releiveInstructions[i*2 + 1] = states;
			} catch (exception& e) {
				int delay = ins["delay"];
				_releiveInstructions[i*2] = 255;
				_releiveInstructions[i*2 + 1] = delay / 100;
			}
			// cout << "ins " << i << " added: " << (int)_releiveInstructions[i*2] << " " << (int)_releiveInstructions[i*2 + 1] << " " << endl;
			_nReleiveInstructions++;
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' releive instruction: " + string(e.what()));
		}
	}
	

	cout << "[INFO][EXTBOARD] load leds.." << endl;
	for (int i = 0; i < leds.size(); i++) {
		try {
			Led led;
			led.id = JParser::getf(leds[i], "id", "");
			led.i = JParser::getf(leds[i], "index", "");
			string lt = JParser::getf(leds[i], "type", "");
			if (lt == "lp") {
				led.type = Led::LEDPANEL;
			} else
			if (lt == "bm") {
				led.type = Led::BUTTONMODULE;
				led.addr = JParser::getf(leds[i], "address", "");
				if (led.i > 7) {
					throw runtime_error("index on button module must be 0..7");
				}
			} else
			if (lt == "eb") {
				led.type = Led::EXTBOARD;
			} else {
				throw runtime_error("unknown type '" + lt + "', must be 'lp', 'bm' or 'eb'");
			}
			_leds.push_back(led);
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' led: " + string(e.what()));
		}
	}

	cout << "[INFO][EXTBOARD] load effects instructions.." << endl;
	for (int i = 0; i < effects.size(); i++) {
		try {
			LightEffect ef;
			ef.id = JParser::getf(effects[i], "id", "");
			ef.delay = JParser::getf(effects[i], "delay", "");
			ef.nRepeats = JParser::getf(effects[i], "repeats", "");
			json& ins = JParser::getf(effects[i], "instructions", "");
			vector<LightInstruction> iv;
			vector<LightInstruction> rstiv;
			for (int j = 0; j < ins.size(); j++) {
				_appendLightIns(iv, ins[j]);
			}
			// iv[iv.size()-1].cmd |= (uint8_t)LightCmd::NODELAY_BIT;
			LightInstruction eli;
			eli.cmd = (uint8_t)LightCmd::END;
			iv.push_back(eli);

			bool needRst = JParser::getf(effects[i], "reset", "");
			if (needRst) {
				ef.needReset = 1;
				json& rstIns = JParser::getf(effects[i], "reset-instructions", "");
				for (int j = 0; j < rstIns.size(); j++) {
					_appendLightIns(rstiv, rstIns[j]);
				}
				if (rstiv.size() > 64) {
					throw runtime_error("too many reset-instructions");
				}
				rstiv[rstiv.size()-1].cmd |= (uint8_t)LightCmd::NODELAY_BIT;
				rstiv.push_back(eli);
				ef.nResetInstructions = rstiv.size();
			} else {
				ef.needReset = false;
				ef.nResetInstructions = 0;
			}
			if (iv.size() + rstiv.size() > 256) {
				throw runtime_error("too many total instructions");
			}
			for (int j = 0; j < rstiv.size(); j++) {
				ef.instructions[j] = rstiv[j];
			}
			for (int j = 0; j < iv.size(); j++) {
				ef.instructions[j + rstiv.size()] = iv[j];
			}
			ef.nTotalInstructions = iv.size() + rstiv.size();
			memset(ef.data, 0, 1288);
			ef.data[1] = (uint8_t)(ef.delay >> 8);
			ef.data[2] = (uint8_t)(ef.delay);
			ef.data[3] = ef.nRepeats;
			ef.data[4] = ef.needReset;
			ef.data[5] = ef.nResetInstructions;
			for (int j = 0; j < ef.nTotalInstructions; j++) {
				ef.data[8 + j*5] = ef.instructions[j].cmd;
				ef.data[8 + j*5 + 1] = ef.instructions[j].data[0];
				ef.data[8 + j*5 + 2] = ef.instructions[j].data[1];
				ef.data[8 + j*5 + 3] = ef.instructions[j].data[2];
				ef.data[8 + j*5 + 4] = ef.instructions[j].data[3];
			}
			_effects.push_back(ef);
			cout << "[INFO][EXTBOARD] effect '" << ef.id << "' loaded: " << "nTI - " << ef.nTotalInstructions << " nRI - " << ef.nResetInstructions;
			// for (int j = 0; j < ef.nTotalInstructions; j++) {
			// 	if (ef.instructions[j].cmd & 0x80) {
			// 		printf(" |%X %X %X %X %X|* ", ef.instructions[j].cmd & 0x7F, ef.instructions[j].data[0], ef.instructions[j].data[1], ef.instructions[j].data[2], ef.instructions[j].data[3]);
			// 	} else {
			// 		printf(" |%X %X %X %X %X| ", ef.instructions[j].cmd & 0x7F, ef.instructions[j].data[0], ef.instructions[j].data[1], ef.instructions[j].data[2], ef.instructions[j].data[3]);
			// 	}
			// }
			cout << endl;
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' effect: " + string(e.what()));
		}
	}
	_giveMoneyEffect = JParser::getf(specEffects, "give-money", "spec-effects");
	_serviceEffect = JParser::getf(specEffects, "service", "spec-effects");

	cout << "[INFO][EXTBOARD] assert spec-effects.." << endl;
	try {
		_getEffect(_giveMoneyEffect);
	} catch (exception& e) {
		throw runtime_error("spec effect 'give-money'(" + to_string(_giveMoneyEffect) + ") not found in effects");
	}
	try {
		_getEffect(_serviceEffect);
	} catch (exception& e) {
		throw runtime_error("spec effect 'service'(" + to_string(_serviceEffect) + ") not found in effects");
	}

	// create mspi connection
	uint8_t buf[1024];
	cout << "[INFO][EXTBOARD] create MSPI connection.." << endl;
	_mspi = new Mspi(driver, speed, csPin, intPin, _int);
	
	try {
		// connect to extboard
		cout << "[INFO][EXTBOARD] connect to extboard.." << endl;
		_connect();

		cout << "[INFO][EXTBOARD] upload options.." << endl;
		_uploadAllOptions();
		cout << "[INFO][EXTBOARD] init external devices.." << endl;
		usleep(10000);		
	} catch (exception& e) {
		_onError(ErrorType::DISCONNECT_DEV, "EXTBOARD");
		throw runtime_error(e.what());
	}


	try {
		_initExtdev();
	} catch (exception& e) {
		_onError(ErrorType::DISCONNECT_DEV, e.what());
		throw runtime_error(e.what());
	}
	cout << "[INFO][EXTBOARD] start handler.." << endl;
	pthread_create(&_thread_id, NULL, _handler, NULL);
	_operations = fifo_create(32, sizeof(Handle));
	_isInit = true;
	_mspi->enableInt();	
}

/* Light control */
void startLightEffect(int id, int index) {
	Handle h;
	h.type = HandleType::START_EFFECT;
	h.data[0] = id;
	h.data[1] = index;
	resetLightEffect(index);
	fifo_put(&_operations, &h);
}

void startLightEffect(SpecEffect effect, int index) {
	int id = -1;
	switch (effect) {
	case SpecEffect::GIVE_MONEY_EFFECT: id = _giveMoneyEffect; break;
	case SpecEffect::SERVICE_EFFECT: id = _serviceEffect; break;
	}
	startLightEffect(id, index);
}

void resetLightEffect(int index) {
	Handle h;
	h.type = HandleType::RESET_EFFECT;
	h.data[0] = index;
	fifo_put(&_operations, &h);
}

/* Performing functions */
void relievePressure() {
	Handle h;
	h.type = HandleType::RELEIVE_PRESSURE;
	fifo_put(&_operations, &h);
}

void setRelayGroup(int id) {
	Handle h;
	try {
		_get_rgi(id);
	} catch (exception& e) {
		cout << "[WARNING][EXTBOARD] relay group " << id << " not found" << endl;
		return;
	}
	_currentRelayGroupId = id;
	h.type = HandleType::SET_RELAY_GROUP;
	h.data[0] = id;
	fifo_put(&_operations, &h);
}

void setRelaysState(int address, int states) {
	Handle h;
	h.type = HandleType::DIRECT_SET_RELAYS;
	h.data[0] = address;
	h.data[1] = states;
	fifo_put(&_operations, &h);
}

/* Other control functions */
void flap(bool state) {
	Handle h;
	h.type = HandleType::FLAP;
	h.data[0] = state ? 1 : 0;
	fifo_put(&_operations, &h);
}

/* Event handlers registration */
void registerOnButtonPushedHandler(void (*handler)(int iButton)) {
	_onButton = handler;
}

void registerOnCardReadHandler(void (*handler)(uint64_t cardid)) {
	_onCard = handler;
}

void registerOnMoneyAddedHandler(void (*handler)(double nMoney)) {
	_onMoney = handler;
}

void registerOnObjectCloserHandler(void (*handler)(bool state)) {
	_onCloser = handler;
}

void registerOnErrorHandler(void (*handler)(ErrorType et, string text)) {
	_onError = handler;
}

void up_cash() {
	char buf[256] = {0};
	double nmoney = 0;
	try {
		ifstream f("./.cash", ios_base::in);
		while (f.getline(buf, 256)) {
			try {
				nmoney += strtod(buf, NULL);
			} catch (exception& e) {
				cout << "[WARNING][EXTBOARD] fail to up cash line: " << e.what() << endl;
			}
			memset(buf, 0, 256);
		}			
	} catch (exception& e) {
		cout << "[WARNING][EXTBOARD] fail to up cash: " << e.what() << endl;
	}
	
	system("rm -f ./.cash");
	if (nmoney > 0) {
		if (_onMoney != nullptr) {
			_onMoney(nmoney);
		}
	}
}

}