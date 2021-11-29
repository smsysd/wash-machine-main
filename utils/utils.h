/* Author: Jodzik */
#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

void init(void (*onCashAppeared)(), void (*onCashRunout)());
void printLogo();
void setGiveMoneyMode();
void setProgram(int iProg);

#endif