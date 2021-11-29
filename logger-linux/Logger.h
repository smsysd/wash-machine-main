#pragma once

#include <string>

using namespace std;

class Logger {
public:
	enum class Type {
		INFO,
		WARNING,
		ERROR
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

	void log(Type type, string module, string message);

	void setConsoleFontColor(Color color);

private:
	string _path;

	string _typeToString(Type type);
	void _setColorAsType(Type type);
	void _clearSpec(string& s);
};
