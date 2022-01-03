#include "ExtBoard.h"
#include "../mspi-linux/Mspi.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <time.h>

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
		COUNTERS = 0x100,
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
		RESET_SUB_EFFECT = 0x21
	};

	struct LightInstruction{
		uint8_t cmd;
		uint8_t data;
	};

	struct LightEffect {
		int id;
		uint16_t delay;
		uint8_t nRepeats;
		uint8_t needReset;
		uint8_t nResetInstructions;
		LightInstruction instructions[508];
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
		uint8_t usedMask;

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

	Mspi* _mspi = nullptr;
	vector<LightEffect> _effects;
	vector<PerformerUnit> _performersu;
	vector<RelaysGroup> _rgroups;
	vector<ButtonModule> _buttonms;
	ButtonModule _buttoneb;
	uint8_t _releiveInstructions[128];
	uint8_t _nReleiveInstructions;
	int _closerRange = -1;
	uint64_t _tempSens1Addr = 0;
	uint64_t _tempSens2Addr = 0;
	int _giveMoneyEffect = -1;
	int _serviceEffect = -1;

	uint64_t _collectn(uint8_t* src, int nb) {
		uint64_t val = 0;
		for (int i = 0; i < nb; i++) {
			val |= src[i] << ((nb - i - 1) * 8);
		}
	}

	void _distribn(uint8_t* dst, uint64_t val, int nb) {
		for (int i = 0; i < nb; i++) {
			dst[i] = (uint8_t) (val >> ((nb - i -1) * 8));
		}
	}

	void _int(int reason) {

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

		_mspi->cmd((int)Cmd::SAVE, 0);
		try {
			_mspi->cmd((int)Cmd::RESET, 0);
		} catch (exception& e) {

		}
		usleep(100000);
	}
}

void init(json& extboard, json& performingUnits, json& relaysGroups, json& buttons, json& rangeFinder, json& tempSens, json& leds, json& effects, json& releiveInstructions) {
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

	cout << "load effects instructions.." << endl;

	cout << "get extboard options.." << endl;
	vector<PerformerUnit> ebpus(4);
	vector<RelaysGroup> ebrgs(24);
	vector<ButtonModule> ebbms(8);
	ButtonModule ebbs;
	uint8_t ebclr;
	uint64_t ebts1a;
	uint64_t ebts2a;
	
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

	cout << "compare success - no differences" << endl;
}

/* Light control */
void startLightEffect(int id, LightEffectType type) {

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

}

/* Performing functions */
void relievePressure() {

}

void setRelayGroup(int iGroup) {

}

void setRelaysState(int address, int states) {

}

/* Event handlers registration */
void registerOnButtonPushedHandler(void (*handler)(int iButton)) {

}

void registerOnEffectEndHandler(void (*handler)(int iEffect)) {

}

void registerOnCardReadHandler(void (*handler)(uint64_t cardid)) {

}

void registerOnCashAddedHandler(void (*handler)(double nCash)) {

}

void registerOnObjectCloserHandler(void (*handler)()) {

}

}