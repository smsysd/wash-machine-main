#include "Font.h"

#include "../rgb332/rgb332.h"

#include "../jparser-linux/JParser.h"

#include <string>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <locale>
#include <codecvt>

using namespace std;
using json = nlohmann::json;

const char Font::_fontMarks[] = {'X', '#', '@'};
const char Font::_bgMarks[] = {'.', '-'};

Font::Font(string pathToFont, string index, int width, int height) {
    int temp = pathToFont.rfind('.');
    string format = pathToFont.substr(temp + 1);
    if (format == "txt") {
        _loadTxtFont(pathToFont);
    } else
    if (format == "json") {
		_loadJsonFont(pathToFont, index);
    } else {
		throw runtime_error("'." + format + "' format not supported");
    }
}

Font::~Font() {
    for (int i = 0; i < _gliphs.size(); i++) {
		rgb332_delete(_gliphs[i].bitmap);
	}
}

RGB332 Font::getGliph(int c) {
	for (int i = 0; i < _gliphs.size(); i++) {
        if (c == _gliphs[i].charCode) {
            return _gliphs[i].bitmap;
        }
    }

    throw runtime_error("char '" + to_string(c) + "' not found");
}

vector<int> Font::getCharCodes() {
	vector<int> charCodes;
	
	for (int i = 0; i < _gliphs.size(); i++) {
		charCodes.push_back(_gliphs[i].charCode);
	}

	return charCodes;
}

int Font::getMaxHeight() {
	int h = 0;
	for (int i = 0; i < _gliphs.size(); i++) {
		if (_gliphs[i].bitmap.height > h) {
			h = _gliphs[i].bitmap.height;
		}
	}
	return h;
}

string Font::getEncode() {
	return _encode;
}

vector<int> Font::getStringDimensions(wstring ws) {
	vector<int> dv;
	int width = 0;
	int height = 0;

	for (int i = 0; i < ws.length(); i++) {
		RGB332 bitmap;
		try {
			bitmap = getGliph(ws.at(i));
		} catch (exception& e) {
			bitmap.width = 0;
			bitmap.height = 0;
		}
		width += bitmap.width;
		if (bitmap.height > height) {
			height = bitmap.height;
		}
	}
	dv.push_back(width);
	dv.push_back(height);

	return dv;
}

wstring Font::_s2ws(const std::string& str) {
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string Font::_ws2s(const std::wstring& wstr) {
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

void Font::_loadJsonFont(string file, string index) {
	ifstream f(file.c_str());

	if (f.is_open()) {
		// get length of file:
		f.seekg (0, f.end);
		int length = f.tellg();
		f.seekg (0, f.beg);
		char * buffer = new char [length + 1];
		f.read(buffer,length);
		buffer[length] = 0;
		f.close();
		cout << "buffer filled, parsing.." << endl;

		json data;
		try {
			data = json::parse(buffer);
			cout << "json parsing success" << endl;
		} catch (exception& e) {
			delete[] buffer;
			throw runtime_error("fail to general parse json format");
		}
		delete[] buffer;

		json f = JParser::getf(data, index, "font file");
		json s = JParser::getf(f, "symbols", index);
		_encode = "UTF-16";
		
		cout << "load symbols.." << endl;
		for (int i = 0; i < s.size(); i++) {
			Gliph cb;
			string str = s[i]["name"];
			wstring wstr = _s2ws(str);
			if (wstr.length() > 0) {
				cb.charCode = wstr.at(0);
			} else {
				throw runtime_error("fail to load char '" + str +  "' code");
			}
			try {
				cb.bitmap = _extractJsonBitmap(JParser::getf(s[i], "bitmap", str));
			} catch (exception& e) {
				throw runtime_error("fail to load char '" + str +  "' bitmap: " + string(e.what()));
			}
			_gliphs.push_back(cb);
		}
		cout << "[Font] loaded " << _gliphs.size() << " symbols from json" << endl;
	} else {
		throw runtime_error("cannot open font file");
	}
}

RGB332 Font::_extractJsonBitmap(json& bitmap) {
	int height = bitmap.size();
	int width = 0;
	vector<vector<uint8_t>> data;
	for (int i = 0; i < height; i++) {
		string strline = bitmap[i];
		int cnt = 0;
		vector<uint8_t> line;
		for (int j = 0; j < strline.length(); j++) {
			if (_isFontMark(strline[j])) {
            		cnt++;
				line.push_back(0xFF);
        		} else
        		if (_isBgMark(strline[j])) {
            		cnt++;
				line.push_back(0x00);
			}
		}

		if (cnt > width) {
			width = cnt;
		}
		data.push_back(line);
	}

	RGB332 bm = rgb332_create(width, height);
	if (bm.data == NULL) {
		throw runtime_error("no memory");
	}
	rgb332_fill(bm, 0x00);
	for (int i = 0; i < data.size(); i++) {
		for (int j = 0; j < data[i].size(); j++) {
			rgb332_set(bm, j, i, data[i][j]);
		}
	}

	return bm;
}

void Font::_loadTxtFont(string file) {
    ifstream f(file.c_str());

    if (f.is_open()) {
		// get length of file:
		f.seekg (0, f.end);
		int length = f.tellg();
		f.seekg (0, f.beg);
		char * buffer = new char [length + 1];
		f.read(buffer,length);
        buffer[length] = 0;

        for (int i = 0; i < length; i++) {
            if (buffer[i] == ':') {
                // entry
                Gliph cbitmap;
                int charCode = _extractTxtCharCode(&buffer[i]);
                int height = _extractTxtCharHeight(&buffer[i]);
                int width = _extractTxtCharWidth(&buffer[i]);
                RGB332 bitmap = _extractTxtCharBitmap(&buffer[i], height, width);
                cbitmap.charCode = charCode;
                cbitmap.bitmap = bitmap;
                _gliphs.push_back(cbitmap);
            }
        }

		f.close();
		delete[] buffer;
	} else {
		throw runtime_error("cannot open font file");
	}
}

int Font::_extractTxtCharCode(char* entry) {
    int cc = strtol(entry + 1, NULL, 10);
    if (cc == 0) {
        throw runtime_error("char code not recognized");
    }

    return cc;
}

int Font::_extractTxtCharHeight(char* entry) {
    for (int i = 0; i < 10; i++) {
        if (entry[i] == '[') {
            int ch = strtol(&entry[i + 1], NULL, 10);
            if (ch == 0) {
                throw runtime_error("char height not recognized");
            }

            return ch;
        }
    }

    throw runtime_error("char height not recognized");
}

int Font::_extractTxtCharWidth(char* entry) {
    int p = 0;
    for (int i = 0; i < 30; i++) {
        if (entry[i] == '[') {
            p++;
            if (p == 2) {
                int cw = strtol(&entry[i + 1], NULL, 10);
                if (cw == 0) {
                    throw runtime_error("char width not recognized");
                }

                return cw;
            }
        }
    }

    throw runtime_error("char width not recognized");
}

RGB332 Font::_extractTxtCharBitmap(char* entry, int height, int width) {
    vector<uint8_t> data;
    int expectedSize = height * width;

    while (true) {
        entry++;
        if (*entry == 0) {
            throw runtime_error("char bitmap not reconized: unexpected end of buffer");
        }
        if (*entry == '\n') {
            break;
        }
    }

    while (true) {
        entry++;
        if (*entry == 0) {
            throw runtime_error("char bitmap not reconized: unexpected end of buffer");
        }
        if (*entry == ':') {
            throw runtime_error("char bitmap not reconized: unexpected end of bitmap");
        }

        if (_isFontMark(*entry)) {
            data.push_back(0xFF);
        } else
        if (_isBgMark(*entry)) {
            data.push_back(0x00);
        }

        if (data.size() >= expectedSize) {
            break;
        }
    }

    RGB332 bitmap = rgb332_create(width, height);
    if (bitmap.data == NULL) {
        throw runtime_error("no memory");
    }

    for (int i = 0; i < expectedSize; i++) {
        bitmap.data[i] = data[i];
    }

    return bitmap;
}

bool Font::_isBgMark(char c) {
    for (int i = 0; i < sizeof(_bgMarks); i++) {
        if (c == _bgMarks[i]) {
            return true;
        }
    }

    return false;
}

bool Font::_isFontMark(char c) {
    for (int i = 0; i < sizeof(_fontMarks); i++) {
        if (c == _fontMarks[i]) {
            return true;
        }
    }

    return false;
}

void Font::_reallocateBitmap(RGB332 bitmap, int width, int height) {
    int maxWidth = width > bitmap.width ? width : bitmap.width;
    int maxHeight = height > bitmap.height ? height : bitmap.height;

    rgb332_delete(bitmap);
    bitmap = rgb332_create(maxWidth, maxHeight);
    if (bitmap.data == NULL) {
        throw runtime_error("No memory");
    }
}
