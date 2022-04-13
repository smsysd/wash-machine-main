#pragma once

#include <string>

using namespace std;

class Logger {
public:
	enum class Type {
		INFO,
		WARNING,
		ERROR,
		DBG
	};

	enum class Color {
		DEFAULT,
		GRAY,
		RED,
		YELLOW,
		GREEN
	};

	Logger(string path);
	virtual ~Logger();

	void log(Type type, string module, string message, int level = 0);

	void setConsoleFontColor(Color color);
	void setLogLevel(int level);

private:
	string _path;
	int _level;

	string _typeToString(Type type);
	void _setColorAsType(Type type);
	void _clearSpec(string& s);
};
