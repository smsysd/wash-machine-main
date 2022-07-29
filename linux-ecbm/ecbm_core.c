#include "ecbm_core.h"

#include "../crc/crc.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#if ECBM_DEBUG
#include <stdio.h>
#endif

#define _ECBM_ASSERT_D(d)		if (d >= ECBM_PORTS || d < 0) {return ECBM_RC_INC_D;} if (!_ecbm_ports[d].usef) {return ECBM_RC_UNUSED;}
#define _ECBM_LOCK(d)			while (_ecbm_ports[d].lock) {_ecbm_ports[d].yeld();} _ecbm_ports[d].lock = 1;
#define _ECBM_UNLOCK(d)			_ecbm_ports[d].lock = 0;

#define _ECBM_RXCTL_NORX				0
#define _ECBM_RXCTL_COMPLETE			1
#define _ECBM_RXCTL_PROC				2

#define _ECBM_SCAN_NOTHING				0
#define _ECBM_SCAN_OK					1
#define _ECBM_SCAN_COLLISION			2

#define _ECBM_COLNDT_INTGR_ERRW			1
#define _ECBM_COLNDT_TIMET_ERRW			1
#define _ECBM_COLNDT_TIMET_MS			50
#define _ECBM_INTRUS_INTGR_ERRW			3
#define _ECBM_INTRUS_TIMET_ERRW			3
#define _ECBM_INTRUS_TIMET_MS			500
#define _ECBM_RELAXE_INTGR_ERRW			1
#define _ECBM_RELAXE_TIMET_ERRW			1
#define _ECBM_RELAXE_TIMET_MS			100
#define _ECBM_SCAN_INTGR_ERRW			2
#define _ECBM_SCAN_TIMET_ERRW			1
#define _ECBM_SCAN_TIMET_MS				50

#define _ECBM_POLL_PERIOD_BASE			100
#define _ECBM_POLL_PERIOD_DIV			50

// MARK TYPE:
// 0x00 PACKET BEGIN:
// 0x00 READ = 0b10000000
// 0x01 WRITE = 0b10001001
// 0x02 ANSWER = 0b10010010
// 0x03 WPOLL = 0b10011011
// 0x01 PACKET END
// = 0b11000010

#define _ECBM_MARK_BEGIN_ANSWER			0b10010010
#define _ECBM_MARK_BEGIN_WRITE			0b10001001
#define _ECBM_MARK_BEGIN_READ			0b10000000
#define _ECBM_MARK_BEGIN_WPOLL			0b10011011
#define _ECBM_MARK_END					0b11000010

#define _ECBM_POLYNOME					0x04C11DB7

#define _ECBM_UDATA_INDEX_IN_ANSWER		5
#define _ECBM_NSRV_DATA_IN_ANSWER		8

#define _ECBM_SIG_RESET				0
#define _ECBM_SIG_ADDR				1
#define _ECBM_SIG_RAND_ADDR			2
#define _ECBM_SIG_REMAP_ADDR		3
#define _ECBM_SIG_INFO				4
#define _ECBM_SIG_RAND				7
#define _ECBM_SIG_INDICATE			8
#define _ECBM_SIG_STATUS			9
#define _ECBM_SIG_SAVE				10
#define _ECBM_SIG_EVENT				11

Ecbm _ecbm_ports[ECBM_PORTS] = {0};

static void _timespec_diff(const struct timespec* start, const struct timespec* stop, struct timespec* result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

static int _timespec2ms(const struct timespec* ts) {
	int ms = ts->tv_sec*1000;
	ms += ts->tv_nsec / 1000000;
	return ms;
}

/* * * Put full data with mark bytes, return new data size
 * ndata must be include full data with mark bytes
 * * */
static int _ecbm_encode(uint8_t* data, int ndata) {
	if (ndata < 2) {
		return -1;
	}
	int nrData = ndata - 2; // Without mark bytes
	int nDataAdd = nrData % 7 == 0 ? nrData / 7 : nrData / 7 + 1;
	int iDataAdd = ndata - 1;
	data[ndata-1+nDataAdd] = data[ndata-1]; // Move END MARK to end
	memset(&data[iDataAdd], 0, nDataAdd);
	for (int i = 0; i < nrData; i++) {
		if (data[i+1] & 0x80) {
			data[iDataAdd + i/7] |= 1 << (i % 7);
			data[i+1] &= ~0x80;
		}
	}
	return nDataAdd + ndata;
}

/* * * Put data without mark bytes, return new data size
 * ndata must be include full data with mark bytes
 * * */
static int _ecbm_decode(uint8_t* data, int ndata) {
	if (ndata < 4) {
		return 0;
	}
	int nDataAdd = ndata % 8 == 0 ? ndata / 8 : ndata / 8 + 1;
	int nrData = ndata - nDataAdd; // Without add bytes
	
	for (int i = 0; i < nrData; i++) {
		if (data[nrData + i/7] & (1 << (i % 7))) {
			data[i] |= 0x80;
		}
	}
	return nrData;	
}

static void _ecbm_delay(int d, int ms) {
	struct timespec start;
	struct timespec now;
	struct timespec diff;

	_ecbm_ports[d].get_time(&start);
	while (1) {
		_ecbm_ports[d].yeld();
		_ecbm_ports[d].get_time(&now);
		_timespec_diff(&start, &now, &diff);
		if (_timespec2ms(&diff) >= ms) {
			return;
		}
	}
}

static int _ecbm_make_base(uint8_t* buf, int begin, int addr, int sig, const uint8_t* data, int ndata) {
	buf[0] = begin;
	buf[1] = addr;
	buf[2] = (uint8_t)(sig >> 8);
	buf[3] = (uint8_t)sig;
	if (data != NULL) {
		memcpy(&buf[4], data, ndata);
	} else {
		ndata = 0;
	}
	uint32_t crc = crc32(&buf[1], 3 + ndata, _ECBM_POLYNOME);
	buf[4+ndata] = (uint8_t)(crc >> 24);
	buf[5+ndata] = (uint8_t)(crc >> 16);
	buf[6+ndata] = (uint8_t)(crc >> 8);
	buf[7+ndata] = (uint8_t)(crc);
	buf[8+ndata] = _ECBM_MARK_END;

#if ECBM_DEBUG
		printf("MDATA: ");
		for (int i = 0; i < ndata+9; i++) {
			printf("%02X ", buf[i]);
		}
		printf("\n");
#endif

	return _ecbm_encode(buf, ndata+9);
}

static int _ecbm_assert(uint8_t* buf, int ndata, int addr, int sig) {
	uint32_t crcr = 0;
	uint32_t crcc;
	// Check CRC
	crcr += buf[ndata-4] << 24;
	crcr += buf[ndata-3] << 16;
	crcr += buf[ndata-2] << 8;
	crcr += buf[ndata-1];
	crcc = crc32(buf, ndata-4, _ECBM_POLYNOME);
	if (crcc != crcr) {
#if ECBM_DEBUG
		printf("ASRT FAIL: CRC\n");
#endif
		return ECBM_RC_INTEGRITY;
	}

	// Check addr and sig
	if (buf[0] != addr || ((buf[1] << 8) + buf[2]) != sig) {
#if ECBM_DEBUG
		printf("ASRT FAIL: addr or sig\n");
#endif
		return ECBM_RC_INTEGRITY;
	}

	// Return RC
	return -buf[3];
}

void _ecbmc_onrx(int d, uint8_t b) {
	if (d < 0 || d > ECBM_PORTS || !_ecbm_ports[d].usef || _ecbm_ports[d].rxctl != _ECBM_RXCTL_PROC) {
		return;
	}
	_ecbm_ports[d].rxf = 1;

	if (_ecbm_ports[d].rxptr >= _ECBM_BUF_SIZE) {
		_ecbm_ports[d].rxctl = _ECBM_RXCTL_NORX;
		return;
	}

	if (_ecbm_ports[d].rxptr == 0) {
		if (b == _ECBM_MARK_BEGIN_ANSWER ||
			b == _ECBM_MARK_BEGIN_WRITE  ||
			b == _ECBM_MARK_BEGIN_READ 	 ||
			b == _ECBM_MARK_BEGIN_WPOLL)
		{
			_ecbm_ports[d].buf[0] = b;
			_ecbm_ports[d].rxptr++;
		}
	} else {
		_ecbm_ports[d].buf[_ecbm_ports[d].rxptr] = b;
		_ecbm_ports[d].rxptr++;
		if (b == _ECBM_MARK_END) {
			_ecbm_ports[d].rxctl = _ECBM_RXCTL_COMPLETE;
		}
	}
}

static int _ecbm_receive(int d, uint8_t* buf, int timeout_ms) {
	int rxptr_old;
	int rc;
	struct timespec tlrx;
	struct timespec now;
	struct timespec diff;

	_ecbm_ports[d].rxptr = 0;
	rxptr_old = _ecbm_ports[d].rxptr;
	_ecbm_ports[d].rxctl = _ECBM_RXCTL_PROC;
	_ecbm_ports[d].rxf = 0;
	_ecbm_ports[d].get_time(&tlrx);
	
	while (1) {
		if (_ecbm_ports[d].rxctl == _ECBM_RXCTL_NORX) {
#if ECBM_DEBUG
			printf("RCV FAIL: rxctl\n");
#endif
			return ECBM_RC_INTEGRITY;
		} else
		if (_ecbm_ports[d].rxctl == _ECBM_RXCTL_COMPLETE) {
			_ecbm_ports[d].rxctl = _ECBM_RXCTL_NORX;
			return _ecbm_ports[d].rxptr;
		} else {
			// update tlrx on soft trigger on rxptr increase
			if (_ecbm_ports[d].rxptr > rxptr_old) {
				rxptr_old = _ecbm_ports[d].rxptr;
				_ecbm_ports[d].get_time(&tlrx);
			}
			_ecbm_ports[d].get_time(&now);
			_timespec_diff(&tlrx, &now, &diff);
			if (_timespec2ms(&diff) > _ecbm_ports[d].timeout_ms) {
				_ecbm_ports[d].rxctl = _ECBM_RXCTL_NORX;
#if ECBM_DEBUG
				printf("RCV FAIL: timeout\n");
#endif
				if (_ecbm_ports[d].rxf) {
					return ECBM_RC_INTEGRITY;
				} else {
					return ECBM_RC_TIMEOUT;
				}
			}
		}
		_ecbm_ports[d].yeld();
	}
}

static int _ecbm_perform(int d, int addr, int sig, const uint8_t* data, int ndata, int begin) {
	int timeouts = 0;
	int integrs = 0;
	int rc;
	int ndec_data;
	_ecbm_ports[d].usr_data = NULL;
	_ecbm_ports[d].nusr_data = 0;
	while (1) {
		if (timeouts >= _ecbm_ports[d].timeout_errw) {
			return ECBM_RC_TIMEOUT;
		}
		if (integrs >= _ecbm_ports[d].integrity_errw) {
			return ECBM_RC_INTEGRITY;
		}
		if (integrs + timeouts > 0) {
			_ecbm_ports[d].yeld();
		}
		// Construct packet in port buffer
		rc = _ecbm_make_base(_ecbm_ports[d].buf, begin, addr, sig, data, ndata); 
		if (rc < 0) {
			return rc;
		}
		// Send requset, try receive answer
		_ecbm_ports[d].write(d, _ecbm_ports[d].buf, rc);
#if ECBM_DEBUG
		printf("SEND : ");
		for (int i = 0; i < rc; i++) {
			printf("%02X ", _ecbm_ports[d].buf[i]);
		}
		printf("\n");
#endif
		if (addr == ECBM_BROADCAST) {
			return ECBM_RC_OK;
		}
		rc = _ecbm_receive(d, _ecbm_ports[d].buf, _ecbm_ports[d].timeout_ms);
#if ECBM_DEBUG
		printf("RECEV: ");
		for (int i = 0; i < _ecbm_ports[d].rxptr; i++) {
			printf("%02X ", _ecbm_ports[d].buf[i]);
		}
		printf("\n");
#endif
		// Check RC
		if (rc == ECBM_RC_TIMEOUT) {
			timeouts++;
			continue;
		} else
		if (rc == ECBM_RC_INTEGRITY) {
			integrs++;
			continue;
		} else
		if (rc <= 0) {
			return ECBM_RC_BUG;
		}
		// Check begin mark
		if (_ecbm_ports[d].buf[0] != _ECBM_MARK_BEGIN_ANSWER) {
			integrs++;
			continue;
		}
		// Decode
		rc = _ecbm_decode(&_ecbm_ports[d].buf[1], rc-2);
		if (rc < 0) {
			integrs++;
			continue;
		}
		ndec_data = rc;
#if ECBM_DEBUG
		printf("RDATA: ");
		for (int i = 0; i < rc; i++) {
			printf("%02X ", _ecbm_ports[d].buf[i+1]);
		}
		printf("\n");
#endif
		// Assert packet
		// If RC == INTEGRITY_APP it is as default integrity error, other app code return to up level immediately
		rc = _ecbm_assert(&_ecbm_ports[d].buf[1], ndec_data, addr, sig);
		if (rc == ECBM_RC_INTEGRITY_APP || rc == ECBM_RC_INTEGRITY) {
			integrs++;
			continue;
		} else {
			// This is only APP codes handle
			// If user data exists - set pointer and len to glob
			if (ndec_data - _ECBM_NSRV_DATA_IN_ANSWER > 0) {
				_ecbm_ports[d].usr_data = &_ecbm_ports[d].buf[_ECBM_UDATA_INDEX_IN_ANSWER];
				_ecbm_ports[d].nusr_data = ndec_data - _ECBM_NSRV_DATA_IN_ANSWER;
			}
			return rc;
		}
	}
}

static int _ecbm_set_mode(int d, EcbmMode mode) {
	_ECBM_ASSERT_D(d)
	switch (mode) {
	case ECBM_MODE_COLNDT:
		_ecbm_ports[d].timeout_ms = _ECBM_COLNDT_TIMET_MS;
		_ecbm_ports[d].timeout_errw = _ECBM_COLNDT_TIMET_ERRW;
		_ecbm_ports[d].integrity_errw = _ECBM_COLNDT_INTGR_ERRW;
		break;
	case ECBM_MODE_SCAN:
		_ecbm_ports[d].timeout_ms = _ECBM_SCAN_TIMET_MS;
		_ecbm_ports[d].timeout_errw = _ECBM_SCAN_TIMET_ERRW;
		_ecbm_ports[d].integrity_errw = _ECBM_SCAN_INTGR_ERRW;
		break;
	case ECBM_MODE_INTRUS:
		_ecbm_ports[d].timeout_ms = _ECBM_INTRUS_TIMET_MS;
		_ecbm_ports[d].timeout_errw = _ECBM_INTRUS_TIMET_ERRW;
		_ecbm_ports[d].integrity_errw = _ECBM_INTRUS_INTGR_ERRW;
		break;
	case ECBM_MODE_RELAXE:
		_ecbm_ports[d].timeout_ms = _ECBM_RELAXE_TIMET_MS;
		_ecbm_ports[d].timeout_errw = _ECBM_RELAXE_TIMET_ERRW;
		_ecbm_ports[d].integrity_errw = _ECBM_RELAXE_INTGR_ERRW;
		break;				
	default: return ECBM_RC_INC_ARG;
	}

	_ecbm_ports[d].mode = mode;
	return ECBM_RC_OK;
}

int ecbm_set_mode(int d, EcbmMode mode) {
	_ECBM_LOCK(d)
	int rc = _ecbm_set_mode(d, mode);
	_ECBM_UNLOCK(d)
	return rc;
}

EcbmMode ecbm_get_mode(int d) {
	_ECBM_ASSERT_D(d)
	return _ecbm_ports[d].mode;
}

static char* _ecbm_get_errmsg(int d, int* len) {
	if (d < 0 || d > ECBM_PORTS || !_ecbm_ports[d].usef) {
		*len = 0;
		return NULL;
	}

	*len = _ecbm_ports[d].nusr_data;
	return (char*)_ecbm_ports[d].usr_data;
}

char* ecbm_get_errmsg(int d, int* len) {
	_ECBM_LOCK(d)
	char* rc = _ecbm_get_errmsg(d, len);
	_ECBM_UNLOCK(d)
	return rc;
}

int _ecbmc_init(int d,
	void (*write)(int d, const uint8_t* data, int ndata),
	void (*send_break)(int d),
	void (*yeld)(),
	void (*get_time)(struct timespec* now))
{
	if (d < 0 || d >= ECBM_PORTS) {
		return ECBM_RC_INC_D;
	}
	_ecbm_ports[d].write = write;
	_ecbm_ports[d].send_break = send_break;
	_ecbm_ports[d].yeld = yeld;
	_ecbm_ports[d].get_time = get_time;
	_ecbm_ports[d].mode = ECBM_DEF_MODE;
	_ecbm_ports[d].nusr_data = 0;
	_ecbm_ports[d].usr_data = NULL;
	_ecbm_ports[d].rxctl = _ECBM_RXCTL_NORX;
	_ecbm_ports[d].rxf = 0;
	_ecbm_ports[d].rxptr = 0;
	_ecbm_ports[d].lock = 0;
	_ecbm_ports[d].usef = 1;
	_ecbm_set_mode(d, _ecbm_ports[d].mode);

	return ECBM_RC_OK;
}

static int _ecbm_write(int d, int addr, int sig, const uint8_t* data, int ndata) {
	_ECBM_ASSERT_D(d)
	if (ndata > ECBM_BUF_SIZE) {
		return ECBM_RC_OVERFLOW;
	}
	return _ecbm_perform(d, addr, sig, data, ndata, _ECBM_MARK_BEGIN_WRITE);
}

int ecbm_write(int d, int addr, int sig, const uint8_t* data, int ndata) {
	_ECBM_LOCK(d)
	int rc = _ecbm_write(d, addr, sig, data, ndata); 
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_read(int d, int addr, int sig, uint8_t* buf, int buf_size) {
	_ECBM_ASSERT_D(d)
	int nwdata;
	int rc = _ecbm_perform(d, addr, sig, NULL, 0, _ECBM_MARK_BEGIN_READ);
	if (rc == ECBM_RC_OK) {
		if (_ecbm_ports[d].nusr_data > buf_size) {
			nwdata = buf_size;
		} else {
			nwdata = _ecbm_ports[d].nusr_data;
		}
		for (int i = 0; i < nwdata; i++) {
			buf[i] = _ecbm_ports[d].usr_data[i];
		}
		return nwdata;
	} else {
		return rc;
	}
}

int ecbm_read(int d, int addr, int sig, uint8_t* buf, int buf_size) {
	_ECBM_LOCK(d)
	int rc = _ecbm_read(d, addr, sig, buf, buf_size);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_wpoll(int d, int addr, int sig) {
	_ECBM_ASSERT_D(d)
	return _ecbm_perform(d, addr, sig, NULL, 0, _ECBM_MARK_BEGIN_WPOLL);
}

int ecbm_wpoll(int d, int addr, int sig) {
	_ECBM_LOCK(d)
	int rc = _ecbm_wpoll(d, addr, sig);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_writew(int d, int addr, int sig, const uint8_t* data, int ndata, int timeout_ms) {
	struct timespec start;
	struct timespec now;
	struct timespec diff;

	int rc = _ecbm_write(d, addr, sig, data, ndata);
	if (rc == ECBM_RC_OKC) {
		_ecbm_ports[d].get_time(&start);
		while (1) {
			_ecbm_delay(d, _ECBM_POLL_PERIOD_BASE + timeout_ms/_ECBM_POLL_PERIOD_DIV);
			rc = _ecbm_wpoll(d, addr, sig);
			if (rc != ECBM_RC_OKC) {
				return rc;
			}
			_ecbm_ports[d].get_time(&now);
			_timespec_diff(&start, &now, &diff);
			if (_timespec2ms(&diff) > timeout_ms) {
				return ECBM_RC_TIMEOUT;
			}
		}
	} else {
		return rc;
	}
}

int ecbm_writew(int d, int addr, int sig, const uint8_t* data, int ndata, int timeout_ms) {
	_ECBM_LOCK(d)
	int rc = _ecbm_writew(d, addr, sig, data, ndata, timeout_ms);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_readw(int d, int addr, int sig, uint8_t* buf, int buf_size, int timeout_ms) {
	struct timespec start;
	struct timespec now;
	struct timespec diff;
	int rc;

	_ecbm_ports[d].get_time(&start);
	while (1) {
		rc = _ecbm_read(d, addr, sig, buf, buf_size);
		if (rc != ECBM_RC_OKC) {
			return rc;
		}
		_ecbm_delay(d, _ECBM_POLL_PERIOD_BASE + timeout_ms/_ECBM_POLL_PERIOD_DIV);
		_ecbm_ports[d].get_time(&now);
		_timespec_diff(&start, &now, &diff);
		if (_timespec2ms(&diff) > timeout_ms) {
			return ECBM_RC_TIMEOUT;
		}
	}
}

int ecbm_readw(int d, int addr, int sig, uint8_t* buf, int buf_size, int timeout_ms) {
	_ECBM_LOCK(d)
	int rc = _ecbm_readw(d, addr, sig, buf, buf_size, timeout_ms);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_status(int d, int addr, int* status, char descr[64]) {
	uint8_t buf[65] = {0};
	int rc = _ecbm_read(d, addr, _ECBM_SIG_STATUS, buf, sizeof(buf));
	if (rc > 0) {
		if (rc >= 1) {
			*status = (int8_t)buf[0];
			memset(descr, 0, 64);
			strncpy(descr, (char*)&buf[1], rc-1);
			return ECBM_RC_OK;
		} else {
			return ECBM_RC_INC_ANSWER;
		}
	} else {
		return rc;
	}
}

int ecbm_status(int d, int addr, int* status, char descr[64]) {
	_ECBM_LOCK(d)
	int rc = _ecbm_status(d, addr, status, descr);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_info(int d, int addr, EcbmInfo* info) {
	uint8_t buf[256+4+2] = {0};

	int rc = _ecbm_read(d, addr, _ECBM_SIG_INFO, buf, sizeof(buf));
	if (rc > 0) {
		if (rc >= 6) {
			info->addr = addr;
			info->wia = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3]); 
			info->version = (buf[4] << 8) + buf[5];
			memset(info->descr, 0, sizeof(info->descr));
			strncpy(info->descr, &buf[6], rc-6);
			rc = _ecbm_status(d, addr, &info->status, info->status_descr);
			return rc;
		} else {
			return ECBM_RC_INC_ANSWER;
		}
	} else {
		return rc;
	}
}

int ecbm_info(int d, int addr, EcbmInfo* info) {
	_ECBM_LOCK(d)
	int rc = _ecbm_info(d, addr, info);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_get_event(int d, int addr, int* event) {
	uint8_t buf[2];
	int rc = _ecbm_read(d, addr, _ECBM_SIG_EVENT, buf, 2);
	if (rc > 0) {
		if (rc == 2) {
			*event = (buf[0] << 8) + buf[1];
			return ECBM_RC_OK;
		} else {
			return ECBM_RC_INC_ANSWER;
		}
	} else {
		return rc;
	}
}

int ecbm_get_event(int d, int addr, int* event) {
	_ECBM_LOCK(d)
	int rc = _ecbm_get_event(d, addr, event);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_rand(int d, int addr) {
	uint8_t buf[4];
	int temp;
	int rc = _ecbm_read(d, addr, _ECBM_SIG_RAND, buf, sizeof(buf));
	if (rc > 0) {
		if (rc == 4) {
			temp = buf[0] << 24;
			temp += buf[1] << 16;
			temp += buf[2] << 8;
			temp += buf[3];
			if (temp < 0) {
				return -temp;
			} else {
				return temp;
			}
		} else {
			return ECBM_RC_INC_ANSWER;
		}
	} else {
		return rc;
	}
}

static int _ecbm_rand_addr(int d, int addr, uint8_t* addrs, int naddrs) {
	return _ecbm_write(d, addr, _ECBM_SIG_RAND_ADDR, addrs, naddrs);
}

static int _ecbm_scan(int d, void (*onscan)(EcbmInfo* info)) {
	int rc;
	int cnt = 0;
	EcbmInfo info;
	EcbmMode mode = _ecbm_ports[d].mode;
	_ecbm_set_mode(d, ECBM_MODE_SCAN);
	for (int i = 1; i < 256; i++) {
		memset(&info, 0, sizeof(EcbmInfo));
		rc = _ecbm_info(d, i, &info);
		if (rc == ECBM_RC_OK) {
			cnt++;
			onscan(&info);
		}
	}
	_ecbm_set_mode(d, mode);
	
	return cnt;
}

int ecbm_scan(int d, void (*onscan)(EcbmInfo* info)) {
	_ECBM_LOCK(d)
	int rc = _ecbm_scan(d, onscan);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_scanex(int d, uint8_t empt[128], uint8_t exsts[256], int* nexsts, int* ncol, int* nempt) {
	int rc;
	int lcol_addr = 0;
	*ncol = 0;
	*nempt = 0;
	*nexsts = 0;
	EcbmMode mode = _ecbm_ports[d].mode;
	_ecbm_set_mode(d, ECBM_MODE_COLNDT);
	for (int i = 1; i < 256; i++) {
		rc = _ecbm_rand(d, i);
		if (rc == ECBM_RC_OK) {
			exsts[*nexsts] = i;
			*nexsts++;
		} else
		if (rc == ECBM_RC_TIMEOUT) {
			if (*nempt >= 128) {
				continue;
			}
			empt[*nempt] = i;
			*nempt++;
		} else {
			*ncol++;
			lcol_addr = i;
		}
	}
	_ecbm_set_mode(d, mode);

	return lcol_addr;
}

static int _ecbm_indicate(int d, int addr, int arg) {
	uint8_t buf[] = {(uint8_t)arg};
	return _ecbm_write(d, addr, _ECBM_SIG_INDICATE, buf, 1);
}

int ecbm_indicate(int d, int addr, int arg) {
	_ECBM_LOCK(d)
	int rc = _ecbm_indicate(d, addr, arg);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_save(int d, int addr, int arg) {
	uint8_t buf[] = {(uint8_t)arg};
	int timeout = _ecbm_ports[d].timeout_ms;
	_ecbm_ports[d].timeout_ms = ECBM_SAVE_TIMEOUT_MS;
	
	int rc = _ecbm_write(d, addr, _ECBM_SIG_SAVE, buf, 1);

	_ecbm_ports[d].timeout_ms = timeout;
	return rc;
}

int ecbm_save(int d, int addr, int arg) {
	_ECBM_LOCK(d)
	int rc = _ecbm_save(d, addr, arg);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_reset(int d, int addr) {
	int timeout = _ecbm_ports[d].timeout_ms;
	_ecbm_ports[d].timeout_ms = ECBM_RESET_TIMEOUT_MS;
	
	int rc = _ecbm_write(d, addr, _ECBM_SIG_RESET, NULL, 0);

	_ecbm_ports[d].timeout_ms = timeout;
	return rc;
}

int ecbm_reset(int d, int addr) {
	_ECBM_LOCK(d)
	int rc = _ecbm_reset(d, addr);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_resethw(int d) {
	_ecbm_ports[d].send_break(d);
	return ECBM_RC_OK;	
}

int ecbm_resethw(int d) {
	_ECBM_LOCK(d)
	int rc = _ecbm_resethw(d);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_set_addr(int d, int addr, int new_addr) {
	uint8_t buf[] = {(uint8_t)new_addr};
	return _ecbm_write(d, addr, _ECBM_SIG_ADDR, buf, 1);
}

int ecbm_set_addr(int d, int addr, int new_addr) {
	_ECBM_LOCK(d)
	int rc = _ecbm_set_addr(d, addr, new_addr);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_remap_addr(int d, uint8_t addrs[256]) {
	return _ecbm_write(d, ECBM_BROADCAST, _ECBM_SIG_REMAP_ADDR, addrs, 256);
}

int ecbm_remap_addr(int d, uint8_t addrs[256]) {
	_ECBM_LOCK(d)
	int rc = _ecbm_remap_addr(d, addrs);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_swap_addr(int d, uint8_t addr1, uint8_t addr2) {
	uint8_t buf[256] = {0};
	if (addr1 == addr2) {
		return ECBM_RC_INC_ARG;
	}
	buf[addr1] = addr2;
	buf[addr2] = addr1;
	return _ecbm_remap_addr(d, buf);
}

int ecbm_swap_addr(int d, uint8_t addr1, uint8_t addr2) {
	_ECBM_LOCK(d)
	int rc = _ecbm_swap_addr(d, addr1, addr2);
	_ECBM_UNLOCK(d)
	return rc;
}

static int _ecbm_redistrib_addr(int d, int tries) {
	uint8_t empt[256];
	uint8_t exsts[256];
	int ncol;
	int nempt;
	int nexsts;
	int lcol_addr;
	for (int i = 0; i < tries; i++) {
		lcol_addr = _ecbm_scanex(d, empt, exsts, &nexsts, &ncol, &nempt);
		if (ncol > 0) {
			if (nempt > 1) {
				_ecbm_rand_addr(d, lcol_addr, empt, nempt);
			} else {
				return ECBM_RC_OVERFLOW;
			}
		} else {
			memset(empt, 0, sizeof(empt));
			for (int j = 0; j < nexsts; j++) {
				empt[exsts[j]] = j+1;
			}
			_ecbm_remap_addr(d, empt);
			lcol_addr = _ecbm_scanex(d, empt, exsts, &nexsts, &ncol, &nempt);
			if (ncol == 0) {
				return ECBM_RC_OK;
			}
		}
	}

	return ECBM_RC_RFAIL;
}

int ecbm_redistrib_addr(int d, int tries) {
	_ECBM_LOCK(d)
	int rc = _ecbm_redistrib_addr(d, tries);
	_ECBM_UNLOCK(d)
	return rc;
}