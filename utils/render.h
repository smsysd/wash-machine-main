/* Author: Jodzik */
#ifndef RENDER_H_
#define RENDER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"

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
		NOMONEY
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

	void init(json& displaycnf, json& frames, json& specFrames, json& option, json& bg, json& fonts);
	void showFrame(SpecFrame frame);
	void showFrame(int idFrame);
	void showTempFrame(SpecFrame frame, int tSec);
	void showTempFrame(int idFrame, int tSec);

	void redraw();

	bool getState();
}

#endif