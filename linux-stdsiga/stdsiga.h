/* Author: Jodzik */
#ifndef STDSIGA_H_
#define STDSIGA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void stdsiga_init(void (*onSoftTerminate)());

#ifdef __cplusplus
}
#endif

#endif