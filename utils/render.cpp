#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../ledmatrix-linux/LedMatrix.h"
#include "../rgb332/rgb332.h"
#include "../font-linux/Font.h"
#include "../BMP.h"
#include "../logger-linux/Logger.h"

#include <stdint.h>
#include <vector>
#include <iostream>
#include <thread>
#include <string>

using namespace std;

namespace render {

DisplayType displayType = DisplayType::STD;

namespace {
	struct MFont {
		Font font;
		int id;
	};
	struct Block {
		RGB332 data;
		int id;
	};
	struct Var {
		const void* var;
		VarType type;
		string name;
		int pos;
	};
	struct Frame {
		int id;
		int font;
		int bg;
		uint8_t color;
		LedMatrix::Mode mode;
		string format;
		vector<Var> vars;
	};

	LedMatrix* _lm;
	vector<MFont> _fonts;
	vector<Block> _blocks;
	vector<Var> _vars;
	int defFont;
	uint8_t defColor;
	uint8_t defX;
	uint8_t defY;

	void _handler() {
		
	}

	string _parse(string b) {
		if ((b[0] != '{' || b[b.length()-1] != '}') || b.find(':', 0)) {
			return b;
		}

		switch (b[1]) {
		case 'c':
			
			break;
		case '$':
			
			break;
		case 'f':

			break;
		default: return b;
		}
	}
}

void regVar(const int* var, string name) {
	Var _var = {var, VarType::INT, name};
	_vars.push_back(_var);
}

void regVar(const double* var, string name) {
	Var _var = {var, VarType::FLOAT, name};
	_vars.push_back(_var);
}

void regVar(const char* var, string name) {
	Var _var = {var, VarType::STRING, name};
	_vars.push_back(_var);
}

void init(json& displaycnf, json& frames, json& option, json& bg, json& fonts) {
	string dt = JParser::getf(displaycnf, "type", "display");
	if (dt == "ledmatrix") {
		// init hardware
		int addr = JParser::getf(displaycnf, "address", "display");
		int dp = JParser::getf(displaycnf, "dir-pin", "display");
		string driver = JParser::getf(displaycnf, "driver", "display");
		_lm = new LedMatrix(addr, driver, B9600, dp);

		// prepare fonts
		for (int i = 0; i < fonts.size(); i++) {
			string name = JParser::getf(fonts[i], "name", "fonts[" + to_string(i) + "]");
			string ind = JParser::getf(fonts[i], "index", "fonts[" + to_string(i) + "]");
			int id = JParser::getf(fonts[i], "id", "fonts[" + to_string(i) + "]");
			Font font("./config/fonts/" + name, ind);
			_lm->prepareFont(font, id);
			MFont mf = {font, id};
			_fonts.push_back(mf);
		}

		// prepare blocks
		for (int i = 0; i < bg.size(); i++) {
			string name = JParser::getf(fonts[i], "name", "bg[" + to_string(i) + "]");
			int id = JParser::getf(fonts[i], "id", "bg[" + to_string(i) + "]");
			BMP bmp(name.c_str());
			int w = bmp.bmp_info_header.width;
			int h = bmp.bmp_info_header.height;
			if (bmp.bmp_info_header.bit_count != 24) {
				throw runtime_error("bg[" + to_string(i) + "] image must be 24-bit encoded");
			}
			RGB332 data = rgb332_create(w, h);
			for (int ih = 0; ih < h; ih++) {
				for (int iw = 0; iw < w; iw++) {
					int ip = ih * w * 3 + iw * 3;
					uint8_t r = bmp.data.at(ip);
					uint8_t g = bmp.data.at(ip + 1);
					uint8_t b = bmp.data.at(ip + 2);
					rgb332_set(data, iw, ih, rgb332_formColor(r, g, b));
				}
			}
			_lm->prepareBlock(data, id);
			Block block = {data, id};
			_blocks.push_back(block);
		}
		_lm->reset();
		_lm->clear();

		// parse general options
		defFont = JParser::getf(option, "default-font", "general-option");
		defColor = JParser::getf(option, "default-color", "general-option");
		json& defCursor = JParser::getf(option, "default-cursor", "general-option");
		defX = defCursor[0];
		defY = defCursor[1];

		// load frames
		for (int i = 0; i < frames.size(); i++) {

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