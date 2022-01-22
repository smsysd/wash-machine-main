#include "bonus.h"
#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <ctime>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;
using json = nlohmann::json;

namespace bonus {

namespace {
	vector<string> _split(const string& s, char delim) {
		vector<string> elems;
		stringstream ss;
		ss.str(s);
		string item;
		while (getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}

	class TimeInterval {
	public:
		TimeInterval() {

		}

		TimeInterval(const TimeInterval& ti, bool isMain) {
			if (isMain) {
				_begin[0] = ti._begin[0];
				_begin[1] = ti._begin[1];
				_end[0] = 0;
				_end[1] = 0;
			} else {
				_begin[0] = 0;
				_begin[1] = 0;
				_end[0] = ti._end[0];
				_end[1] = ti._end[1];
			}
		}

		TimeInterval(const string& time) {
			load(time);
		}

		virtual ~TimeInterval() {

		}

		void load(const string& time) {
			auto sv = _split(time, '-');
			if (sv.size() < 2) {
				throw runtime_error("incorrect time format");
			}

			auto sv0 = _split(sv[0], ':');
			if (sv0.size() < 2) {
				throw runtime_error("incorrect time format");
			}

			auto sv1 = _split(sv[1], ':');
			if (sv1.size() < 2) {
				throw runtime_error("incorrect time format");
			}

			_begin[0] = stoi(sv0[0]);
			_begin[1] = stoi(sv0[1]);
			_end[0] = stoi(sv1[0]);
			_end[1] = stoi(sv1[1]);
		}

		bool isNow(int hours, int minutes) {
			if (isOverflowed()) {
				if (hours > _begin[0]) {
					return true;
				} else
				if (hours == _begin[0]) {
					if (minutes >= _begin[1]) {
						return true;
					}
				}
				if (hours < _end[0]) {
					return true;
				} else
				if (hours == _end[0]) {
					if (minutes <= _end[1]) {
						return true;
					}
				}

				return false;
			} else {
				if (hours > _begin[0] && hours < _end[0]) {
					return true;
				} else
				if (hours == _begin[0]) {
					if (minutes >= _begin[1]) {
						return true;
					}
				} else
				if (hours == _end[0]) {
					if (minutes <= _end[1]) {
						return true;
					}
				}

				return false;
			}
		}

		bool isOverflowed() {
			if (_begin[0] > _end[0]) {
				return true;
			}
			if (_begin[0] == _end[0]) {
				if (_begin[1] > _end[1]) {
					return true;
				}
			}

			return false;
		}
	private:
		int _begin[2];
		int _end[2];
	};

	struct Promotion {
		enum Type {
			COEFFICIENT
		};

		int id;
		Type type;
		double k;
		double sk;
		int priority;
		TimeInterval times;
		bool weekdays[7];
		bool months[12];

		bool isActive() {
			time_t ct = time(nullptr);
			tm* cdt = localtime(&ct);
			if (months[cdt->tm_mon]) {
				if (weekdays[cdt->tm_wday-1]) {
					if (times.isNow(cdt->tm_hour, cdt->tm_min)) {
						return true;
					}
				}
			}
			return false;
		}
	};

	vector<Promotion> _promotions;
	int _bonus = 0;
	Type _type;
	string _cert;
	char _access[256];
	double _desiredWriteoff = 0;
	bool _multibonus = false;
	bool _isOpen = false;

	json _japi(string ucmd) {
		char buf[4096] = {0};
		string cmd  = "python3 ./point_api.py " + _cert + " " + string(_access) + " " + ucmd;
		FILE* pp = popen(cmd.c_str(), "r");
		if (pp == NULL) {
			throw runtime_error("fail to open pipe to 'point_api.py'");
		}
		int rc = fread(buf, 2, 2048, pp);
		if (rc < 1) {
			throw runtime_error("fail to read out from 'point_api.py'");
		}
		pclose(pp);
		json data;
		try {
			data = json::parse(buf);
		} catch (exception& e) {
			throw runtime_error("fail to parse result from 'point_api.py': " + string(e.what()) + "\noutput: '" + buf + "'");
		}
		rc = JParser::getf(data, "rc", "'point_api.py'");
		string rt = JParser::getf(data, "rt", "'point_api.py'");
		if (rc < 0) {
			throw runtime_error("error response '" + to_string(rc) + "': " + rt);
		}
		return data;
	}
}

void init(json& bonusSysCnf, json& promotions) {
	// link with render
	render::regVar(&_bonus, L"gbonus");
	
	// general init
	string bt = JParser::getf(bonusSysCnf, "type", "bonus-system");
	if (bt == "aquanter") {
		_type = Type::AQUANTER;
		_cert = JParser::getf(bonusSysCnf, "certificate", "bonus-system");

	} else
	if (bt == "gazoil") {
		_type = Type::GAZOIL;
		throw runtime_error("bonus system '" + bt + "' still not supported");
	} else {
		throw runtime_error("unknown bonus system type '" + bt + "'");
	}

	_desiredWriteoff = JParser::getf(bonusSysCnf, "bonus-writeoff", "bonus-system");
	_multibonus = JParser::getf(bonusSysCnf, "multibonus", "bonus-system");

	// fill promotions
	for (int i = 0; i < promotions.size(); i++) {
		try {
			json& jp = promotions[i];
			Promotion sp;
			sp.id = JParser::getf(jp, "id", "promotion");
			string pt = JParser::getf(jp, "type", "promotion");
			if (pt == "coefficient") {
				sp.type = Promotion::COEFFICIENT;
				sp.k = JParser::getf(jp, "k", "promotion");
				sp.sk = JParser::getf(jp, "sumk", "promotion");
				sp.priority = JParser::getf(jp, "priority", "promotion");
				string timint = JParser::getf(jp, "time", "promotion");
				if (timint == "every") {
					sp.times.load("00:00-23:59");
				} else {
					sp.times.load(timint);
				}
				if (jp["week-days"].is_string()) {
					for (int i = 0; i < 7; i++) {
						sp.weekdays[i] = true;
					}
				} else {
					for (int i = 0; i < 7; i++) {
						sp.weekdays[i] = false;
					}
					for (int i = 0; i < jp["week-days"].size(); i++) {
						int day = jp["week-days"][i];
						if (day < 1 || day > 7) {
							throw runtime_error("incorrect 'week-days'");
						}
						day--;
						sp.weekdays[day] = true;
					}
				}
				if (jp["months"].is_string()) {
					for (int i = 0; i < 12; i++) {
						sp.months[i] = true;
					}
				} else {
					for (int i = 0; i < 12; i++) {
						sp.months[i] = false;
					}
					for (int i = 0; i < jp["months"].size(); i++) {
						int month = jp["months"][i];
						if (month < 1 || month > 12) {
							throw runtime_error("incorrect 'months'");
						}
						month--;
						sp.months[month] = true;
					}
				}
			} else {
				throw runtime_error("unknown type '" + pt + "'");
			}
			_promotions.push_back(sp);
			cout << "[INFO][BONUS] add promotion " << sp.id << endl;
		} catch (exception& e) {
			throw runtime_error("fail to load promomtion " + to_string(i) + ": " + string(e.what()));
		}
	}
}

bool open(CardInfo& card, const char* access) {
	memset(_access, 0, sizeof(_access));
	strcpy(_access, access);
	CardInfo ci = {.id = 0, .count = 0};
	ci.type = CardInfo::UNKNOWN;
	try {
		json res = _japi("open");
		int ct = JParser::getf(res, "type", "server response");
		double cnt = JParser::getf(res, "count", "server response");
		uint64_t cid = JParser::getf(res, "id", "server response");
		if (ct == 0) {
			ci.type = CardInfo::BONUS_PERS;
		} else
		if (ct == 1) {
			ci.type = CardInfo::BONUS_ORG;
		} else
		if (ct == 5) {
			ci.type = CardInfo::SERVICE;
		} else {
			ci.type = CardInfo::UNKNOWN;
		}
		ci.id = cid;
		ci.count = cnt;
		card = ci;
	} catch (exception& e) {
		cout << "[INFO][BONUS] can't open transaction: " << e.what() << endl;
		return false;
	}
	_isOpen = true;
	return true;
}

bool open(CardInfo& card, uint64_t access) {
	memset(_access, 0, sizeof(_access));
	char straccess[32] = {0};
	sprintf(straccess, "SAK%X", access);
	strcpy(_access, straccess);
	CardInfo ci = {.id = 0, .count = 0};
	ci.type = CardInfo::UNKNOWN;
	try {
		json res = _japi("open");
		int ct = JParser::getf(res, "type", "server response");
		double cnt = JParser::getf(res, "count", "server response");
		uint64_t cid = JParser::getf(res, "id", "server response");
		if (ct == 0) {
			ci.type = CardInfo::BONUS_PERS;
		} else
		if (ct == 1) {
			ci.type = CardInfo::BONUS_ORG;
		} else
		if (ct == 5) {
			ci.type = CardInfo::SERVICE;
		} else {
			ci.type = CardInfo::UNKNOWN;
		}
		ci.id = cid;
		ci.count = cnt;
		card = ci;
	} catch (exception& e) {
		cout << "[INFO][BONUS] can't open transaction: " << e.what() << endl;
		return false;
	}
	_isOpen = true;
	return true;
}

double writeoff() {
	if (!_isOpen) {
		throw runtime_error("transaction is not open");
	}
	json res = _japi("writeoff " + to_string(_desiredWriteoff));
	return JParser::getf(res, "count", "server response");
}

void close(double acrue) {
	if (!_isOpen) {
		throw runtime_error("transaction is not open");
	}
	_japi("close " + to_string(acrue));
	_isOpen = false;
}

double getCoef() {
	vector<int> vap;
	for (int i = 0; i < _promotions.size(); i++) {
		if (_promotions[i].isActive()) {
			vap.push_back(i);
		}
	}
	if (vap.size() == 0) {
		cout << "[INFO][BONUS] no one promotions is active" << endl;
		_bonus = 100;
		return 1;
	} else
	if (vap.size() == 1) {
		cout << "one promotions is active, type: " << _promotions[vap[0]].type << ", k: " << _promotions[vap[0]].k << endl;
		_bonus = round(_promotions[vap[0]].k*100);
		return _promotions[vap[0]].k;
	} else {
		int k = 0;
		cout << vap.size() << " is active, them id: ";
		for (int i = 0; i < vap.size(); i++) {
			k += _promotions[vap[i]].sk;
			cout << _promotions[vap[i]].id << " ";
		}
		cout << ", summary k: " << k << endl;
		_bonus = round(k*100);
		return k;
	}
}

bool ismultibonus() {
	return _multibonus;
}

}