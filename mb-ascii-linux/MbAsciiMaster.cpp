#include "MbAsciiMaster.h"
#include "../crc/crc.h"
#include "wiringPi.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <time.h>
#include <chrono>
#include <iostream>

using namespace std;

MbAsciiMaster::MbAsciiMaster(int dirPin, const char* driver, int baudRate, bool loopback) {
	_dir = dirPin;
	if (dirPin > 0) {
		pinMode(dirPin, OUTPUT);
		digitalWrite(_dir, LOW);
	}

	_fd = open (driver, O_RDWR | O_NOCTTY);
	if (_fd < 0) {
		throw runtime_error("fail to open driver");
	}
	_loopback = loopback;

	termios tio = {0};
	cfsetispeed(&tio, baudRate); 
	cfsetospeed(&tio, baudRate);
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |=  CS8;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CRTSCTS;
	tio.c_cflag |= CREAD | CLOCAL;
	tio.c_iflag &= ~(IXON | IXOFF | IXANY);
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tio.c_oflag &= ~OPOST;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_cflag &= ~(PARENB | CMSPAR | PARODD);
	tcsetattr(_fd, TCSANOW, &tio);
	tcflush(_fd, TCIFLUSH);
}

MbAsciiMaster::~MbAsciiMaster() {
	close(_fd);
}

void MbAsciiMaster::cmd(int addr, int cmd, int timeout) {
	char buff[512] = {0};
	uint8_t lrcb[5];
	lrcb[0] = 0;
	lrcb[1] = 0;
	lrcb[2] = (uint8_t)(cmd >> 8);
	lrcb[3] = (uint8_t)(cmd);
	sprintf(buff, ":%02X%02X%04X%04X%02X\r\n", addr, 0x06, 0, cmd, lrc8(lrcb, 4));
	if (_dir > 0) {
		digitalWrite(_dir, HIGH);
	}
	
	write(_fd, buff, 17);
	tcdrain(_fd);
	usleep(1100);
	int n = _receive(buff, timeout);
	if (_extract8(buff, 2) != 0x06) {
		throw runtime_error("error response: " + to_string(_extract8(buff, 4)) + ", func: " + to_string(_extract8(buff, 2)));
	}
}

void MbAsciiMaster::str(int addr, char* str, int timeout) {
	char buff[512] = {0};
	sprintf(buff, ":%02X%02X%s%02X\r\n", addr, 0x42, str, lrc8((uint8_t*)str, strlen(str)));
	if (_dir > 0) {
		digitalWrite(_dir, HIGH);
	}
	write(_fd, buff, 9 + strlen(str));
	tcdrain(_fd);
	usleep(1100);
	int n = _receive(buff, timeout);
	if (_extract8(buff, 2) != 0x42) {
		throw runtime_error("error response: " + to_string(_extract8(buff, 4)) + ", func: " + to_string(_extract8(buff, 2)));
	}
	for (int i = 0; i < n - 4; i++) {
		str[i] = buff[i + 4];
	}
}

void MbAsciiMaster::rwrite(int addr, int raddr, const int* regs, int nRegs, int timeout) {
	char buff[512] = {0};
	uint8_t lrcb[256];
	lrcb[0] = (uint8_t)(raddr >> 8);
	lrcb[1] = (uint8_t)(raddr);
	lrcb[2] = (uint8_t)(nRegs >> 8);
	lrcb[3] = (uint8_t)(nRegs);
	lrcb[4] = nRegs * 2;
	for (int i = 0; i < nRegs; i++) {
		lrcb[5 + i*2] = (regs[i] >> 8) & 0xFF;
		lrcb[6 + i*2] = regs[i] & 0xFF;
	}
	sprintf(buff, ":%02X%02X%04X%04X%02X", addr, 0x10, raddr, nRegs, nRegs * 2);
	for (int i = 0; i < nRegs; i++) {
		sprintf(&buff[15 + i*4], "%04X", regs[i]);
	}
	sprintf(&buff[15 + nRegs*4], "%02X\r\n\0", lrc8(lrcb, 5 + nRegs*2));

	// for (int i = 0; i < 5 + nRegs*2; i++) {
	// 	printf("%02X ", lrcb[i]);
	// }
	// cout << endl << "buf: " << buff << endl;
	
	if (_dir > 0) {
		digitalWrite(_dir, HIGH);
	}
	write(_fd, buff, 19 + nRegs*4);
	tcdrain(_fd);
	usleep(1100);
	int n = _receive(buff, timeout);
	if (_extract8(buff, 2) != 0x10) {
		throw runtime_error("error response: " + to_string(_extract8(buff, 4)) + ", func: " + to_string(_extract8(buff, 2)));
	}
}

void MbAsciiMaster::rread(int addr, int raddr, int* regs, int nRegs, int timeout) {
	char buff[512] = {0};
	uint8_t lrcb[5];
	lrcb[0] = (uint8_t)(raddr >> 8);
	lrcb[1] = (uint8_t)(raddr);
	lrcb[2] = (uint8_t)(nRegs >> 8);
	lrcb[3] = (uint8_t)(nRegs);
	sprintf(buff, ":%02X%02X%04X%04X%02X\r\n", addr, 0x03, raddr, nRegs, lrc8(lrcb, 4));
	if (_dir > 0) {
		digitalWrite(_dir, HIGH);
	}
	while (read(_fd, buff, sizeof(buff)) > 0) {}
	write(_fd, buff, 17);
	tcdrain(_fd);
	usleep(1100);
	int n = _receive(buff, timeout);
	if (_extract8(buff, 2) != 0x03) {
		throw runtime_error("error response: " + to_string(_extract8(buff, 4)) + ", func: " + to_string(_extract8(buff, 2)));
	}
	if (n != 6 + nRegs * 4) {
		buff[n + 2] = 0;
		throw runtime_error("incorrect response, n: " + to_string(n) + ",buff: " + string(buff));
	}
	for (int i = 0; i < nRegs; i++) {
		regs[i] = _extract16(buff, 6 + i*4);
	}
}

int MbAsciiMaster::_receive(char* buff, int timeout) {
	char c;
	if (_dir > 0) {
		digitalWrite(_dir, LOW);
	}

	// wait entry
	bool first = true;
	auto tla = chrono::steady_clock::now();
	while (true) {
		if (read(_fd, &c, 1) > 0) {
			// cout << c;
			tla = chrono::steady_clock::now();
			if (c == ':') {
				if (_loopback) {
					if (first) {
						first = false;
					} else {
						break;
					}
				} else {
					break;
				}
			}
		}
		auto d = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - tla);
		if (d.count() > timeout) {
			// cout << endl;
			throw runtime_error("timeout");
		}
	}

	// fill buffer
	int i = 0;
	while (true) {
		if (read(_fd, &c, 1) > 0) {
			// cout << c;
			tla = chrono::steady_clock::now();
			buff[i] = c;
			i++;
			if (c == '\n') {
				return i - 4;
			}
			if (i >= 512) {
				throw runtime_error("overflow");	
			}
		}
		auto d = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - tla);
		if (d.count() > timeout) {
			// cout << endl;
			throw runtime_error("timeout");
		}
	}
	// cout << endl;
}

uint8_t MbAsciiMaster::_extract8(char* buff, uint8_t b) {
	uint8_t tempb;
	uint16_t val;
	tempb = buff[b + 2];
	buff[b + 2] = 0;
	val = strtoul(&buff[b], NULL, 16);
	buff[b + 2] = tempb;
	return val;
}

uint16_t MbAsciiMaster::_extract16(char* buff, uint8_t b) {
	uint8_t tempb;
	uint16_t val;
	tempb = buff[b + 4];
	buff[b + 4] = 0;
	val = strtoul(&buff[b], NULL, 16);
	buff[b + 4] = tempb;
	return val;
}
