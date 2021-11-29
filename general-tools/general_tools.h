/*
 * core.h
 *
 *  Created on: 6 июл. 2020 г.
 *      Author: Jodzik
 */

#ifndef GENERAL_H_
#define GENERAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define GTP_CUSTOM			0
#define GTP_HAL				1
#define GTP_LINUX			2

#define GTP					GTP_LINUX

#define MAX_CALL_HANDLES			10

typedef enum ReturnCode {
	OK				=	-1,
	OKC 			=	-2,
	FAIL 			=	-3,
	INCORRECT_ARG	=	-4,
	NULL_POINTER	=	-5,
	NO_MEMORY		=	-6,
	BUSY			=	-7,
	ALREADY			=	-8,
	INCORRECT_CALL	=	-9,
	OVERFLOW		=	-10,
	NOT_FOUND		=	-11,
	INTEGRITY_ERROR =	-12,
	TIMEOUT			=	-13
} ReturnCode;

typedef struct CallHandle {
	ReturnCode (*function)(uint16_t, void*);
	void* arg;
	uint32_t callPeriodMs;
	uint32_t nCalls;
	uint32_t iCall;
	uint32_t timeNextCall;
	uint8_t isUsed;
} CallHandle;

void *addElement(void *ptr, uint32_t sizeEl, uint32_t curLen, int32_t afterIndex);
void *deleteElement(void *ptr, uint32_t sizeEl, uint32_t curLen, uint32_t deleteIndex);
int8_t shiftElements(void *ptr, uint32_t sizeEl, uint32_t realLen, uint32_t fullLen, int32_t shift);

#if GTP == GTP_CUSTOM
void general_tools_init(uint32_t (*getTimeMs)());
#endif

#if GTP == GTP_HAL
void general_tools_init();
#endif

#if GTP == GTP_LINUX
void general_tools_init();
#endif

/* if nCalls == 0 - will infinity calls */
int32_t callHandler(ReturnCode (*function)(uint16_t callHandleId, void* arg), void* arg, uint32_t callPeriodMs, uint32_t nCalls);
ReturnCode killCallHandle(uint16_t callHandleId);
ReturnCode setCallHandlePeriod(uint16_t callHandleId, uint32_t callPeriodMs);
ReturnCode setCallHandleCalls(uint16_t callHandleId, uint32_t nCalls);
ReturnCode addCallHandleCalls(uint16_t callHandleId, int32_t nCalls);

void loop();

#ifdef __cplusplus
}
#endif

#endif /* GENERAL_H_ */
