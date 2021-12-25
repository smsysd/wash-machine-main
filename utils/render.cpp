#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../ledmatrix-linux/LedMatrix.h"
#include "../rgb332/rgb332.h"
#include "../font-linux/Font.h"

#include <stdint.h>

using namespace std;

namespace render {

DisplayType displayType = DisplayType::STD;

namespace {
	LedMatrix* _lm;
}

void init(json& displaycnf, json& frames, json& option, json& bg, json& fonts) {
	string dt = JParser::getf(displaycnf, "type", "display");
	if (dt == "ledmatrix") {
		int addr = JParser::getf(displaycnf, "address", "display");
		int dp = JParser::getf(displaycnf, "dir-pin", "display");
		string driver = JParser::getf(displaycnf, "driver", "display");
		_lm = new LedMatrix(addr, driver, B9600, dp);
		for (int i = 0; i < fonts.size(); i++) {
			string name = JParser::getf(fonts[i], "name", "fonts[" + to_string(i) + "]");
			string ind = JParser::getf(fonts[i], "index", "fonts[" + to_string(i) + "]");
			int id = JParser::getf(fonts[i], "id", "fonts[" + to_string(i) + "]");
			Font font("./config/fonts/" + name, ind);
			_lm->prepareFont(font, id);
		}
		
	} else
	if (dt == "std") {
		throw runtime_error("'std' display type still not supported");
	} else {
		throw runtime_error("unknown display type");
	}
}

void showFrame(int iFrame) {

}

}