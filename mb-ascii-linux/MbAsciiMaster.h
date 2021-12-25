/* Author: Jodzik */
#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <termios.h>

class MbAsciiMaster {
public:
	MbAsciiMaster(int dirPin, const char* driver = "/dev/tty0", int baudRate = B9600);
	virtual ~MbAsciiMaster();

	void cmd(int addr, int cmd, int timeout);
	void str(int addr, char* str, int timeout);
	void rwrite(int addr, int raddr, const int* regs, int nRegs, int timeout);
	void rread(int addr, int raddr, int* regs, int nRegs, int timeout);

private:
	int _fd;
	int _dir;

	int _receive(char* buffer, int timeout);
	uint8_t _extract8(char* buff, uint8_t b);
	uint16_t _extract16(char* buff, uint8_t b);
};


using namespace std;