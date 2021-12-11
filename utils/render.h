/* Author: Jodzik */
#ifndef RENDER_H_
#define RENDER_H_

#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdint.h>

using namespace std;

namespace render {
	void init(json& displaycnf, json& frames);
	void showFrame(int iFrame);
}

#endif