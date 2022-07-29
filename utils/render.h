/* Author: Jodzik */
#ifndef RENDER_H_
#define RENDER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"

#include <stdint.h>

using namespace std;

namespace render {

	void init(json& display_cnf, Logger* log);
	void showFrame(string name);

	void showTempFrame(string name);

	void terminate();

	bool getState();
}

#endif