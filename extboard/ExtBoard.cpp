#include "ExtBoard.h"
#include "../mspi-linux/Mspi.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

using namespace std;
using json = nlohmann::json;

namespace extboard {

namespace {

	enum class LightCmd {
		WRITE0_BM = 0x02,
		WRITE1_BM = 0x01,
		WRITE0_LP = 0x04,
		WRITE1_LP = 0x03,
		WRITE0_IL = 0x06,
		WRITE1_IL = 0x05,
		DELAY = 0x0A,
		NODELAY = 0x0B
	};
	
	enum class Addr {
		WIA = 0x00,
		VERSION = 0x03,
		LOR = 0x05,
		STATUS = 0x06,
		INTREASON = 0x07,
		RELAY_GROUPS = 0x10,
		DIRECT_SET = 0xD0,
		SET_GROUP = 0xD2,
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
		BUTTONS_INFO = 0x1060
	};

	enum class Cmd {
		RESET = 0x00,
		SAVE = 0x08,
		RELIEVE_PRESSURE = 0x10,
		OPEN_FLAP = 0x11,
		CLOSE_FLAP = 0x12,
		RESET_MAIN_EFFECT = 0x20,
		RESET_SUB_EFFECT = 0x21,
		APPLAY_RGROUP = 0x30
	};

	enum class IntReason {
		BOOTLOADER = 0x01,
		INIT = 0x02,
		ERROR = 0x03,
		BUTTON = 0x04,
		CARD = 0x05,
		MONEY = 0x06,
		OBJ_CLOSER = 0x07,
		OBJ_WASTE = 0x08,
		EFFECTEND = 0x09
	};

	struct LightInstruction {
		uint8_t cmd;
		uint8_t data;
	};

	struct LightEffect {
		int id;
		uint16_t delay;
		uint8_t nRepeats;
		uint8_t needReset;
		uint8_t nResetInstructions;
		uint16_t nTotalInstructions;
		LightInstruction instructions[508];
		uint8_t data[1024];
	};

	struct Payment {
		enum Mode {
			NOT_USE = 0x00,
			COUNTER = 0x01,
			BILL_CODE = 0x02
		};
		Mode mode;
		double rate;
		double billcodes[32];
	};

	struct PerformerUnit {
		uint8_t addr;
		uint8_t normalStates;
		uint8_t dependencies[6];

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
		uint8_t addr[4];
		uint8_t state[4];
		
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
		uint8_t addr;
		uint8_t activeLevels;
		uint8_t io;

		bool operator!=(const ButtonModule& rv) {
			if (addr != rv.addr) {
				return true;
			}
			if (activeLevels != rv.activeLevels) {
				return true;
			}
			if (usedMask != rv.usedMask) {
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
		uint8_t addr;
		uint8_t i;
		bool state;
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
	uint8_t _nReleiveInstructions;
	int _closerRange = -1;
	uint64_t _tempSens1Addr = 0;
	uint64_t _tempSens2Addr = 0;
	int _giveMoneyEffect = -1;
	int _serviceEffect = -1;

	void (*_onButton)(int id);
	void (*_onCard)(uint64_t id);
	void (*_onMoney)(double nMoney);
	void (*_onCloser)(bool state);

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
		vector<Led> leds(16);
		bool setf = false;
		bool resetf = false;
		json set;
		json reset;
		try {
			set = ins["set"];
			setf = true;
		} catch (exception& e) {}
		try {
			reset = ins["reset"];
			resetf = true;
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
			delay /= 5;
			if (delay > 4095) {
				delay = 4095;
			}
			LightInstruction li;
			li.cmd = 0x0A << 4;
			li.cmd |= (delay >> 8) & 0x0F;
			li.data = (uint8_t)delay;
			iv.push_back(li);
		} else {
			vector<Led> lps(8);
			vector<Led> ebs(8);
			vector<Led> lpr(8);
			vector<Led> ebr(8);
			vector<Led> bms(8);
			vector<Led> bmr(8);
			for (int i = 0; i < leds.size(); i++) {
				if (leds[i].type == Led::LEDPANEL) {
					if (leds[i].state) {
						lps.push_back(leds[i]);
					} else {
						lpr.push_back(leds[i]);
					}
				} else
				if (leds[i].type == Led::EXTBOARD) {
					if (leds[i].state) {
						ebs.push_back(leds[i]);
					} else {
						ebr.push_back(leds[i]);
					}
				} else
				if (leds[i].type == Led::BUTTONMODULE) {
					if (leds[i].state) {
						bms.push_back(leds[i]);
					} else {
						bmr.push_back(leds[i]);
					}
				} else {
					throw runtime_error("undefined led type");
				}
			}
			vector<LightInstruction> aiv(4);
			if (lps.size() > 0) {
				LightInstruction li;
				li.cmd = 0x03 << 4;
				int bits = 0;
				for (int i = 0; i < lps.size(); i++) {
					bits |= 1 << lps[i].i;
				}
				li.cmd |= (bits >> 8) & 0x0F; 
				li.data = (uint8_t)bits;
				aiv.push_back(li);
			}
			if (lpr.size() > 0) {
				LightInstruction li;
				li.cmd = 0x04 << 4;
				int bits = 0;
				for (int i = 0; i < lps.size(); i++) {
					bits |= 1 << lps[i].i;
				}
				li.cmd |= (bits >> 8) & 0x0F; 
				li.data = (uint8_t)bits;
				aiv.push_back(li);
			}
			if (ebs.size() > 0) {
				LightInstruction li;
				li.cmd = 0x05 << 4;
				int bits = 0;
				for (int i = 0; i < lps.size(); i++) {
					bits |= 1 << lps[i].i;
				}
				li.cmd |= (bits >> 8) & 0x0F; 
				li.data = (uint8_t)bits;
				aiv.push_back(li);
			}
			if (ebr.size() > 0) {
				LightInstruction li;
				li.cmd = 0x06 << 4;
				int bits = 0;
				for (int i = 0; i < lps.size(); i++) {
					bits |= 1 << lps[i].i;
				}
				li.cmd |= (bits >> 8) & 0x0F; 
				li.data = (uint8_t)bits;
				aiv.push_back(li);
			}

			vector<uint8_t> bmsi(4);
			for (int i = 0; i < bms.size(); i++) {
				uint8_t bmi = _get_bmi(bms[i].addr);
				if (!_is_contain(bmsi, bmi)) {
					bmsi.push_back(bmi);
				}
			}
			for (int i = 0; i < bmsi.size(); i++) {
				LightInstruction li;
				li.cmd = (0x01 << 4) | ((bmsi[i]) & 0x0F);
				li.data = 0;
				for (int j = 0; j < bms.size(); j++) {
					if (bms[j].addr == _buttonms[bmsi[i]].addr) {
						li.data |= 1 << bms[j].i;
					}
				}
				aiv.push_back(li);
			}
			vector<uint8_t> bmri(4);
			for (int i = 0; i < bmr.size(); i++) {
				uint8_t bmi = _get_bmi(bmr[i].addr);
				if (!_is_contain(bmri, bmi)) {
					bmri.push_back(bmi);
				}
			}
			for (int i = 0; i < bmri.size(); i++) {
				LightInstruction li;
				li.cmd = (0x01 << 4) | ((bmri[i]) & 0x0F);
				li.data = 0;
				for (int j = 0; j < bmr.size(); j++) {
					if (bmr[j].addr == _buttonms[bmri[i]].addr) {
						li.data |= 1 << bmr[j].i;
					}
				}
				aiv.push_back(li);
			}

			if (aiv.size() > 1) {
				if (aiv.size() > 32) {
					throw runtime_error("too many once time instructions");
				}
				LightInstruction li;
				li.cmd = 0x0B << 4;
				li.data = aiv.size();
				iv.push_back(li);
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
			for (int i = 0; i < bcs.size(); i++) {
				try {
					int bc = bcs[i][0];
					int bn = bcs[i][1];
					if (bc < 1 || bc > 31) {
						throw runtime_error("incorrect bill code");
					}
					dst.billcodes[bc] = bn;
				} catch (exception& e) {
					throw runtime_error("fail load '" + to_string(i) + "' bill code for " + name + ": " + string(e.what()));
				}
			}
		} else {
			throw runtime_error("unknown payment mode '" + mode + "' for cash");
		}
	}

	void _int(int reason) {
		if (reason == (int)IntReason::BUTTON && _onButton != nullptr) {

		} else
		if (reason == (int)IntReason::CARD && _onCard != nullptr) {

		} else
		if (reason == (int)IntReason::MONEY && _onMoney != nullptr) {
			
		} else
		if (reason == (int)IntReason::OBJ_CLOSER && _onCloser != nullptr) {
			_onCloser(true);
		} else
		if (reason == (int)IntReason::OBJ_WASTE && _onCloser != nullptr) {
			_onCloser(false);
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

		memset(buf, 0, 6);
		buf[0] = (uint8_t)_cash.mode;
		buf[1] = (uint8_t)_coin.mode;
		buf[2] = (uint8_t)_terminal.mode;
		_mspi->write((int)Addr::PAYMENT_OPT, buf, 3);

		memset(buf, 0, 26);
		buf[0] = _buttoneb.usedMask;
		buf[1] = _buttoneb.activeLevels;
		for (int i = 0; i < _buttonms.size(); i++) {
			buf[2 + i*3] = _buttonms[i].addr;
			buf[2 + i*3 + 1] = _buttonms[i].usedMask;
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

		_mspi->write((int)Addr::BM_IO_OPT, _buttonms_io, 8);

		_mspi->cmd((int)Cmd::SAVE, 0);
		try {
			_mspi->cmd((int)Cmd::RESET, 0);
		} catch (exception& e) {

		}
		usleep(100000);
	}
}

void init(json& extboard, json& performingUnits, json& relaysGroups, json& payment, json& buttons, json& rangeFinder, json& tempSens, json& leds, json& effects, json& releiveInstructions) {
	start:

	// get hw extboard config
	cout << "load hardware extboard config.." << endl;
	string driver = JParser::getf(extboard, "driver", "extboard");
	int speed = JParser::getf(extboard, "speed", "extboard");
	int csPin = JParser::getf(extboard, "cs-pin", "extboard");
	int intPin = JParser::getf(extboard, "int-pin", "extboard");

	// create mspi connection
	cout << "create MSPI connection.." << endl;
	_mspi = new Mspi(driver, speed, csPin, intPin, _int);
	uint8_t buf[1024];
	_mspi->read((int)Addr::WIA, buf, 3);
	_mspi->read((int)Addr::VERSION, &buf[3], 2);
	cout << "extboard connected, WIA_PRJ: " << (buf[0] << 8 | buf[1]) << ", WIA_SUB: " << buf[2] << ", VERSION: [" << buf[3] << ", " << buf[4] << "]" << endl;
	while (true) {
		_mspi->read((int)Addr::STATUS, buf, 1);
		if (buf[0] > 1) {
			break;
		}
	}

	cout << "load performing units config.." << endl;
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

	cout << "load relays groups.." << endl;
	for (int i = 0; i < relaysGroups.size(); i++) {
		try {
			RelaysGroup rg;
			rg.id = JParser::getf(relaysGroups[i], "id", "");
			json& ps = JParser::getf(relaysGroups[i], "performers", "");
			if (ps.size() > 4) {
				throw runtime_error("performers may be 4 or less");
			}
			for (int j = 0; j < ps.size(); j++) {
				try {
					rg.addr[j] = JParser::getf(ps[j], "address", "");
					rg.state[j] = JParser::getf(ps[j], "state", "");
				} catch (exception& e) {
					throw runtime_error("fail load '" + to_string(j) + "' performer: " + string(e.what()));
				}
			}
		} catch (exception& e) {
			throw runtime_error("fail load '" + to_string(i) + "' relay group: " + string(e.what()));
		}
	}
	
	cout << "load payment hardware config.." << endl;
	json& cash = JParser::getf(payment, "cash", "payment");
	_loadPayment(cash, _cash, "cash");
	json& coin = JParser::getf(payment, "coin", "payment");
	_loadPayment(coin, _coin, "coin");
	json& terminal = JParser::getf(payment, "terminal", "payment");
	_loadPayment(terminal, _terminal, "terminal");

	cout << "load buttons hardware config.." << endl;
	json& bseb = JParser::getf(buttons, "extboard", "buttons");
	_buttoneb.addr = 0;
	_buttoneb.activeLevels = JParser::getf(bseb, "active-levels", "buttons extborad");
	_buttoneb.usedMask = JParser::getf(bseb, "used-mask", "buttons extborad");
	json& bsms = JParser::getf(buttons, "modules", "buttons");
	if (bsms.size() > 8) {
		throw runtime_error("buttons modules must be 8 or less");
	}
	for (int i = 0; i < bsms.size(); i++) {
		try {
			ButtonModule bm;
			bm.addr = JParser::getf(bsms[i], "address", "");
			bm.activeLevels = JParser::getf(bsms[i], "active-levels", "");
			bm.usedMask = JParser::getf(bsms[i], "used-mask", "");
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

	cout << "load range-finder hardware config.." << endl;
	_closerRange = JParser::getf(rangeFinder, "closer-range", "range-finder");

	cout << "load temperature sensors hardware config.." << endl;
	_tempSens1Addr = JParser::getf(tempSens[0], "address", "temp-sens '0'");
	_tempSens2Addr = JParser::getf(tempSens[1], "address", "temp-sens '1'");

	cout << "load releive instructions.." << endl;
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
				_releiveInstructions[i*2] = 0;
				_releiveInstructions[i*2 + 1] = delay / 100;
			}
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' releive instruction: " + string(e.what()));
		}
	}

	cout << "load leds.." << endl;
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

	cout << "load effects instructions.." << endl;
	for (int i = 0; i < effects.size(); i++) {
		try {
			LightEffect ef;
			ef.id = JParser::getf(effects[i], "id", "");
			ef.delay = JParser::getf(effects[i], "delay", "");
			ef.nRepeats = JParser::getf(effects[i], "repeats", "");
			json& ins = JParser::getf(effects[i], "instructions", "");
			vector<LightInstruction> iv(512);
			vector<LightInstruction> rstiv(256);
			for (int j = 0; j < ins.size(); j++) {
				_appendLightIns(iv, ins[j]);
			}
			bool needRst = JParser::getf(effects[i], "need-reset", "");
			if (needRst) {
				ef.needReset = 1;
				json& rstIns = JParser::getf(effects[i], "reset-instructions", "");
				for (int j = 0; j < rstIns.size(); j++) {
					_appendLightIns(rstiv, rstIns[j]);
				}
				if (rstiv.size() > 255) {
					throw runtime_error("too many reset-instructions");
				}
				ef.nResetInstructions = rstiv.size();
			} else {
				ef.needReset = 0;
				ef.nResetInstructions = 0;
			}
			if (iv.size() + rstiv.size() > 508) {
				throw runtime_error("too many total instructions");
			}
			for (int j = 0; j < rstiv.size(); j++) {
				ef.instructions[j] = rstiv[j];
			}
			for (int j = 0; j < rstiv.size(); j++) {
				ef.instructions[j + rstiv.size()] = iv[j];
			}
			ef.nTotalInstructions = iv.size() + rstiv.size();
			memset(ef.data, 0, 1024);
			ef.data[1] = (uint8_t)(ef.delay >> 8);
			ef.data[2] = (uint8_t)(ef.delay);
			ef.data[3] = ef.nRepeats;
			ef.data[4] = ef.needReset;
			ef.data[5] = ef.nResetInstructions;
			for (int j = 0; j < ef.nTotalInstructions; j++) {
				ef.data[8 + j*2] = ef.instructions[j].cmd;
				ef.data[8 + j*2 + 1] = ef.instructions[j].data;
			}
			_effects.push_back(ef);
			cout << "effect '" << ef.id << "' loaded: " << "nTI - " << ef.nTotalInstructions << " nRI - " << ef.nResetInstructions;
			for (int j = 0; j < ef.nTotalInstructions; j++) {
				printf(" |%X %X| ", ef.instructions[j].cmd, ef.instructions[j].data);
			}
			cout << endl;
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' effect: " + string(e.what()));
		}
	}

	cout << "get extboard options.." << endl;
	vector<PerformerUnit> ebpus(4);
	vector<RelaysGroup> ebrgs(24);
	Payment ebpmcash;
	Payment ebpmcoin;
	Payment ebpmterm;
	vector<ButtonModule> ebbms(8);
	ButtonModule ebbs;
	uint8_t ebclr;
	uint64_t ebts1a;
	uint64_t ebts2a;
	uint8_t rvis[128] = {0};
	
	_mspi->read((int)Addr::PERFORMERS_OPT, buf, 32);
	for (int i = 0; i < 4; i++) {
		ebpus[i].addr = buf[i*8];
		ebpus[i].normalStates = buf[i*8 + 1];
		ebpus[i].dependencies[0] = buf[i*8 + 2];
		ebpus[i].dependencies[1] = buf[i*8 + 3];
		ebpus[i].dependencies[2] = buf[i*8 + 4];
		ebpus[i].dependencies[3] = buf[i*8 + 5];
		ebpus[i].dependencies[4] = buf[i*8 + 6];
		ebpus[i].dependencies[5] = buf[i*8 + 7];
	}

	_mspi->read((int)Addr::RELAY_GROUPS, buf, 192);
	for (int i = 0; i < 24; i++) {
		for (int j = 0; j < 4; j++) {
			ebrgs[i].addr[j] = buf[i*8 + j*2];
			ebrgs[i].state[j] = buf[i*8 + j*2  +1];
		}
	}

	_mspi->read((int)Addr::PAYMENT_OPT, buf, 3);
	ebpmcash.mode = (Payment::Mode)buf[0];
	ebpmcoin.mode = (Payment::Mode)buf[1];
	ebpmterm.mode = (Payment::Mode)buf[2];
	
	_mspi->read((int)Addr::BUTTONS_OPT, buf, 26);
	ebbs.addr = 0;
	ebbs.usedMask = buf[0];
	ebbs.activeLevels = buf[1];
	for (int i = 0; i < 8; i++) {
		ebbms[i].addr = buf[2 + i*3];
		ebbms[i].usedMask = buf[2 + i*3 + 1];
		ebbms[i].activeLevels = buf[2 + i*3 + 2];
	}

	_mspi->read((int)Addr::RANGEFINDER_OPT, buf, 1);
	ebclr = buf[0];

	_mspi->read((int)Addr::TEMPSENS_OPT, buf, 16);
	ebts1a = _collectn(buf, 8);
	ebts2a = _collectn(&buf[8], 8);

	_mspi->read((int)Addr::RELEIVE_INS, rvis, 128);

	cout << "compare extboard options.." << endl;
	for (int i = 0; i < _performersu.size(); i++) {
		if (_performersu[i] != ebpus[i]) {
			cout << "find differences in performer units options" << endl;
			_uploadAllOptions();
			goto start;
		}
	}
	for (int i = _performersu.size(); i < 4; i++) {
		if (ebpus[i].addr != 0) {
			cout << "find differences in performer units options" << endl;
			_uploadAllOptions();
			goto start;
		}
	}

	for (int i = 0; i < _rgroups.size(); i++) {
		if (_rgroups[i] != ebrgs[i]) {
			cout << "find differences in relays groups options" << endl;
			_uploadAllOptions();
			goto start;
		}
	}
	if (ebpmcash.mode != _cash.mode || ebpmcoin.mode != _coin.mode || ebpmterm.mode != _terminal.mode) {
			cout << "find differences in payment options" << endl;
			_uploadAllOptions();
			goto start;
	}

	if (_buttoneb != ebbs) {
		cout << "find differences in buttons options" << endl;
		_uploadAllOptions();
		goto start;
	}
	for (int i = 0; i < _buttonms.size(); i++) {
		if (_buttonms[i] != ebbms[i]) {
			cout << "find differences in buttons options" << endl;
			_uploadAllOptions();
			goto start;
		}
	}
	for (int i = _buttonms.size(); i < 8; i++) {
		if (ebbms[i].addr != 0) {
			cout << "find differences in buttons options" << endl;
			_uploadAllOptions();
			goto start;
		}
	}

	if (_closerRange != ebclr) {
		cout << "find differences in range finder options" << endl;
		_uploadAllOptions();
		goto start;
	}

	if (_tempSens1Addr != ebts1a || _tempSens2Addr != ebts2a) {
		cout << "find differences in temp sens options" << endl;
		_uploadAllOptions();
		goto start;
	}

	if (memcmp(_releiveInstructions, rvis, 128) != 0) {
		cout << "find differences in releive pressure instructions" << endl;
		_uploadAllOptions();
		goto start;
	}

	cout << "compare success - no differences" << endl;
}

/* Light control */
void startLightEffect(int id, LightEffectType type) {
	LightEffect* ef = _getEffect(id);
	if (type == LightEffectType::MAIN) {
		_mspi->cmd((int)Cmd::RESET_MAIN_EFFECT, 0);
		ef->data[0] = 0x01;
	} else 
	if (type == LightEffectType::SUB) {
		_mspi->cmd((int)Cmd::RESET_SUB_EFFECT, 0);
		ef->data[0] = 0x02;
	} else {
		throw runtime_error("undefined LightEffectType");
	}
	_mspi->write((int)Addr::EFFECT, ef->data, 8 + ef->nTotalInstructions*2);
}

void startLightEffect(SpecEffect effect, LightEffectType type) {
	int id = -1;
	switch (effect) {
	case SpecEffect::GIVE_MONEY_EFFECT: id = _giveMoneyEffect; break;
	case SpecEffect::SERVICE_EFFECT: id = _serviceEffect; break;
	}
	startLightEffect(id, type);
}

void resetLightEffect(LightEffectType type) {
	if (type == LightEffectType::MAIN) {
		_mspi->cmd((int)Cmd::RESET_MAIN_EFFECT, 0);
	} else 
	if (type == LightEffectType::SUB) {
		_mspi->cmd((int)Cmd::RESET_SUB_EFFECT, 0);
	} else {
		throw runtime_error("undefined LightEffectType");
	}
}

/* Performing functions */
void relievePressure() {
	_mspi->cmd((int)Cmd::RELIEVE_PRESSURE, 0);
}

void setRelayGroup(int id) {
	_mspi->cmd((int)Cmd::APPLAY_RGROUP, _get_rgi(id));
}

void setRelaysState(int address, int states) {
	uint8_t buf[2] = {(uint8_t)address, (uint8_t)states};
	_mspi->write((int)Addr::DIRECT_SET, buf, 2);
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

}