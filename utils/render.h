/* Author: Jodzik */
#ifndef RENDER_H_
#define RENDER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"

#include <stdint.h>

using namespace std;

namespace render {
	enum class SpecFrame {
		LOGO,
		UNKNOWN_CARD,
		BONUS_ERROR,
		INTERNAL_ERROR,
		REPAIR,
		GIVE_MONEY,
		GIVE_MONEY_BONUS,
		SERVICE,
		NOMONEY,
		WAIT,
		SERVER_CONNECT
	};
	enum DisplayType {
		LEDMATRIX,
		STD
	};
	enum VarType {
		INT,
		FLOAT,
		STRING
	};

	void regVar(const int* var, wstring name);
	void regVar(const double* var, wstring name, int precision);
	void regVar(const char* var, wstring name);

	void init(json& displaycnf, Logger* log);
	void showFrame(SpecFrame frame);
	void showFrame(int idFrame);

	/* * * Show frame for a few seconds, if tSec = 0 - frame showing while call dropTempFrame * * */
	void showTempFrame(SpecFrame frame, int tSec);
	void showTempFrame(int idFrame, int tSec);
	void dropTempFrame();

	void redraw();

	bool getState();
}

#endif