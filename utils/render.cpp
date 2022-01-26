#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../ledmatrix-linux/LedMatrix.h"
#include "../rgb332/rgb332.h"
#include "../font-linux/Font.h"
#include "../BMP.h"
#include "../logger-linux/Logger.h"

#include <queue>
#include <sstream>
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
#include <time.h>

using namespace std;

namespace render {

DisplayType displayType = DisplayType::STD;

namespace {
	struct Var {
		const void* var;
		VarType type;
		wstring name;
		int precision;
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
	struct TempFrame {
		Frame* frame;
		time_t tBegin;
		int tShow;
	};

	thread* _genth = nullptr;
	LedMatrix* _lm;
	vector<Var> _vars;
	vector<Frame> _lmframes;
	int defFont;
	uint8_t defColor;
	uint8_t defX;
	uint8_t defY;
	LedMatrix::Mode defMode;
	
	Frame* _currentRenderingFrame = nullptr;
	Frame* _renderingFrame = nullptr;
	queue<TempFrame> _tempFrameQueue;
	int _redrawBorehole = 200;
	int _handlerDelay = 10000;
	bool _pushedRedraw = false;
	
	int _giveMoneyFrame = -1;
	int _bonusGiveMoneyFrame = -1;
	int _logoFrame = -1;
	int _unknownCardFrame = -1;
	int _bonusErrorFrame = -1;
	int _internalErrorFrame = -1;
	int _serviceFrame = -1;
	int _repairFrame = -1;
	int _disconnectBg = -1;
	int _disconnectFrameTimeout = -1;
	char _encode[32] = {0};
	vector<int> _fontsId;
	vector<int> _blocksId;

	int _parse(wstring& s, int b, Frame& f);
	int _parseCtrl(wstring& s, int b, Frame& f);

	template<typename T>
	bool _is_contain(vector<T> vec, T val) {
		for (int i = 0; i < vec.size(); i++) {
			if (vec[i] == val) {
				return true;
			}
		}

		return false;
	}

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

	void _lmredraw() {
		// setup default to frame options
		if (_currentRenderingFrame->bg >= 0) {
			_lm->writePrepareBlock(_currentRenderingFrame->bg, false);
			usleep(50000);
		}
		_lm->setOpt(_currentRenderingFrame->mode, _currentRenderingFrame->font, _currentRenderingFrame->color);
		usleep(5000);
		// cout << "set opt: M " << (int)_currentRenderingFrame->mode << ", F " << (int)_currentRenderingFrame->font << ", C " << (int)_currentRenderingFrame->color << endl;
		// inject variables
		wstring ss = _currentRenderingFrame->format;
		for (int i = 0; i < _vars.size(); i++) {
			size_t vp = ss.find(L"$" + _vars[i].name);
			if (vp != string::npos) {
				wstring vsv;
				if (_vars[i].type == VarType::FLOAT) {
					double dv = *((const double*)_vars[i].var);
					int iv = (int)ceil(dv);
					vsv = to_wstring(iv);
				} else 
				if (_vars[i].type == VarType::INT) {
					vsv = to_wstring(*((const int*)_vars[i].var));
				} else
				if (_vars[i].type == VarType::STRING) {
					string s(((const char*)_vars[i].var));
					vsv = _s2ws(s);
				}
				ss = ss.replace(vp, _vars[i].name.length() + 1, vsv);
			}
		}
		string css = _ws2s(ss);
		_lm->writeString(css.c_str(), _encode);
	}

	void _redraw() {
		if (_currentRenderingFrame == nullptr) {
			return;
		}
		cout << "[INFO][RENDER] redraw " << _currentRenderingFrame->id << " frame.." << endl;
		if (_lm != nullptr) {
			_lmredraw();
		} else {
			sleep(5);
			throw runtime_error ("std redraw still not supported");
		}
	}

	void _handler() {
		int i = 0;
		while (true) {
			try {
				if (_tempFrameQueue.size() > 0) {
					TempFrame& tf = _tempFrameQueue.front();
					if (tf.tBegin == 0) {
						_currentRenderingFrame = tf.frame;
						tf.tBegin = time(NULL);
						_redraw();
						_pushedRedraw = false;
						i = _redrawBorehole + 1;
					} else {
						if (time(NULL) - tf.tBegin >= tf.tShow) {
							_tempFrameQueue.pop();
							if (_tempFrameQueue.size() == 0) {
								_currentRenderingFrame = _renderingFrame;
								_redraw();
								_pushedRedraw = false;
								i = _redrawBorehole + 1;
							}
						}
					}
				}

				if (i % _redrawBorehole == 0 || _pushedRedraw) {
					_redraw();
					i = _redrawBorehole;
					_pushedRedraw = false;
				}
				usleep(_handlerDelay);
				i++;
			} catch (exception& e) {
				cout << "[WARNING][RENDER]|HANDLER| " << e.what() << endl;
				usleep(10000);
			}
		}
	}

	Frame* _getFrame(int id) {
		for (int i = 0; i < _lmframes.size(); i++) {
			if (_lmframes[i].id == id) {
				return & _lmframes[i];
			}
		}
		throw runtime_error("ledmatrix frame '" + to_string(id) + "' not found");
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
			if (mark != b + 3) {
				return 0;
			}
			f.format.push_back((wchar_t)0x01);
			f.format += s.at(b+2);
			int al = _parse(s, b + 4, f);
			f.format.push_back((wchar_t)0x06);
			return b + 4 + al;
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
	Var _var = {var, VarType::INT, name, 0};
	_vars.push_back(_var);
}

void regVar(const double* var, wstring name, int precision) {
	Var _var = {var, VarType::FLOAT, name, precision};
	_vars.push_back(_var);
}

void regVar(const char* var, wstring name) {
	Var _var = {var, VarType::STRING, name, 0};
	_vars.push_back(_var);
}

void init(json& displaycnf, json& frames, json& specFrames, json& option, json& bg, json& fonts) {
	// get necessary config fields
	cout << "[INFO][RENDER] get necessary config fields.." << endl;
	string dt = JParser::getf(displaycnf, "type", "display");
	_giveMoneyFrame = JParser::getf(specFrames, "give-money", "spec-frames");
	_bonusGiveMoneyFrame = JParser::getf(specFrames, "give-money-bonus", "spec-frames");
	_bonusErrorFrame = JParser::getf(specFrames, "error-bonus", "spec-frames");
	_unknownCardFrame = JParser::getf(specFrames, "unknown-card", "spec-frames");
	_internalErrorFrame = JParser::getf(specFrames, "error-internal", "spec-frames");
	_logoFrame = JParser::getf(specFrames, "logo", "spec-frames");
	_serviceFrame = JParser::getf(specFrames, "service", "spec-frames");
	_repairFrame = JParser::getf(specFrames, "repair", "spec-frames");

	if (dt == "ledmatrix") {
		cout << "[INFO][RENDER] display type is 'ledmatrix'" << endl;
		// parse general options
		cout << "[INFO][RENDER] parse general options.." << endl;
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
		string encode = JParser::getf(option, "encode", "general-option");
		strcpy(_encode, encode.c_str());
		_disconnectFrameTimeout = JParser::getf(option, "disconnect-frame-timeout", "general-option");
		_disconnectBg = JParser::getf(option, "disconnect-bg", "general-option");

		// load frames
		cout << "[INFO][RENDER] load frames.." << endl;
		for (int i = 0; i < frames.size(); i++) {
			try {
				Frame f;
				f.id = frames[i]["id"];
				try {
					f.bg = frames[i]["bg"];
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
				f.format = L"";
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
				f.format.push_back((wchar_t)0x08);
				_lmframes.push_back(f);
				// cout << "frame " << f.id << " added, format: " << _ws2s(f.format)  << endl;
			} catch (exception& e) {
				throw runtime_error("fail to load " + to_string(i) + " frame: " + string(e.what()));
			}
		}

		cout << "[INFO][RENDER] assert spec frames.." << endl;
		try {
			_getFrame(_logoFrame);
		} catch (exception& e) {
			throw runtime_error("no logo frame (" + to_string(_logoFrame) + ") in frames");
		}
		try {
			_getFrame(_giveMoneyFrame);
		} catch (exception& e) {
			throw runtime_error("no give-money frame (" + to_string(_giveMoneyFrame) + ") in frames");
		}
		try {
			_getFrame(_bonusGiveMoneyFrame);
		} catch (exception& e) {
			throw runtime_error("no give-money-bonus frame (" + to_string(_bonusGiveMoneyFrame) + ") in frames");
		}
		try {
			_getFrame(_bonusErrorFrame);
		} catch (exception& e) {
			throw runtime_error("no error-bonus frame (" + to_string(_bonusErrorFrame) + ") in frames");
		}
		try {
			_getFrame(_unknownCardFrame);
		} catch (exception& e) {
			throw runtime_error("no unknown-card frame (" + to_string(_unknownCardFrame) + ") in frames");
		}
		try {
			_getFrame(_internalErrorFrame);
		} catch (exception& e) {
			throw runtime_error("no error-internal frame (" + to_string(_internalErrorFrame) + ") in frames");
		}
		try {
			_getFrame(_serviceFrame);
		} catch (exception& e) {
			throw runtime_error("no service frame (" + to_string(_serviceFrame) + ") in frames");
		}
		try {
			_getFrame(_repairFrame);
		} catch (exception& e) {
			throw runtime_error("no repair frame (" + to_string(_repairFrame) + ") in frames");
		}

		// init hardware
		cout << "[INFO][RENDER] init hardware" << endl;
		int addr = JParser::getf(displaycnf, "address", "display");
		int dp = JParser::getf(displaycnf, "dir-pin", "display");
		string driver = JParser::getf(displaycnf, "driver", "display");
		int brn = JParser::getf(displaycnf, "baud-rate", "display");
		_handlerDelay = JParser::getf(displaycnf, "handler-delay", "display");
		_redrawBorehole = JParser::getf(displaycnf, "redraw-borehole", "display");
		int br;
		if (brn == 2400) {
			br = B2400;
		} else
		if (brn == 9600) {
			br = B9600;
		} else
		if (brn == 38400) {
			br = B38400;
		} else
		if (brn == 115200) {
			br = B115200;
		} else {
			throw runtime_error("baud-rate '" + to_string(brn) + "' not supported, choose from 2400, 9600, 38400, 115200");
		}

		try {
			_lm = new LedMatrix(addr, driver, br, dp);
		} catch (exception& e) {
			throw runtime_error("fail to hardware init: " + string(e.what()));
		}

		// prepare fonts
		cout << "[INFO][RENDER] prepare fonts.." << endl;
		for (int i = 0; i < fonts.size(); i++) {
			try {
				string name = JParser::getf(fonts[i], "name", "fonts[" + to_string(i) + "]");
				string ind = JParser::getf(fonts[i], "index", "fonts[" + to_string(i) + "]");
				int id = JParser::getf(fonts[i], "id", "fonts[" + to_string(i) + "]");
				if (_is_contain(_fontsId, id)) {
					throw runtime_error("font " + to_string(id) + " duplicate");
				}
				_fontsId.push_back(id);
				Font font("./config/fonts/" + name, ind);
				_lm->prepareFont(font, id);
			} catch (exception& e) {
				throw runtime_error("fail to prepare font " + to_string(i) + ": " + string(e.what()));
			}
		}

		// prepare blocks
		cout << "[INFO][RENDER] prepre blocks.." << endl;
		for (int i = 0; i < bg.size(); i++) {
			try {
				string name = JParser::getf(bg[i], "name", "bg[" + to_string(i) + "]");
				int id = JParser::getf(bg[i], "id", "bg[" + to_string(i) + "]");
				if (_is_contain(_blocksId, id)) {
					throw runtime_error("block " + to_string(id) + " duplicate");
				}
				_blocksId.push_back(id);
				_lm->prepareBlock("./config/img/" + name, id);
			} catch (exception& e) {
				throw runtime_error("fail to prepare block " + to_string(i) + ": " + string(e.what()));
			}
		}
		if (_disconnectBg >= 0 && _disconnectFrameTimeout > 0) {
			cout << "[INFO][RENDER] configure disconnect behavior.." << endl;
			_lm->setDisconnectBlock(_disconnectBg, _disconnectFrameTimeout);
		}
		cout << "[INFO][RENDER] reset and clear display.." << endl;
		try {
			_lm->reset();
			_lm->clear();
		} catch (exception& e) {
			throw runtime_error("fail to reset display: " + string(e.what()));
		}
	} else
	if (dt == "std") {
		throw runtime_error("'std' display type still not supported");
	} else {
		throw runtime_error("unknown display type '" + dt + "'");
	}
	cout << "[INFO][RENDER] start handler.." << endl;
	_genth = new thread(_handler);
}

void showFrame(SpecFrame frame) {
	int f = -1;
	
	switch (frame) {
	case SpecFrame::LOGO: f = _logoFrame; break;
	case SpecFrame::UNKNOWN_CARD: f = _unknownCardFrame; break;
	case SpecFrame::BONUS_ERROR: f = _bonusErrorFrame; break;
	case SpecFrame::INTERNAL_ERROR: f = _internalErrorFrame; break;
	case SpecFrame::GIVE_MONEY: f = _giveMoneyFrame; break;
	case SpecFrame::GIVE_MONEY_BONUS: f = _bonusGiveMoneyFrame; break;
	case SpecFrame::SERVICE: f = _serviceFrame; break;
	case SpecFrame::REPAIR: f = _repairFrame; break;
	}

	try {
		showFrame(f);
	} catch (exception& e) {
		cout << "[WARNING][RENDER] fail show frame: " << e.what() << endl;
	}
}

void showFrame(int idFrame) {
	Frame* f = _getFrame(idFrame);
	_renderingFrame = f;
	if (_tempFrameQueue.size() == 0) {
		_currentRenderingFrame = f;
		_pushedRedraw = true;
	}
}

void showTempFrame(SpecFrame frame, int tSec) {
	int f = -1;
	
	switch (frame) {
	case SpecFrame::LOGO: f = _logoFrame; break;
	case SpecFrame::UNKNOWN_CARD: f = _unknownCardFrame; break;
	case SpecFrame::BONUS_ERROR: f = _bonusErrorFrame; break;
	case SpecFrame::INTERNAL_ERROR: f = _internalErrorFrame; break;
	case SpecFrame::GIVE_MONEY: f = _giveMoneyFrame; break;
	case SpecFrame::GIVE_MONEY_BONUS: f = _bonusGiveMoneyFrame; break;
	case SpecFrame::SERVICE: f = _serviceFrame; break;
	case SpecFrame::REPAIR: f = _repairFrame; break;
	}
	
	try {
		showTempFrame(f, tSec);
	} catch (exception& e) {
		cout << "[WARNING][RENDER] fail show temp frame: " << e.what() << endl;
	}
}

void showTempFrame(int idFrame, int tSec) {
	TempFrame tf;
	tf.frame = _getFrame(idFrame);
	tf.tShow = tSec;
	tf.tBegin = 0;
	_tempFrameQueue.push(tf);
}

void redraw() {
	_pushedRedraw = true;
}

}