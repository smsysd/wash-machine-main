#include "qrscaner.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_t _qrs_thread_id;
static char _qrs_command[512];
static void (*_qrs_callback)(const char* qr_code);
static char _qrs_qr_old[256];
static int _qrs_read_delay;
static int _qrs_equal_blocking;

int qrscaner_init(const char* camPath, int width, int height, int readDelayMs, int bright, int expos, int focus, int contrast, int equalBlocking, void (*callback)(const char* qr_code)) {
	char buf[256];
	memset(buf, 0, sizeof(buf));
	FILE* tzbc = popen("zbarcam --version", "r");
	fread(buf, sizeof(buf), 1, tzbc);
	fclose(tzbc);
	int resplen = strlen(buf);
	if (resplen > 12 || resplen < 5) {
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

	_qrs_read_delay = readDelayMs;
	_qrs_equal_blocking = equalBlocking;

	memset(buf, 0, 256);
	snprintf(buf, 256, "v4l2-ctl --set-ctrl=brightness=%i", bright);
	system(buf);

	memset(buf, 0, 256);
	snprintf(buf, 256, "v4l2-ctl --set-ctrl=contrast=%i", contrast);
	system(buf);
	
	if (expos != QRSCANER_AUTO) {
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=exposure_absolute=%i", contrast);
		system(buf);
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=exposure_auto=0");
		system(buf);
	} else {
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=exposure_auto=1");
		system(buf);
	}

	if (focus != QRSCANER_AUTO) {
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=focus_absolute=%i", contrast);
		system(buf);
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=focus_auto=0");
		system(buf);
	} else {
		memset(buf, 0, 256);
		snprintf(buf, 256, "v4l2-ctl --set-ctrl=focus_auto=1");
		system(buf);
	}

	snprintf(_qrs_command, 256, "zbarcam --raw --prescale=%ix%i --nodisplay --oneshot -Sdisable -Sqrcode.enable %s", width, height, camPath);
    memset(_qrs_qr_old, 0, 256);
	_qrs_callback = callback;
	return 0;
}

static void *_qrs_thread(void *param) {
    char qr_data[7168];
    while (1) {
        FILE *fp = popen(_qrs_command, "r");
		usleep(1000);
        fread(qr_data, 7168, 1, fp);
        pclose(fp);
		if (_qrs_equal_blocking) {
			if (strncmp(qr_data, _qrs_qr_old, 256)) {
				_qrs_callback(qr_data);
				strncpy(_qrs_qr_old, qr_data, 256);
			}
		} else {
			_qrs_callback(qr_data);
		}
		usleep(1000*_qrs_read_delay);
    }
    pthread_exit(0);
}
void qrscaner_start() {
    pthread_create(&_qrs_thread_id, NULL, _qrs_thread, NULL);
}

void qrscaner_stop() {
    pthread_cancel(_qrs_thread_id);
}

void qrscaner_clear() {
	memset(_qrs_qr_old, 0, 256);
}
