#include "stdsiga.h"

#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

static void (*_stdsiga_onSoftTerminate)();

static void* _stdsiga_tsoftTerminate(void* arg) {
	_stdsiga_onSoftTerminate();
	exit(0);
	
	return NULL;
}

static void _stdsiga_intSig(int sig) {
	pthread_t th = {0};
	pthread_create(&th, NULL, _stdsiga_tsoftTerminate, NULL);
	pthread_detach(th);
}

void stdsiga_init(void (*onSoftTerminate)()) {
	_stdsiga_onSoftTerminate = onSoftTerminate;
	signal(SIGINT, _stdsiga_intSig);
}