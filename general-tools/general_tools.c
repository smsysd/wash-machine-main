/*
 * core.c
 *
 *  Created on: 6 июл. 2020 г.
 *      Author: Jodzik
 */

#include "general_tools.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if GTP == GTP_HAL
#include "main.h"
#endif

#if GTP == GTP_LINUX
#include <unistd.h>
#include <pthread.h>

static pthread_t _pthread_id = 0;
#endif

static uint32_t _timeMs = 0;
static uint32_t (*_getTimeMs)() = NULL;
static uint32_t _nCallHandles = 0;
static CallHandle _callHandles[MAX_CALL_HANDLES];

// if afterIndex == -1 inserting being to end
void *addElement(void *ptr, uint32_t sizeEl, uint32_t curLen, int32_t afterIndex){
	uint32_t i;
	uint32_t j;
	uint32_t fullSize = sizeEl*(curLen);
	uint32_t firstSize = sizeEl*afterIndex;
	void *ptrn = malloc(fullSize+sizeEl);
	if(ptrn == NULL || curLen == 0) {
		return calloc(1, sizeEl);
	}
	if(afterIndex == -1){
		for(i = 0; i < fullSize; i++){
			((uint8_t *)ptrn)[i] = ((uint8_t *)ptr)[i];
		}
		for(j = 0; j < sizeEl; j++){
			((uint8_t *)ptrn)[i+j] = 0;
		}
	} else {
		if(afterIndex >= curLen) return NULL;
		for(i = 0; i < firstSize; i++){
			((uint8_t *)ptrn)[i] = ((uint8_t *)ptr)[i];
		}
		for(j = 0; j < sizeEl; j++){
			((uint8_t *)ptrn)[i+j] = 0;
		}
		for(; i < fullSize; i++){
			((uint8_t *)ptrn)[i+j] = ((uint8_t *)ptr)[i];
		}
	}
	free(ptr);
	return ptrn;
}

// if deleteIndex == -1 then will delete last element
void *deleteElement(void *ptr, uint32_t sizeEl, uint32_t curLen, uint32_t deleteIndex){
	uint32_t i;
	uint32_t fullSize = sizeEl*(--curLen);
	uint32_t firstSize = sizeEl*deleteIndex;

	if (ptr == NULL) return NULL;

	void *ptrn;
	if(deleteIndex == -1){
		ptrn = malloc(fullSize);
		if(ptrn == NULL) return NULL;
		for(i = 0; i < fullSize; i++){
			((uint8_t *)ptrn)[i] = ((uint8_t *)ptr)[i];
		}
	} else {
		if(deleteIndex > curLen) return NULL;
		ptrn = malloc(fullSize);
		if(ptrn == NULL) return NULL;
		for(i = 0; i < firstSize; i++){
			((uint8_t *)ptrn)[i] = ((uint8_t *)ptr)[i];
		}
		for(; i < fullSize; i++){
			((uint8_t *)ptrn)[i] = ((uint8_t *)ptr)[i+sizeEl];
		}
	}
	free(ptr);
	return ptrn;
}

// If shift > 0 - to right, else to left
int8_t shiftElements(void *ptr, uint32_t sizeEl, uint32_t realLen, uint32_t fullLen, int32_t shift){
	uint32_t fullLenBytes;
	uint32_t realLenBytes;
	uint8_t rightOrLeft;
	uint32_t shiftBytes;
	uint32_t end;
	uint32_t begin;

	if (shift == 0) {
		return OK;
	}

	if (shift > 0) {
		rightOrLeft = 1;
	} else {
		rightOrLeft = 0;
	}

	fullLenBytes = sizeEl * fullLen;
	realLenBytes = sizeEl * realLen;
	if (shift < 0) {
		shift = -shift;
	}
	shiftBytes = sizeEl * shift;

	if (realLenBytes > fullLenBytes) {
		return INCORRECT_ARG;
	}

	if (rightOrLeft) {
		if (fullLenBytes - realLenBytes <= shiftBytes) {
			end = realLenBytes + shiftBytes - 1;
		} else {
			end = fullLenBytes - 1;
		}
		begin = end - shiftBytes;
		while (begin >= 0) {
			((uint8_t*)ptr)[end] = ((uint8_t*)ptr)[begin];
			begin--;
			end--;
		}
	} else {
		if (shiftBytes >= realLenBytes) {
			return INCORRECT_ARG;
		}
		begin = shiftBytes;
		end = 0;
		while (begin < realLenBytes) {
			((uint8_t*)ptr)[end] = ((uint8_t*)ptr)[begin];
			begin++;
			end++;
		}
	}

	return OK;
}

#if GTP == GTP_CUSTOM
void general_tools_init(uint32_t (*getTimeMs)()) {
	_getTimeMs = getTimeMs;
	memset(_callHandles, 0, sizeof(_callHandles));
}
#endif

#if GTP == GTP_HAL
uint32_t gtp_hal_getTimeMs() {
	return HAL_GetTick();
}

void general_tools_init() {
	_getTimeMs = gtp_hal_getTimeMs;
	memset(_callHandles, 0, sizeof(_callHandles));
}
#endif

#if GTP == GTP_LINUX
void* gtp_linux_thread(void* arg) {
	while (1) {
		usleep(1000);
		_timeMs++;
		loop();
	}
}

uint32_t gtp_linux_getTimeMs() {
	return _timeMs;
}

void general_tools_init() {
	_getTimeMs = gtp_linux_getTimeMs;
	pthread_create(&_pthread_id, NULL, &gtp_linux_thread, NULL);
	pthread_detach(_pthread_id);
	memset(_callHandles, 0, sizeof(_callHandles));
}
#endif

static int16_t _getCallHandleIndex() {
	uint16_t i;

	for (i = _nCallHandles; i < MAX_CALL_HANDLES; i++) {
		if (_callHandles[i].isUsed == 0) {
			return i;
		}
	}

	return OVERFLOW;
}

/* return error code (if < 0), else return callHandleId
 * if nCalls == 0 - will infinity calls */
int32_t callHandler(ReturnCode (*function)(uint16_t callHandleId, void* arg), void* arg, uint32_t callPeriodMs, uint32_t nCalls) {
	int32_t id;

	if (function == NULL || _getTimeMs == NULL) {
		return NULL_POINTER;
	}

	if (_nCallHandles >= MAX_CALL_HANDLES) {
		return OVERFLOW;
	}

	id = _getCallHandleIndex();
	if (id < 0) {
		return id;
	}

	_callHandles[id].arg = arg;
	_callHandles[id].isUsed = 1;
	_callHandles[id].callPeriodMs = callPeriodMs;
	_callHandles[id].function = function;
	_callHandles[id].iCall = 0;
	_callHandles[id].nCalls = nCalls;
	_callHandles[id].timeNextCall = _getTimeMs() + callPeriodMs;

	return id;
}

ReturnCode killCallHandle(uint16_t callHandleId) {
	if (callHandleId >= MAX_CALL_HANDLES) {
		return INCORRECT_ARG;
	}
	if (_callHandles[callHandleId].isUsed == 0) {
		return NOT_FOUND;
	} else {
		_callHandles[callHandleId].isUsed = 0;
		return OK;
	}
}

ReturnCode setCallHandlePeriod(uint16_t callHandleId, uint32_t callPeriodMs) {
	if (callHandleId >= MAX_CALL_HANDLES) {
		return INCORRECT_ARG;
	}
	if (_callHandles[callHandleId].isUsed == 0) {
		return NOT_FOUND;
	} else {
		_callHandles[callHandleId].callPeriodMs = callPeriodMs;
		return OK;
	}
}

ReturnCode setCallHandleCalls(uint16_t callHandleId, uint32_t nCalls) {
	if (callHandleId >= MAX_CALL_HANDLES) {
		return INCORRECT_ARG;
	}
	if (_callHandles[callHandleId].isUsed == 0) {
		return NOT_FOUND;
	} else {
		_callHandles[callHandleId].nCalls = nCalls;
		return OK;
	}
}

ReturnCode addCallHandleCalls(uint16_t callHandleId, int32_t nCalls) {
	if (callHandleId >= MAX_CALL_HANDLES) {
		return INCORRECT_ARG;
	}
	if (_callHandles[callHandleId].isUsed == 0) {
		return NOT_FOUND;
	} else {
		if (nCalls < 0) {
			if (_callHandles[callHandleId].nCalls <= -nCalls) {
				_callHandles[callHandleId].isUsed = 0;
			} else {
				_callHandles[callHandleId].nCalls += nCalls;
			}
		} else {
			_callHandles[callHandleId].nCalls += nCalls;
		}

		return OK;
	}
}

static void _callHandlerLoop() {
	uint16_t i;
	for (i = 0; i < MAX_CALL_HANDLES; i++) {
		if (_callHandles[i].isUsed) {
			if (_getTimeMs() >= _callHandles[i].timeNextCall) {
				if (_callHandles[i].function(i, _callHandles[i].arg) == OK) {
					_callHandles[i].timeNextCall = _getTimeMs() + _callHandles[i].callPeriodMs;
					_callHandles[i].iCall++;
					if (_callHandles[i].nCalls > 0) {
						if (_callHandles[i].iCall >= _callHandles[i].nCalls) {
							killCallHandle(i);
						}
					}
				}
			}
		}
	}
}

void loop() {
	_callHandlerLoop();
}
