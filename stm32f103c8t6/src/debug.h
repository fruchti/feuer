#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdint.h>
#include <string.h>

#include "stm32f1xx.h"

void Debug_Print(const char *message);
void Debug_PutChar(char c);

#endif
