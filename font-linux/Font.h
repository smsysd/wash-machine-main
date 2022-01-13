#ifndef _FONT_H_
#define _FONT_H_

#include "../rgb332/rgb332.h"
#include "../jparser-linux/JParser.h"

#include <string>
#include <stdint.h>
#include <vector>

using namespace std;
using json = nlohmann::json;

class Font {
public:
    Font(string pathToFont, string index = "0", int width = 0, int height = 0);
    ~Font();

    RGB332 getGliph(int c);
    vector<int> getCharCodes();
    vector<int> getStringDimensions(wstring);
	int getMaxHeight();
private:

    struct Gliph {
        RGB332 bitmap;
        int charCode;
    };

    static const char _fontMarks[];
    static const char _bgMarks[];

    vector<Gliph> _gliphs;

    void _loadJsonFont(string file, string index);
    RGB332 _extractJsonBitmap(json& bitmap);

    void _loadTxtFont(string file);
    int _extractTxtCharCode(char* entry);
    int _extractTxtCharHeight(char* entry);
    int _extractTxtCharWidth(char* entry);
    RGB332 _extractTxtCharBitmap(char* entry, int height, int width);

    bool _isBgMark(char c);
    bool _isFontMark(char c);

    void _reallocateBitmap(RGB332 bitmap, int width, int height);
};

#endif
