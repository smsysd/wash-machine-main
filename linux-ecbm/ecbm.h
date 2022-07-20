#ifndef _ECBM_H
#define _ECBM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ecbm_core.h"

#include <termios.h>
#include <pthread.h>
#include <stdio.h>

#define ECBM_RC_THREAD_FAIL			-128
#define ECBM_RC_DRIVER_FAIL			-129

typedef enum EcbmSpeed {
	ECBM_SPEED_SLOW,
	ECBM_SPEED_FAST
} EcbmSpeed;

typedef struct EcbmLinux {
	int fd;
	FILE* fh;
	pthread_t thread;
	int d;
} EcbmLinux;

int ecbm_init(int d, EcbmSpeed speed, const char* dev);
int ecbm_deinit(int d);
int ecbm_set_speed(int d, EcbmSpeed speed);

#ifdef __cplusplus
}
#endif
#endif