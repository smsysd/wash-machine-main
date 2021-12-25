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

	void init(json& displaycnf, json& frames, json& option, json& bg, json& fonts);
	void showFrame(int iFrame);
}

#endif