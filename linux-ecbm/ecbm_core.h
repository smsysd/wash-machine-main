#ifndef _ECBM_CORE_H
#define _ECBM_CORE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>

#define ECBM_PORTS		2

#define ECBM_BUF_SIZE	4096
#define _ECBM_BUF_SIZE	(ECBM_BUF_SIZE+10)+(ECBM_BUF_SIZE+10)/7

#define ECBM_RC_OK						0
#define ECBM_RC_OKC						-1
#define ECBM_RC_INTEGRITY_APP			-3
#define ECBM_RC_INC_D					-32
#define ECBM_RC_UNUSED					-33
#define ECBM_RC_NULL_PTR				-34
#define ECBM_RC_TIMEOUT					-35
#define ECBM_RC_INTEGRITY				-36
#define ECBM_RC_INC_ARG					-37
#define ECBM_RC_OVERFLOW				-38
#define ECBM_RC_RFAIL					-39
#define ECBM_RC_INC_ANSWER				-40
#define ECBM_RC_BUG						-127

#define ECBM_SAVE_TIMEOUT_MS			1000
#define ECBM_RESET_TIMEOUT_MS			1000

#define ECBM_DEF_MODE					ECBM_MODE_INTRUS

#define ECBM_DEBUG						0

#define ECBM_BROADCAST					0

typedef enum EcbmMode {
	ECBM_MODE_COLNDT,
	ECBM_MODE_INTRUS,
	ECBM_MODE_RELAXE,
	ECBM_MODE_SCAN
} EcbmMode;

typedef struct Ecbm {
	uint8_t usef;
	uint8_t lock;
	uint8_t buf[_ECBM_BUF_SIZE];
	int integrity_errw;
	int timeout_errw;
	int timeout_ms;
	uint8_t* usr_data;
	int nusr_data;
	EcbmMode mode;
	int rxptr;
	uint8_t rxctl;
	uint8_t rxf;

	void (*write)(int D, const uint8_t* data, int nData);
	void (*send_break)(int d);
	void (*yeld)();
	void (*get_time)(struct timespec* now);
} Ecbm;

typedef struct EcbmInfo {
	int addr;
	int wia;
	int version;
	int status;
	char status_descr[64];
	char descr[256];
} EcbmInfo;

int _ecbmc_init(int d,
	void (*write)(int d, const uint8_t* data, int nData),
	void (*send_break)(int d),
	void (*yeld)(),
	void (*get_time)(struct timespec* now));

void _ecbmc_onrx(int d, uint8_t b);

int ecbm_write(int d, int addr, int sig, const uint8_t* data, int n_data);
int ecbm_read(int d, int addr, int sig, uint8_t* buf, int buf_size);
int ecbm_wpoll(int d, int addr, int sig);

int ecbm_writew(int d, int addr, int sig, const uint8_t* data, int n_data, int timeout_ms);
int ecbm_readw(int d, int addr, int sig, uint8_t* buf, int buf_size, int timeout_ms);

int ecbm_info(int d, int addr, EcbmInfo* info);
int ecbm_status(int d, int addr, int* status, char descr[64]);
int ecbm_get_event(int d, int addr, int* event);
int ecbm_scan(int d, void (*onscan)(EcbmInfo* info));
int ecbm_indicate(int d, int addr, int arg);
int ecbm_save(int d, int addr, int arg);
int ecbm_reset(int d, int addr);
int ecbm_resethw(int d);

int ecbm_set_addr(int d, int addr, int new_addr);
int ecbm_swap_addr(int d, uint8_t addr1, uint8_t addr2);
int ecbm_remap_addr(int d, uint8_t addrs[256]);
int ecbm_redistrib_addr(int d, int tries);

int ecbm_set_mode(int d, EcbmMode mode);
EcbmMode ecbm_get_mode(int d);

char* ecbm_get_errmsg(int d, int* len);

#ifdef __cplusplus
}
#endif
#endif