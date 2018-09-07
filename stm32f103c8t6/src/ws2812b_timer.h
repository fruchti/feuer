#ifndef WS2812B_TIMER_H
#define WS2812B_TIMER_H

#include <stdbool.h>
#include <string.h>

#include "stm32f103x6.h"

#define WS2812B_MAX_LED_COUNT                   512
#define WS2812B_INVERT_DO                       0

#define WS2812B_ZERO_LENGTH                     29
#define WS2812B_ONE_LENGTH                      57

typedef struct
{
    uint8_t g;
    uint8_t r;
    uint8_t b;
} __attribute__((packed, aligned(1))) WS2812B_Colour_t;

extern WS2812B_Colour_t WS2812B_BackBuffer[WS2812B_MAX_LED_COUNT];
extern int WS2812B_LEDCount;
extern volatile bool WS2812B_Transferring;

void WS2812B_Init(void);
void WS2812B_StartTransfer(void);

#endif