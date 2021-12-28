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
#include <wchar.h>
#include <wctype.h>
#include <codecvt>
#include <locale>
#include <unistd.h>

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
		wstring name;
	};
	struct Frame {
		int id;
		int font;
		int bg;
		uint8_t color;
		LedMatrix::Mode mode;
		wstring format;
		vector<Var> vars;
	};

	thread* _lmth = nullptr;
	LedMatrix* _lm;
	vector<MFont> _fonts;
	vector<Block> _blocks;
	vector<Var> _vars;
	int defFont;
	uint8_t defColor;
	uint8_t defX;
	uint8_t defY;
	LedMatrix::Mode defMode;

	int _parse(wstring& s, int b, Frame& f);
	int _parseCtrl(wstring& s, int b, Frame& f);

	wstring _s2ws(const std::string& str) {
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.from_bytes(str);
	}

	string _ws2s(const std::wstring& wstr) {
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.to_bytes(wstr);
	}

	void _lmhandler() {

		usleep(10000);
	}

	Var& _getVar(wstring name) {
		for (int i = 0; i < _vars.size(); i++) {
			if (_vars[i].name == name) {
				return _vars[i];
			}
		}
		throw runtime_error("var '" + _ws2s(name) + "' not fount");
	}

	int _parse(wstring& s, int b, Frame& f) {
		int i = 0;
		for (i = b; i < s.length(); i++) {
			if (s.at(i) == L'{') {
				int il = _parseCtrl(s, i, f);
				if (il == 0) {
					f.format += L'{';
				} else {
					i += il;
				}
			} else
			if (s.at(i) == L'}') {
				return i + 1;
			} else {
				f.format += s.at(i);
			}
		}
		return i;
	}

	int _parseCtrl(wstring& s, int b, Frame& f) {
		if (s[b] != '{') {
			return 0;
		}
		int mark = s.find(':', b);
		if (mark == -1) {
			return 0;
		}

		if (s.at(b+1) == 'c') {
			if (mark != b + 4) {
				return 0;
			}
			f.format.push_back((wchar_t)0x01);
			f.format += s.at(b+2);
			f.format += s.at(b+3);
			int al = _parse(s, b + 5, f);
			f.format.push_back((wchar_t)0x06);
			return b + 5 + al;
		} else
		if (s.at(b+1) == 'f') {
			if (mark != b + 3) {
				return 0;
			}
			f.format.push_back((wchar_t)0x05);
			f.format += s.at(b+2);
			int al = _parse(s, b + 4, f);
			f.format.push_back((wchar_t)0x07);
			return b + 4 + al;
		} else {
			return 0;
		}

	}
}

void regVar(const int* var, wstring name) {
	Var _var = {var, VarType::INT, name};
	_vars.push_back(_var);
}

void regVar(const double* var, wstring name) {
	Var _var = {var, VarType::FLOAT, name};
	_vars.push_back(_var);
}

void regVar(const char* var, wstring name) {
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

		try {
			_lm = new LedMatrix(addr, driver, B9600, dp);
		} catch (exception& e) {
			throw runtime_error("fail to hardware init: fail init display: " + string(e.what()));
		}

		// prepare fonts
		for (int i = 0; i < fonts.size(); i++) {
			try {
				string name = JParser::getf(fonts[i], "name", "fonts[" + to_string(i) + "]");
				string ind = JParser::getf(fonts[i], "index", "fonts[" + to_string(i) + "]");
				int id = JParser::getf(fonts[i], "id", "fonts[" + to_string(i) + "]");
				Font font("./config/fonts/" + name, ind);
				_lm->prepareFont(font, id);
				MFont mf = {font, id};
				_fonts.push_back(mf);
			} catch (exception& e) {
				throw runtime_error("fail to prepare font: " + string(e.what()));
			}
		}

		// prepare blocks
		for (int i = 0; i < bg.size(); i++) {
			try {
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
			} catch (exception& e) {
				throw runtime_error("fail to prepare block: " + string(e.what()));
			}
		}

		try {
			_lm->reset();
			_lm->clear();
		} catch (exception& e) {
			throw runtime_error("fail to reset display");
		}

		// parse general options
		defFont = JParser::getf(option, "default-font", "general-option");
		defColor = JParser::getf(option, "default-color", "general-option");
		json& defCursor = JParser::getf(option, "default-cursor", "general-option");
		defX = defCursor[0];
		defY = defCursor[1];
		string dm = JParser::getf(option, "default-mode", "general-option");
		if (dm == "left") {
			defMode = LedMatrix::Mode::LEFT;
		} else
		if (dm == "center") {
			defMode = LedMatrix::Mode::CENTER;
		} else {
			throw runtime_error("unknown render mode: " + dm);
		}

		// load frames
		for (int i = 0; i < frames.size(); i++) {
			try {
				Frame f;
				f.id = frames[i]["id"];
				try {
					f.bg = frames[i]["background"];
				} catch (exception& e) {
					f.bg = -1;
				}
				try {
					f.font = frames[i]["font"];
				} catch (exception& e) {
					f.font = defFont;
				}
				try {
					f.color = frames[i]["color"];
				} catch (exception& e) {
					f.color = defColor;
				}
				try {
					string mode = frames[i]["mode"];
					if (mode == "left") {
						f.mode = LedMatrix::Mode::LEFT;
					} else
					if (mode == "center") {
						f.mode = LedMatrix::Mode::CENTER;
					} else {
						throw runtime_error("");
					}
				} catch (exception& e) {
					f.mode = defMode;
				}

				// parse frame format
				f.format = L" ";
				f.format[0] = 0x08;
				json& lines = frames[i]["lines"];
				for (int j = 0; j < lines.size(); j++) {
					try {
						wstring line = _s2ws(lines[j]);
						_parse(line, 0, f);
						f.format.push_back((wchar_t)0x0B);
					} catch (exception& e) {
						throw runtime_error("fail to parse frame format: " + string(e.what()));
					}
				}
				f.format.push_back((wchar_t)0x04);
			} catch (exception& e) {
				throw runtime_error("fail to load " + to_string(i) + " frame: " + string(e.what()));
			}
		}

		// start handler
		_lmth = new thread(_lmhandler);

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