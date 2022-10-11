#include "ExtBoard.h"
#include "../linux-ecbm/ecbm.h"
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
#include <algorithm>

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
		WORK = 0x06,
		READY_TO_RESET = 0x09
	};

	enum class LightCmd {
		WRITE_BM = 0x01,
		WRITE_LP = 0x03,
		WRITE_IL = 0x05,
		DELAY = 0x07,
		END = 0x00,
		NODELAY_BIT = 0x80
	};
	
	enum class Sig {
		PAYMENT = 100,
		MONEY_CTRL = 102,
		PAYMENT_CTRL = 103,
		BUTTONS = 110,
		BUTTONS_OPT = 111,
		BUTTONS_INF = 112,
		RFID_CARD = 120,
		FLAP = 130,
		EFFECT = 140,
		EFFECT_RST = 141
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
		FLAP,				// data: state
		MONEY_CTRL,			// data: drop or restore money (0 - drop, 1 - restore)
		PAYMENT_CTRL		// data: 0 - disable paymentm, 1 - enable
	};

	struct Handle {
		HandleType type;
		uint8_t data[8];
	};

	vector<Led> _leds;
	vector<LightEffect> _effects;
	int _buttoneb_alvl;
	Payment _cash;
	Payment _coin;
	Payment _terminal;
	int _giveMoneyEffect = -1;
	int _serviceEffect = -1;
	int _waitEffect = -1;
	bool _isInit = false;
	pthread_t _thread_id;
	Fifo _operations;
	int _nrstPin = -1;
	mutex _mutex;
	mutex _fifomutex;
	int _maxHandleErrors = 5;
	int _maxReinitTries = 3;
	int _card_read_timeout_ms = 1000;
	timespec _tlcard_read = {0};
	Logger* _log;
	bool _first_init = true;
	bool _payment_state = false;
	void (*_moneyrestcplt)();

	void (*_onButton)(int iButton) = nullptr;
	void (*_onCard)(uint64_t id) = nullptr;
	void (*_onMoney)(Pay pay) = nullptr;

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

	template<typename T>
	bool _is_contain(vector<T> vec, T val) {
		for (int i = 0; i < vec.size(); i++) {
			if (vec[i] == val) {
				return true;
			}
		}

		return false;
	}
	
	void _timespec_diff(const struct timespec* start, const struct timespec* stop, struct timespec* result) {
	    if ((stop->tv_nsec - start->tv_nsec) < 0) {
	        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
	        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	    } else {
	        result->tv_sec = stop->tv_sec - start->tv_sec;
	        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	    }
	}

	int _timespec2ms(const struct timespec* ts) {
		int ms = ts->tv_sec*1000;
		ms += ts->tv_nsec / 1000000;
		return ms;
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
					_log->log(Logger::Type::WARNING, "EXTBOARD", "undefined led type", 7);
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

			vector<int> bm_addrs;
			for (int i = 0; i < bm.size(); i++) {
				if (!_is_contain(bm_addrs, bm[i].addr)) {
					bm_addrs.push_back(bm[i].addr);
				}
			}
			sort(bm_addrs.begin(), bm_addrs.end());
			// sort leds by bm addr
			vector<vector<Led>> bms(32);
			for (int i = 0; i < bm.size(); i++) {
				int bmi = -1;
				for (int j = 0; j < bm_addrs.size(); j++) {
					if (bm_addrs[j] == bm[i].addr) {
						bmi = j;
						break;
					}
				}
				if (bmi >= 0) {
					bms[bmi].push_back(bm[i]);
				}
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
					li.data[0] = bm_addrs[i];
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
				_log->log(Logger::Type::ERROR, "EXTBOARD", "received " + to_string(val) + " bill code number, it is overflow, set max bill code: " + to_string(pm.maxbillcode ), 2);
				return pm.billcodes[pm.maxbillcode];
			}
			if (!pm.usedbillcodes[val]) {
				int nextbc = pm.maxbillcode;
				for (int i = val; i < 32; i++) {
					if (pm.usedbillcodes[i]) {
						nextbc = i;
					}
				}
				_log->log(Logger::Type::WARNING, "EXTBOARD", "received " + to_string(val) + " bill code number, it is not used, set next bill code: " + to_string(nextbc), 2);
				return pm.billcodes[nextbc];
			}
			return pm.billcodes[val];
		} else {
			return 0;
		}
	}

	vector<Pay> _injectMoney(uint8_t* buf) {
		uint16_t ncash = _collectn(buf, 2);
		uint16_t nterm = _collectn(&buf[2], 2);
		uint16_t ncoin = _collectn(&buf[4], 2);
		vector<Pay> pays;
		
		if (ncash > 0) {
			Pay pay;
			pay.type = Payment::Type::CASH;
			pay.count = _injectMoney(_cash, ncash);
			pays.push_back(pay);
		}
		if (nterm > 0) {
			Pay pay;
			pay.type = Payment::Type::TERM;
			pay.count = _injectMoney(_terminal, nterm);
			pays.push_back(pay);
		}
		if (ncoin > 0) {
			Pay pay;
			pay.type = Payment::Type::COIN;
			pay.count = _injectMoney(_coin, ncoin);
			pays.push_back(pay);
		}
		return pays;
	}

	void _connect () {
		uint8_t buf[8];
		int tries = 0;
		int timeout;

		EcbmInfo info = {0};
		int rc;
		begin:
		rc = ecbm_info(0, 1, &info);
		if (rc == ECBM_RC_OK) {
			cout << "[INFO][EXTBOARD] connected, WIA: " << info.wia << ", VER: " << info.version << endl << "\tDESCR: " << info.descr << endl << "\tstatus: " << info.status << " - " << info.status_descr << endl;
		} else {
			tries++;
			if (tries > 1) {
				throw runtime_error("fail to connect to extboard: fail read info: " + to_string(rc));
			} else {
				digitalWrite(_nrstPin, 0);
				usleep(10000);
				digitalWrite(_nrstPin, 1);
				sleep(1);
				goto begin;
			}
		}

		_first_init = true;
	}

	void* _handler(void* arg) {
		uint8_t buf[4096];
		int suspicion = 0;
		int borehole = 0;
		bool hpos;
		int tries;
		while (true) {
			_mutex.lock();
			tries = 0;
			while (!_isInit) {
				try {
					hpos = false;
					_connect();
					hpos = true;
					usleep(50000);
					_isInit = true;
					suspicion = 0;
				} catch (exception& e) {
					tries++;
					digitalWrite(_nrstPin, 0);
					usleep(10000);
					digitalWrite(_nrstPin, 1);
					sleep(2);
					_log->log(Logger::Type::ERROR, "EXTBOARD", "fail to reinit: " + string(e.what()), 2);
				}
			}
			try {
				Handle* h = (Handle*)fifo_get(&_operations);
				if (h != nullptr) {
					// cout << "[INFO][EXTBOARD] handling " << (int)h->type << ", total fifo size " << _operations.nElements << endl;
				}
				if (h == nullptr) {
					// Poll for event
					int event = 0;
					int rc = ecbm_get_event(0, 1, &event);
					if (rc == ECBM_RC_OK) {
						if (event == (int)Sig::PAYMENT) {
							rc = ecbm_read(0, 1, event, buf, sizeof(buf));
							if (rc == 6) {
								uint8_t moneybuf[6] = {0};
								memcpy(moneybuf, buf, 6);
								memset(buf, 0, 6);
								rc = ecbm_write(0, 1, (int)Sig::PAYMENT, buf, 6);
								if (rc == ECBM_RC_OK) {
									vector<Pay> pays = _injectMoney(moneybuf);
									cout << "[INFO][EXTBOARD] moeny received and cleared: " << pays.size() << endl;
									for (int i = 0; i < pays.size(); i++) {
										if (_onMoney != nullptr) {
											_onMoney(pays[i]);
										}
									}
								}
							}
						} else
						if (event == (int)Sig::BUTTONS) {
							rc = ecbm_read(0, 1, event, buf, sizeof(buf));
							if (rc == 9) {
								for (int i = 0; i < 8; i++) {
									if (buf[0] & (1 << i)) {
										if (_onButton != nullptr) {
											_onButton(i);
										}
									}
								}
								for (int i = 0; i < 8; i++) {
									for (int j = 0; j < 8; j++) {
										if (buf[i+1] & (1 << j)) {
											_onButton(8+i*8+j);
										}
									}
								}
							}
						} else
						if (event == (int)Sig::BUTTONS_INF) {
							rc = ecbm_read(0, 1, event, buf, sizeof(buf));
							if (rc > 0) {
								
							}
						} else
						if (event == (int)Sig::RFID_CARD) {
							rc = ecbm_read(0, 1, event, buf, sizeof(buf));
							if (rc >= 4) {
								timespec now = {0};
								timespec res = {0};
								clock_gettime(CLOCK_MONOTONIC, &now);
								_timespec_diff(&_tlcard_read, &now, &res);
								if (_timespec2ms(&res) > _card_read_timeout_ms) {
									uint64_t cid = _collectn(buf, rc);
									cid &= 0xFFFFFFFF;
									printf("[INFO][EXTBOARD] card read: %X\n", cid);
									clock_gettime(CLOCK_MONOTONIC, &_tlcard_read);
									if (_onCard != nullptr) {
										_onCard(cid);
									}
									clock_gettime(CLOCK_MONOTONIC, &_tlcard_read);
								}
							}
						} else {
							if (event > 0) {
								cout << "[INFO][EXTBOARD] read unknown event " << event << endl;
							}
							rc = ECBM_RC_OK;
						}
						if (rc < 0) {
							throw runtime_error("fail to get event data: " + to_string(rc));
						}
					} else {
						throw runtime_error("fail to get event: " + to_string(rc));
					}
					usleep(10000);
				} else
				if (h->type == HandleType::RESET_EFFECT) {
					ecbm_writew(0, 1, (int)Sig::EFFECT_RST, h->data, 1, 5000);
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					suspicion = 0;
				} else
				if (h->type == HandleType::FLAP) {
					ecbm_write(0, 1, (int)Sig::FLAP, h->data, 1);
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					suspicion = 0;
				} else
				if (h->type == HandleType::START_EFFECT) {
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					try {
						LightEffect* ef = _getEffect(h->data[0]);
						ef->data[0] = h->data[1];
						ecbm_write(0, 1, (int)Sig::EFFECT, ef->data, 8 + ef->nTotalInstructions*5);
						suspicion = 0;
					} catch (exception& e) {
						_log->log(Logger::Type::ERROR, "EXTBOARD", "fail to start effect: " + string(e.what()), 4);
					}
				} else
				if (h->type == HandleType::MONEY_CTRL) {
					if (h->data[0]) {
						ecbm_write(0, 1, (int)Sig::MONEY_CTRL, h->data, 1);
						_moneyrestcplt();
					} else {
						ecbm_write(0, 1, (int)Sig::MONEY_CTRL, h->data, 1);
					}
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					suspicion = 0;
				} else
				if (h->type == HandleType::PAYMENT_CTRL) {
					ecbm_write(0, 1, (int)Sig::PAYMENT_CTRL, h->data, 1);
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					suspicion = 0;
				} else {
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					_log->log(Logger::Type::WARNING, "EXTBOARD", "undefined handle: " + to_string((int)h->type), 4);
				}
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "EXTBOARD", "fail handle: " + string(e.what()), 4);
				suspicion++;
				usleep(200000);
			}
			_mutex.unlock();
			if (suspicion > _maxHandleErrors) {
				suspicion = 0;
				_isInit = false;
				_log->log(Logger::Type::WARNING, "EXTBOARD", "extboard was not responding at commands - reinit..", 3);
			} else {
				_isInit = true;
			}
			if (borehole % 100 == 0) {
				// add repetive handles
				if (_payment_state) {
					payment_ctl(true);
				}
			}
			usleep(20000);
			borehole++;
		}
	}
}

void init(json& extboard, json& payment, json& leds, json& effects, json& specEffects, Logger* log) {
	_log = log;
	// get hw extboard config
	cout << "[INFO][EXTBOARD] load hardware extboard config.." << endl;
	string driver = JParser::getf(extboard, "driver", "extboard");
	_nrstPin = JParser::getf(extboard, "nrst-pin", "extboard");
	try {
		_card_read_timeout_ms = JParser::getf(extboard, "card_read_timeout_ms", "extboard");
	} catch (exception& e) {
		_card_read_timeout_ms = 1000;
	}
	try {
		_maxHandleErrors = JParser::getf(extboard, "max-handle-error", "extboard");
	} catch (exception& e) {

	}
	try {
		_maxReinitTries = 5;
	} catch (exception& e) {
		_maxReinitTries = 3;
	}
	
	cout << "[INFO][EXTBOARD] load payment hardware config.." << endl;
	json& cash = JParser::getf(payment, "cash", "payment");
	_loadPayment(cash, _cash, "cash");
	json& coin = JParser::getf(payment, "coin", "payment");
	_loadPayment(coin, _coin, "coin");
	json& terminal = JParser::getf(payment, "terminal", "payment");
	_loadPayment(terminal, _terminal, "terminal");

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
	_waitEffect = JParser::getf(specEffects, "wait", "spec-effects");

	cout << "[INFO][EXTBOARD] assert spec-effects.." << endl;
	try {
		_getEffect(_giveMoneyEffect);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "spec effect 'give-money' (" + to_string(_giveMoneyEffect) + ") not found in effects");
	}
	try {
		_getEffect(_serviceEffect);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "spec effect 'service' (" + to_string(_serviceEffect) + ") not found in effects");
	}
	try {
		_getEffect(_waitEffect);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "EXTBOARD", "spec effect 'wait' (" + to_string(_waitEffect) + ") not found in effects");
	}

	digitalWrite(_nrstPin, 1);
	pinMode(_nrstPin, OUTPUT);
	digitalWrite(_nrstPin, 1);
	usleep(1000);

	uint8_t buf[1024];
	cout << "[INFO][EXTBOARD] init ecbm.." << endl;
	int rc = ecbm_init(0, ECBM_SPEED_FAST, driver.c_str());
	if (rc != ECBM_RC_OK) {
		throw runtime_error("fail to init ecbm: " + to_string(rc));
	}

	try {
		// connect to extboard
		cout << "[INFO][EXTBOARD] connect to extboard.." << endl;
		_connect();
	} catch (exception& e) {
		throw runtime_error(e.what());
	}

	cout << "[INFO][EXTBOARD] start handler.." << endl;
	pthread_create(&_thread_id, NULL, _handler, NULL);
	_operations = fifo_create(32, sizeof(Handle));
	_isInit = true;
}

bool getState() {
	return _isInit;
}

/* Light control */
void startLightEffect(int id, int index) {
	Handle h;
	h.type = HandleType::START_EFFECT;
	h.data[0] = id;
	h.data[1] = index;
	resetLightEffect(index);
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

void startLightEffect(SpecEffect effect, int index) {
	int id = -1;
	switch (effect) {
	case SpecEffect::GIVE_MONEY_EFFECT: id = _giveMoneyEffect; break;
	case SpecEffect::SERVICE_EFFECT: id = _serviceEffect; break;
	case SpecEffect::WAIT: id = _waitEffect; break;
	}
	startLightEffect(id, index);
}

void resetLightEffect(int index) {
	Handle h;
	h.type = HandleType::RESET_EFFECT;
	h.data[0] = index;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

/* Other control functions */
void flap(bool state) {
	Handle h;
	h.type = HandleType::FLAP;
	h.data[0] = state ? 1 : 0;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

void payment_ctl(bool state) {
	_payment_state = state;
	Handle h;
	h.type = HandleType::PAYMENT_CTRL;
	h.data[0] = state ? 1 : 0;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

/* Event handlers registration */
void registerOnButtonPushedHandler(void (*handler)(int iButton)) {
	_onButton = handler;
}

void registerOnCardReadHandler(void (*handler)(uint64_t cardid)) {
	_onCard = handler;
}

void registerOnMoneyAddedHandler(void (*handler)(Pay pay)) {
	_onMoney = handler;
}

void restoreMoney(void (*cplt)()) {
	_moneyrestcplt = cplt; 
	Handle h;
	h.type = HandleType::MONEY_CTRL;
	h.data[0] = 1;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

void dropMoney() {
	Handle h;
	h.type = HandleType::MONEY_CTRL;
	h.data[0] = 0;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

}