#include "qrscaner.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static FILE *_qrs_pipe = NULL;
static pthread_t _qrs_thread_id;
static char _qrs_command[1024];
static void (*_qrs_callback)(const char* qr_code);
static char _qrs_qr_old[256];
static int _qrs_equal_blocking;
static char _qrs_camPath[1024];
static int _qrs_bright;
static int _qrs_contrast;
static int _qrs_expos;
static int _qrs_focus;

void _qrs_ctrlcam() {
	char buf[1024];

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=brightness=%i --device %s", _qrs_bright, _qrs_camPath);
	system(buf);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=contrast=%i --device %s", _qrs_contrast, _qrs_camPath);
	system(buf);
	
	if (_qrs_expos != QRSCANER_AUTO) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=exposure_absolute=%i --device %s", _qrs_expos, _qrs_camPath);
		system(buf);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=exposure_auto=1 --device %s", _qrs_camPath);
		system(buf);
	} else {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=exposure_auto=3 --device %s", _qrs_camPath);
		system(buf);
	}

	if (_qrs_focus != QRSCANER_AUTO) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=focus_absolute=%i --device %s", _qrs_focus, _qrs_camPath);
		system(buf);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=focus_auto=0 --device %s", _qrs_camPath);
		system(buf);
	} else {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "v4l2-ctl --set-ctrl=focus_auto=1 --device %s", _qrs_camPath);
		system(buf);
	}
}

int qrscaner_init(const char* camPath, int width, int height, int bright, int expos, int focus, int contrast, int equalBlocking, void (*callback)(const char* qr_code)) {
	char buf[256];
	memset(buf, 0, sizeof(buf));
	FILE* tzbc = popen("zbarcam --version", "r");
	fread(buf, sizeof(buf), 1, tzbc);
	pclose(tzbc);
	int resplen = strlen(buf);
	if (resplen > 12 || resplen < 3) {
		printf("[ERROR][QRSCANER]: no zbarcam\n");
		return -2;
	}
	if (width == QRSCANER_AUTO || height == QRSCANER_AUTO) {
		printf("[ERROR][QRSCANER]: width and height must be no auto\n");
		return -1;
	}
	if (bright == QRSCANER_AUTO) {
		printf("[ERROR][QRSCANER]: bright must be no auto\n");
		return -1;
	}
	if (contrast == QRSCANER_AUTO) {
		printf("[ERROR][QRSCANER]: contrast must be no auto\n");
		return -1;
	}

	_qrs_equal_blocking = equalBlocking;
	_qrs_bright = bright;
	_qrs_contrast = contrast;
	_qrs_expos = expos;
	_qrs_focus = focus;
	memset(_qrs_camPath, 0, sizeof(_qrs_camPath));
	strncpy(_qrs_camPath, camPath, sizeof(_qrs_camPath));

	snprintf(_qrs_command, sizeof(buf), "zbarcam --raw --prescale=%ix%i --nodisplay -Sdisable -Sqrcode.enable %s", width, height, camPath);
    memset(_qrs_qr_old, 0, sizeof(buf));
	_qrs_ctrlcam();
	_qrs_callback = callback;
	return 0;
}

static void *_qrs_thread(void *param) {
    char qr_data[512];
	int resplen;
	int suspision = 0;
	time_t tlterm = 0;
	system("sudo pkill zbarcam");
	_qrs_pipe = popen(_qrs_command, "r");
    char* ptr;
	while (1) {
		memset(qr_data, 0, sizeof(qr_data));
		ptr = fgets(qr_data, sizeof(qr_data), _qrs_pipe);
		if (ptr != NULL) {
			resplen = strlen(qr_data);
			if (resplen < 4 || resplen > 128) {
				printf("[WARNING][QRSCANER] incorrect size of qr code\n");
				continue;
			}
			printf("[INFO][QRSCANER] QR: %s\n", qr_data);
			if (_qrs_equal_blocking) {
				if (strncmp(qr_data, _qrs_qr_old, sizeof(_qrs_qr_old)) != 0) {
					_qrs_callback(qr_data);
					strncpy(_qrs_qr_old, qr_data, sizeof(_qrs_qr_old));
				}
			} else {
				_qrs_callback(qr_data);
			}
			usleep(10000);
		} else {
			printf("[WARNING][QRSCANER] zbarcam was terminate\n");
			pclose(_qrs_pipe);
			if (time(NULL) - tlterm < 10) {
				suspision++;
				if (suspision > 3) {
					pthread_exit(NULL);
					return NULL;
				}
			} else {
				suspision = 0;
			}
			_qrs_ctrlcam();
			tlterm = time(NULL);
			_qrs_pipe = popen(_qrs_command, "r");
		}
    }
	pclose(_qrs_pipe);
    pthread_exit(NULL);
	return NULL;
}

void qrscaner_start() {
    pthread_create(&_qrs_thread_id, NULL, _qrs_thread, NULL);
}

void qrscaner_stop() {
    pthread_cancel(_qrs_thread_id);
}

void qrscaner_clear() {
	memset(_qrs_qr_old, 0, sizeof(_qrs_qr_old));
}
