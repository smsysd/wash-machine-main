#include "Mspi.h"
extern "C" {
#include "../crc/crc.h"
#include "../general-tools/general_tools.h"
}

#include <wiringPi.h>

#include <iostream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdexcept>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <atomic>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

using namespace std;

Mspi* Mspi::_this;
vector<Mspi::Callback> Mspi::_regs;
atomic<bool> Mspi::_handling {false};
mutex Mspi::_mutex;
void (*Mspi::_callback)(int);
int Mspi::_csPin;
int Mspi::_intPin;
bool Mspi::_needInt = false;
bool Mspi::_enableInt = false;

void Mspi::_interrupt() {
	_mutex.lock();
	_handling.store(true, std::memory_order_relaxed);

	int reason;
	try {
		reason = _this->_getIntReason();
	} catch (exception& e) {
		_this->_mutex.unlock();
		cout << "fail get interrupt reason: " << e.what() << endl;
		return;
	}

	_handling.store(false, std::memory_order_relaxed);
	_mutex.unlock();

	if (_callback != NULL) {
		_callback(reason);
	}

	for (int i = 0; i < _regs.size(); i++) {
		if (_regs.at(i).reason == reason) {
			_regs.at(i).function();
			return;
		}
	}
}

ReturnCode Mspi::_handle(uint16_t id, void* arg) {
	if (_handling) {
		return OK;
	}
	if (!_enableInt) {
		return OK;
	}

	if (digitalRead(_intPin) == 0) {
		// cout << "mspi poll int" << endl;
		_needInt = true;
	}
	if (_needInt) {
		_needInt = false;
		_interrupt();
	}

	return OK;
}

void Mspi::registerCallback(int reason, void (*callback)()) {
	Callback cb = {0};
	cb.reason = reason;
	cb.function = callback;
	_regs.push_back(cb);
}

Mspi::Mspi(string driver, int speedHz, int csPin, int intPin, void (*callback)(int reason)) {
    memset(&_spi_tr, 0, sizeof(struct spi_ioc_transfer));
	
    _fd = open(driver.c_str(), O_RDWR);
    if(_fd == -1) {
        throw runtime_error("fail to open driver: " + to_string(_fd));
    }

	uint8_t mode = SPI_MODE_0;
    if(ioctl(_fd, SPI_IOC_WR_MODE, &mode) == -1) {
		throw runtime_error("fail to configure driver: mode");
    }

	uint8_t bits_per_word = 8;
	if(ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) == -1){
		throw runtime_error("fail to configure driver: word");
    }
    _spi_tr.bits_per_word = bits_per_word;

	uint32_t max_speed_hz = speedHz;
	if(ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed_hz) == -1) {
		throw runtime_error("fail to configure driver: speed");
    }
    _spi_tr.speed_hz = max_speed_hz;
	_delay = 1000000 / speedHz;

	_csPin = csPin;
	_intPin = intPin;
	_callback = callback;

	pinMode(_csPin, OUTPUT);
	digitalWrite(_csPin, 1);
	pinMode(_intPin, INPUT);
	pullUpDnControl(_intPin, PUD_UP);

	_this = this;

	int rc = callHandler(&Mspi::_handle, NULL, 10, 0);
	if (rc < 0) {
		throw runtime_error("fail to create handle: " + to_string(rc));
	}
}

Mspi::~Mspi() {
    close(_fd);
}

void Mspi::write(int raddr, uint8_t* data, int nData) {
	try {
		_mutex.lock();
		_handling.store(true, std::memory_order_relaxed);
		_begin();
		_sendPreambule(OB::WRITE, raddr, nData);
		_waitof(0, timeOutMs);
		_writeData(data, nData);
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
	} catch(exception& e) {
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
		throw runtime_error("fail mspi write: " + string(e.what()));
	}
}

void Mspi::read(int raddr, uint8_t* buffer, int nData) {
	try {
		_mutex.lock();
		_handling.store(true, std::memory_order_relaxed);
		_begin();
		_sendPreambule(OB::READ, raddr, nData);
		_waitof(0, timeOutMs);
		_readData(buffer, nData);
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
	} catch(exception& e) {
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
		throw runtime_error("fail mspi read: " + string(e.what()));
	}
}

void Mspi::cmd(int cmd, int arg) {
	try {
		_mutex.lock();
		_handling.store(true, std::memory_order_relaxed);
		_begin();
		_sendPreambule(OB::CMD, cmd, arg);
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
	} catch(exception& e) {
		_handling.store(false, std::memory_order_relaxed);
		_mutex.unlock();
		throw runtime_error("fail send mspi cmd: " + string(e.what()));
	}
}

void Mspi::enableInt() {
	_enableInt = true;
}

void Mspi::disableInt() {
	_enableInt = false;
}

void Mspi::setTimeout(int timeoutMs) {
	timeOutMs = timeoutMs;
}

int Mspi::_getIntReason() {
	uint8_t data[6] = {0};
	data[0] = (uint8_t)OB::INTR;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = crc8(data, 5);
	
	try {
		_begin();
		_write(data, 6);
		_waitof(1, timeOutMs);
		_read(data, 1);
	} catch (exception& e) {
		throw runtime_error("fail get interrupt reason: " + string(e.what()));
	}

	return data[0];
}

void Mspi::_write(uint8_t* data, int nData) {
    _spi_tr.tx_buf = (uintptr_t)data;
    _spi_tr.rx_buf = (uintptr_t)NULL;
    _spi_tr.len    = nData;

    int rc = ioctl(_fd, SPI_IOC_MESSAGE(1), &_spi_tr);
	// usleep(nData*8*_delay);
    if(rc != nData) {
        throw runtime_error("fail to write: " + to_string(rc));
    }
}

void Mspi::_read(uint8_t* buffer, int nData) {
    _spi_tr.tx_buf = (uintptr_t)NULL;
    _spi_tr.rx_buf = (uintptr_t)buffer;
    _spi_tr.len    = nData;

    int rc = ioctl(_fd, SPI_IOC_MESSAGE(1), &_spi_tr);
	// usleep(nData*8*_delay);
    if(rc < nData) {
		throw runtime_error("fail to read: " + to_string(rc));
    }
}

void Mspi::_waitof(int intState, int timeOutMs) {
	int i = 0;
	while(_checkInt() != intState) {
		usleep(100);
		i++;
		if (i > timeOutMs * 10) {
			throw runtime_error("sync timeout");
		}
	}
}

int Mspi::_checkInt() {
	static int old = 1;

	int dempf0 = 0;
	int dempf1 = 0;
	int val;
	
	for (int i = 0; i < 7; i++) {
		val = digitalRead(_intPin);
		if (val == 0) {
			dempf0++;
			dempf1 = 0;
		}
		if (val == 1) {
			dempf0 = 0;
			dempf1++;
		}
		if (dempf0 > 5) {
			old = 0;
			return 0;
		}
		if (dempf1 > 5) {
			old = 1;
			return 1;
		}
	}

	return old;
}

void Mspi::_cs(int state) {
	digitalWrite(_csPin, state > 0 ? 1 : 0);
}

void Mspi::_begin() {
	try {
		if (_checkInt()) {
			_cs(0);
			_waitof(0, timeOutMs);
			_cs(1);
		} else {
			_cs(0);
			_waitof(1, timeOutMs);
			_cs(1);
			_waitof(0, timeOutMs);
		}
	} catch (exception& e) {
		_cs(1);
		throw runtime_error("failed begin: " + string(e.what()));
	}
}

void Mspi::_sendPreambule(OB ob, int raddr, int nData) {
	uint8_t data[6] = {0};
	data[0] = (uint8_t)ob;
	data[1] = (uint8_t)(raddr >> 8);
	data[2] = (uint8_t)(raddr);
	data[3] = (uint8_t)(nData >> 8);
	data[4] = (uint8_t)(nData);
	data[5] = crc8(data, 5);
	_write(data, 6);
	try {
		_waitof(1, timeOutMs);
	} catch (exception& e) {
		throw runtime_error("fail send preambule: " + string(e.what()));
	}
	_read(data, 1);
	if (data[0] & (uint8_t)ACK::INTF) {
		// cout << "mspi logic int" << endl;
		_needInt = true;
	}
	data[0] &= ~((uint8_t)ACK::INTF);
	if (data[0] != (uint8_t)ACK::ACK) {
		throw runtime_error("incorrect request, NACK: " + to_string(data[0]));
	}
}

void Mspi::_writeData(uint8_t* data, int nData) {
	uint8_t ack = 0;
	data[nData] = crc8(data, nData);
	try {
		_write(data, nData + 1);
		_waitof(1, timeOutMs);
		_read(&ack, 1);
	} catch (exception& e) {
		throw runtime_error("fail to write data: " + string(e.what()));
	}
	if (ack != (uint8_t)ACK::ACK) {
		throw runtime_error("fail to write data, NACK: " + to_string(ack));
	}
}

void Mspi::_readData(uint8_t* buffer, int nData) {
	uint8_t ack = 0;

	try {
		_read(buffer, nData + 1);
		_waitof(1, timeOutMs);
	} catch (exception& e) {
		throw runtime_error("fail to read data: " + string(e.what()));
	}
	
	if (buffer[nData] != crc8(buffer, nData)) {
		ack = (uint8_t)ACK::NACK_CRC;
		_write(&ack, 1);
		throw runtime_error("fail to read data: CRC");
	} else {
		ack = (uint8_t)ACK::ACK;
		_write(&ack, 1);
	}
}
