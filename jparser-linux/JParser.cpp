#include "JParser.h"

#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace std;
using json = nlohmann::json;

JParser::JParser(string path) {
	_path = path;
	ifstream configFile(path, ios_base::in);
	if (configFile.is_open()) {
		// get length of file:
		configFile.seekg (0, configFile.end);
		int length = configFile.tellg();
		configFile.seekg (0, configFile.beg);
		
		char * buffer = new char [length + 1];

		configFile.read(buffer,length);
		
		if (configFile) {
			buffer[length] = 0;
			try {
				_jconfig = json::parse(buffer);
			} catch (exception& e) {
				configFile.close();
				delete[] buffer;
				throw runtime_error(string("json file '" + path + "' have syntax error: ") + string(e.what()) );
			}
		}
		else {
			configFile.close();
			delete[] buffer;
			throw runtime_error("fail to read config file '" + path + "'");
		}
		configFile.close();
		delete[] buffer;
	} else {
		throw runtime_error("cannot open config file '" + path + "'");
	}
}

JParser::~JParser() {
    
}

json& JParser::get(string field) {
    try {
		if (_jconfig[field] != nullptr) {
			return _jconfig[field];
		} else {
			throw runtime_error("no such");
		}
	} catch (exception& e) {
		throw runtime_error("fail to get config field '" + field + "' from '" + _path + "': " + string(e.what()));
	}
}

json& JParser::getf(json& jobj, string field, string from) {
    try {
		if (jobj[field] != nullptr) {
			return jobj[field];
		} else {
			throw runtime_error("no such");
		}
	} catch (exception& e) {
		throw runtime_error("fail to get field '" + field + "' from '" + from + "': " + string(e.what()));
	}
}

json& JParser::getFull() {
	return _jconfig;
}

json& JParser::getByName(json& jarray, string name, string from) {
	if (!jarray.is_array()) {
		throw runtime_error("fail get '" + name + "' form '" + from + "': not is array");
	}

	for (int i = 0; i < jarray.size(); i++) {
		string ename;
		try {
			ename = jarray[i]["name"];
		} catch (exception& e) {
			throw runtime_error("fail get '" + name + "' form '" + from + "': element " + to_string(i) + " no have name");
		}

		if (ename == name) {
			return jarray[i];
		}
	}

	throw runtime_error("fail get '" + name + "' form '" + from + "': not found");
}