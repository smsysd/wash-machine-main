#include "qrscaner.h"

#include <string.h>
#include <stdio.h>
#include <pthread.h>

static pthread_t _qrs_thread_id;
static char _qrs_command[256];
static void (*_qrs_callback)(const char* qr_code);
static char _qrs_qr_old[256];

int qrscaner_init(const char* camPath, int width, int height, void (*callback)(const char* qr_code)) {
	snprintf(_qrs_command, 256, "zbarcam --raw --prescale=%ix%i --nodisplay --oneshot -Sdisable -Sqrcode.enable %s", width, height, camPath);
    memset(_qrs_qr_old, 0, 256);
	_qrs_callback = callback;
	return 0;
}

static void *_qrs_thread(void *param) {
    char qr_data[7168];
    while (1) {
        FILE *fp = popen(_qrs_command, "r");
        fread(qr_data, 7168, 1, fp);
        pclose(fp);
        if (strncmp(qr_data, _qrs_qr_old, 256)) {
            _qrs_callback(qr_data);
			strncpy(_qrs_qr_old, qr_data, 256);
        }
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
