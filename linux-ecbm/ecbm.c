#include "ecbm.h"

#include <asm-generic/ioctls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>


EcbmLinux _ecbml_ports[ECBM_PORTS] = {0}; 


static void* _ecbm_thread(void* arg) {
	EcbmLinux* ecbm = (EcbmLinux*)arg;
	int rc;
	uint8_t c;

#if ECBM_DEBUG
	printf("ecbm thread %i run.\n", ecbm->d);
#endif

	while (1) {
		rc = read(ecbm->fd, &c, 1);
		if (rc == 1) {
			// printf("%02X\n", c);
			_ecbmc_onrx(ecbm->d, c);
		}
	}
}

static void _ecbm_yeld() {
	usleep(100);
}

static void _ecbm_get_time(struct timespec* now) {
	clock_gettime(CLOCK_MONOTONIC, now);
}

static void _ecbm_write(int d, const uint8_t* data, int ndata) {
	write(_ecbml_ports[d].fd, data, ndata);
}

static void _ecbm_sendbreak(int d) {
	tcsendbreak(_ecbml_ports[d].fd, 1);
}

int ecbm_init(int d, EcbmSpeed speed, const char* dev) {
	if (d < 0 || d >= ECBM_PORTS) {
		return ECBM_RC_INC_D;
	}
	int fd = open (dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		return ECBM_RC_DRIVER_FAIL;
	}

	struct termios tio = {0};
	int baudrate;
	if (speed == ECBM_SPEED_FAST) {
		baudrate = B115200;
	} else {
		baudrate = B19200;
	}
	cfsetispeed(&tio, baudrate); 
	cfsetospeed(&tio, baudrate);
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |=  CS8;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CRTSCTS;
	tio.c_cflag |= CREAD | CLOCAL;
	tio.c_iflag &= ~(IXON | IXOFF | IXANY);
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tio.c_oflag &= ~OPOST;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	tio.c_cflag &= ~(PARENB | CMSPAR | PARODD);
	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIFLUSH);

	_ecbml_ports[d].fd = fd;
	_ecbml_ports[d].d = d;
	int rc = _ecbmc_init(d, _ecbm_write, _ecbm_sendbreak, _ecbm_yeld, _ecbm_get_time);
	if (rc == ECBM_RC_OK) {
		_ecbml_ports[d].fh = fdopen(_ecbml_ports[d].fd, "rb+");
		if (pthread_create(&_ecbml_ports[d].thread, NULL, _ecbm_thread, &_ecbml_ports[d]) != 0) {
			return ECBM_RC_THREAD_FAIL;
		}
	} else {
		return rc;
	}

	return ECBM_RC_OK;
}

int ecbm_deinit(int d) {
	int rc = ecbm_get_mode(d);
	if (rc < 0) {
		return rc;
	}

	close(_ecbml_ports[d].fd);
	return ECBM_RC_OK;
}

int ecbm_set_speed(int d, EcbmSpeed speed) {
	int rc = ecbm_get_mode(d);
	int baudrate;
	if (rc < 0) {
		return rc;
	}

	if (speed == ECBM_SPEED_FAST) {
		baudrate = B115200;
	} else {
		baudrate = B19200;
	}
	struct termios tio = {0};
	tcgetattr(_ecbml_ports[d].fd, &tio);
	cfsetispeed(&tio, baudrate); 
	cfsetospeed(&tio, baudrate);
	return tcsetattr(_ecbml_ports[d].fd, TCSANOW, &tio);
}