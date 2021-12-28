/* Author: Jodzik */
#ifndef RENDER_H_
#define RENDER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdint.h>

using namespace std;

namespace render {
	enum DisplayType {
		LEDMATRIX,
		STD
	};
	enum VarType {
		INT,
		FLOAT,
		STRING
	};

	void regVar(int* var, string name);
	void regVar(double* var, string name);
	void regVar(const char* var, string name);

	void init(json& displaycnf, json& frames, json& option, json& bg, json& fonts);
	void showFrame(int iFrame);
}

#endif