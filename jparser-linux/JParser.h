#pragma once

#include "../json.h"

#include <string>

using namespace std;
using json = nlohmann::json;

class JParser {
public:
    JParser(string path = "./config/config.json");
    virtual ~JParser();
    
	json& get(string field);
	static json& getf(json& jobj, string field, string from = "some jobj");

	json& getFull();

	static json& getByName(json& jarray, string name, string from = "some jarray");
private:
    json _jconfig;
};
