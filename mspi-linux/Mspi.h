#pragma once

#include <iostream>
#include <string>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <vector>
#include <atomic>
#include <mutex>

extern "C" {
#include "../general-tools/general_tools.h"
}

using namespace std;

class Mspi {
public:
    Mspi(string driver, int speedHz, int csPin, int intPin, void (*callback)(int reason));
    virtual ~Mspi();

    void write(int raddr, uint8_t* data, int nData);
    void read(int raddr, uint8_t* buffer, int nData);
    void cmd(int cmd, int arg);

	void registerCallback(int reason, void (*callback)());
	void enableInt();
	void disableInt();

private:
    enum class OB {
        WRITE = 0x01,
        READ = 0x02,
        CMD = 0x03,
		INTR = 0x04
    };
	enum class ACK {
		NACK = 0x00,
		ACK = 0x01,
		NACK_CRC = 0x02,
		INTF = 0x80
	};
	struct Callback {
		int reason;
		void (*function)();
	};

	const int timeOutMs = 100;

    int _fd;
	int _delay;
    struct spi_ioc_transfer _spi_tr;
    static int _csPin;
	static int _intPin;
	
	static void (*_callback)(int reason);
	static Mspi* _this;
	static vector<Callback> _regs;
	static atomic<bool> _handling;
	static mutex _mutex;
	static bool _needInt;
	static bool _enableInt;

	static void _interrupt();
	static ReturnCode _handle(uint16_t id, void* arg);

	void _write(uint8_t* data, int nData);
	void _read(uint8_t* buffer, int nData);

	int _checkInt();
    void _cs(int state);
	void _waitof(int intState, int timeOutMs);
    void _begin();
    void _sendPreambule(OB ob, int raddr, int nData);
    void _writeData(uint8_t* data, int nData);
    void _readData(uint8_t* buffer, int nData);
	int _getIntReason();
};

