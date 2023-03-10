/* Author: Jodzik */
#ifndef PERF_H_
#define PERF_H_

#include "../json.h"
#include "../logger-linux/Logger.h"

#include <vector>
#include <string>
#include <stdint.h>

using namespace std;
using json = nlohmann::json;

namespace perf {

	void init(json& performingGen, json& performingUnits, json& relaysGroups, json& releiveInstructions, Logger* log);
	
	int getState();

	/* Performing functions no exceptions */
	void relievePressure();
	void setRelayGroup(int iGroup, bool internal = false);
	void setRelaysState(int address, int states);
	void reinit();
}

#endif