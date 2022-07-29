#include "render.h"
#include "../json.h"
#include "../jparser-linux/JParser.h"
#include "../logger-linux/Logger.h"
#include "../linux-ipc/ipc.h"

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
#include <fstream>

using namespace std;

namespace render {

/* * * * * * * * * * * * INIT BEGIN * * * * * * * * * * * */
namespace {
	Logger* _log;
	bool _state = false;
	char _rsock[256];
	char _render[256];
	thread* _thread_id;

	void _thread() {
		_state = true;
		system("sudo pkill render_ledmatrx");
		system(_render);
		_state = false;
		_log->log(Logger::Type::WARNING, "RENDER", "render server was crushed", 2);
		sleep(10);
	}
}

void init(json& display_cnf, Logger* log) {
	// get necessary config fields
	_log = log;
	memset(_rsock, 0, sizeof(_rsock));
	memset(_render, 0, sizeof(_render));
	string rsock = JParser::getf(display_cnf, "rsock", "display_cnf");
	string render = JParser::getf(display_cnf, "render", "display_cnf");
	strcpy(_rsock, rsock.c_str());
	strcpy(_render, render.c_str());

	_thread_id = new thread(_thread);
	sleep(2);
}

void showFrame(string name) {
	char buf[256] = {0};
	int rc = sprintf(buf, "f %s\n", name.c_str());
	rc = ipc_send(_rsock, buf, rc);
	if (rc != 0) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show frame '" + name + "'", 3);
	}
}

void showTempFrame(string name) {
	char buf[256] = {0};
	int rc = sprintf(buf, "t %s\n", name.c_str());
	rc = ipc_send(_rsock, buf, rc);
	if (rc != 0) {
		_log->log(Logger::Type::WARNING, "RENDER", "fail to show frame '" + name + "'", 3);
	}
}

void terminate() {
	char buf[8];
	buf[0] = 'e';
	buf[1] = '\n';
	buf[2] = 0;
	ipc_send(_rsock, buf, 2);
}

bool getState() {
	return _state;
}

}