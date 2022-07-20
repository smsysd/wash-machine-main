#include "Perf.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../general-tools/general_tools.h"
#include "../mb-ascii-linux/MbAsciiMaster.h"

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

namespace perf {

namespace {
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

	enum class ReleiveInsType {
		DELAY,
		SET
	};

	struct ReleiveIns {
		ReleiveInsType type;
		int addr;
		int state;
		int delay;
	};

	enum class HandleType {
		DIRECT_SET_RELAYS,	// data: addr, states
		DELAY
	};

	struct Handle {
		HandleType type;
		uint8_t data[8];
	};

	MbAsciiMaster* _mb = nullptr;
	vector<PerformerUnit> _performersu;
	vector<RelaysGroup> _rgroups;
	vector<ReleiveIns> _releiveIns;
	pthread_t _thread_id;
	Fifo _operations;
	int _currentRelayGroupId = -1;
	mutex _mutex;
	mutex _fifomutex;
	int _maxHandleErrors = 5;
	Logger* _log;
	bool _isInit;

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

	void* _handler(void* arg) {
		int buf[256];
		int suspicion = 0;
		int borehole = 0;
		while (true) {
			_mutex.lock();
			try {
				Handle* h = (Handle*)fifo_get(&_operations);
				if (h != nullptr) {
					cout << "[INFO][PERF] handling " << (int)h->type << ", total fifo size " << _operations.nElements << endl;
				}
				if (h == nullptr) {
					usleep(10000);
				} else
				if (h->type == HandleType::DIRECT_SET_RELAYS) {
					buf[0] = h->data[1];
					_mb->rwrite(h->data[0], 0x13, buf, 1, 500);

					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					suspicion = 0;
				} else
				if (h->type == HandleType::DELAY) {
					usleep((h->data[0] << 8) + h->data[1]);

					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
				} else {
					_fifomutex.lock();
					fifo_pop(&_operations);
					_fifomutex.unlock();
					_log->log(Logger::Type::WARNING, "EXTBOARD", "undefined handle: " + to_string((int)h->type), 4);
				}
			} catch (exception& e) {
				_log->log(Logger::Type::WARNING, "PERF", "fail handle: " + string(e.what()), 4);
				suspicion++;
				usleep(200000);
			}
			_mutex.unlock();
			if (suspicion > _maxHandleErrors) {
				suspicion = 0;
				_isInit = false;
				_log->log(Logger::Type::WARNING, "EXTBOARD", "extboard was not responding at commands - reinit..", 3);
				// _onError(ErrorType::DISCONNECT_DEV, "EXTBOARD");
			}
			if (borehole % 1000 == 0) {
				// add repetive handles
				if (_currentRelayGroupId >= 0) {
					setRelayGroup(_currentRelayGroupId);
				}
			}
			usleep(20000);
			borehole++;
		}
	}
}

void init(json& performingGen, json& performingUnits, json& relaysGroups, json& releiveInstructions, Logger* log) {
	_log = log;

	string driver = JParser::getf(performingGen, "driver", "performing-gen");
	_maxHandleErrors = JParser::getf(performingGen, "max-handle-errors", "performing-gen");

	cout << "[INFO][PERF] load releive instructions.." << endl;
	for (int i = 0; i < releiveInstructions.size(); i++) {
		try {
			ReleiveIns rins;
			json& ins = releiveInstructions[i];
			try {
				json& set = ins["set"];
				uint8_t addr = set[0];
				uint8_t states = set[1];	
				rins.type = ReleiveInsType::SET;
				rins.state = states;
				rins.addr = addr;
			} catch (exception& e) {
				int delay = ins["delay"];
				rins.delay = delay;
				rins.type = ReleiveInsType::DELAY;
			}
			_releiveIns.push_back(rins);
		} catch (exception& e) {
			throw runtime_error("fail to load '" + to_string(i) + "' releive instruction: " + string(e.what()));
		}
	}

	cout << "[INFO][PERF] load performing units config.." << endl;
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
			_log->log(Logger::Type::WARNING, "PERF", "address '" + to_string(i) + "' of performer unit was repeated", 3);
		}
	}

	cout << "[INFO][PERF] load relays groups.." << endl;
	for (int i = 0; i < relaysGroups.size(); i++) {
		try {
			RelaysGroup rg = {0};
			rg.id = JParser::getf(relaysGroups[i], "id", "relay-group");
			json& ps = JParser::getf(relaysGroups[i], "performers", "relay-group");
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

	cout << "[INFO][PERF] init performers.." << endl;
	_mb = new MbAsciiMaster(-1, driver.c_str(), B9600);


	for (int i = 0; i < _performersu.size(); i++) {
		while (true) {
			try {
				_mb->rwrite(_performersu[i].addr, 0x20, &_performersu[i].normalStates, 1, 1000);
				_mb->rwrite(_performersu[i].addr, 0x21, _performersu[i].dependencies, 6, 1000);
			} catch (exception& e) {
				_log->log(Logger::Type::ERROR, "PERF", "fail to init performer at addr " + to_string(_performersu[i].addr), 1);
				sleep(5);
			}
 		}
	}

	cout << "[INFO][PERF] start handler.." << endl;
	pthread_create(&_thread_id, NULL, _handler, NULL);
	_operations = fifo_create(32, sizeof(Handle));
	_isInit = true;
}

bool getState() {
	return _isInit;
}

/* Performing functions */
void relievePressure() {
	Handle h;
	
	for (int i = 0; i < _releiveIns.size(); i++) {
		if (_releiveIns[i].type == ReleiveInsType::SET) {
			setRelaysState(_releiveIns[i].addr, _releiveIns[i].state);			
		} else
		if (_releiveIns[i].type == ReleiveInsType::DELAY) {
			h.type = HandleType::DELAY;
			h.data[0] = (uint8_t)(_releiveIns[i].delay >> 8);
			h.data[1] = (uint8_t)_releiveIns[i].delay;
			_fifomutex.lock();
			fifo_put(&_operations, &h);
			_fifomutex.unlock();
		}
	}
}

void setRelayGroup(int id) {
	Handle h;
	int rgi;
	try {
		rgi = _get_rgi(id);
	} catch (exception& e) {
		_log->log(Logger::Type::WARNING, "PERF", "relay group " + to_string(id) + " not found", 5);
		return;
	}
	_currentRelayGroupId = id;
	for (int i = 0; i < 4; i++) {
		if (_rgroups[rgi].addr[i] > 0) {
			setRelaysState(_rgroups[rgi].addr[i], _rgroups[rgi].state[i]);
		}
	}
}

void setRelaysState(int address, int states) {
	Handle h;
	h.type = HandleType::DIRECT_SET_RELAYS;
	h.data[0] = address;
	h.data[1] = states;
	_fifomutex.lock();
	fifo_put(&_operations, &h);
	_fifomutex.unlock();
}

}