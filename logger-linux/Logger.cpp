#include "Logger.h"

#include <string>
#include <iostream>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace std;

Logger::Logger(string path) {
	_path = path;
	_level = 0;
}

Logger::~Logger() {

}

void Logger::log(Type type, string module, string message, int level) {
	time_t theTime = time(NULL);
	tm* ptm = localtime(&theTime);

	_clearSpec(module);
	_clearSpec(message);

	ostringstream strTime;
	ostringstream strYear;
	ostringstream strMon;
	ostringstream strDay;
	ostringstream strHour;
	ostringstream strMin;
	ostringstream strSec;
	ostringstream strType;
	ostringstream strModule;
	strYear << setfill('0') << setw(4) << 1900 + ptm->tm_year;
	strMon << setfill('0') << setw(2) << ptm->tm_mon + 1;
	strDay << setfill('0') << setw(2) << ptm->tm_mday;
	strHour << setfill('0') << setw(2) << ptm->tm_hour;
	strMin << setfill('0') << setw(2) << ptm->tm_min;
	strSec << setfill('0') << setw(2) << ptm->tm_sec;
	strTime << "[" << strYear.str() << "-" << strMon.str() << "-" << strDay.str() << " " << strHour.str() << ":" << strMin.str() << ":" << strSec.str() << "]";
	strType << "[" << _typeToString(type) << "]";
	strModule << "[" << module << "]";

	_setColorAsType(type);
	cout << strTime.str() << strType.str() << strModule.str() << " " << message << endl;
	setConsoleFontColor(Color::DEFAULT);

	if (level <= _level) {
		ofstream f(_path.c_str(), ofstream::out | ofstream::app);
		if (!f.is_open()) {
			throw runtime_error("fail to open log file");
		}
		f << strTime.str() << strType.str() << strModule.str() << " " << message << endl;
		f.close();
	}
}

void Logger::setLogLevel(int level) {
	_level = level;
}

void Logger::setConsoleFontColor(Color color) {
	switch (color)
	{
	case Color::DEFAULT:
		cout << "\033[0m";
		break;
	case Color::GRAY:
		cout << "\033[0;37m";
		break;
	case Color::RED:
		cout << "\033[1;31m";
		break;
	case Color::YELLOW:
		cout << "\033[0;33m";
		break;
	case Color::GREEN:
		cout << "033[32";
		break;
	default:
		break;
	}
}

string Logger::_typeToString(Type type) {
	if (type == Type::INFO) {
		return "INFO";
	} else
	if (type == Type::WARNING) {
		return "WARNING";
	} else
	if (type == Type::ERROR) {
		return "ERROR";
	} else
	if (type == Type::DBG) {
		return "DBG";
	} else {
		throw runtime_error("unknown log type");
	}
}

void Logger::_setColorAsType(Type type) {
	if (type == Type::INFO) {
		setConsoleFontColor(Color::DEFAULT);
	} else
	if (type == Type::WARNING) {
		setConsoleFontColor(Color::YELLOW);
	} else
	if (type == Type::ERROR) {
		setConsoleFontColor(Color::RED);
	} else {
		throw runtime_error("unknown log type");
	}
}

void Logger::_clearSpec(string& s) {
	for (int i = 0; i < s.length(); i++) {
		if (s.at(i) == '[' || s.at(i) == ']') {
			s[i] = '|';
		}
		if (s.at(i) == '\n') {
			s[i] = '\t';
		}
		if (s.at(i) == '\r') {
			s.erase(i, 1);
		}
	}
}