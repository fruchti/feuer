#include "main.h"

int main(void)
{
    // RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    USB_Init();
    WS2812B_Init();

    // GPIOC->CRH &= ~GPIO_CRH_CNF13 & ~GPIO_CRH_MODE13;
    // GPIOC->CRH |= GPIO_CRH_MODE13_0;
    // GPIOC->BRR = (1 << PIN_LED);

    // int offset = 0;
    for(;;)
    {
        while(WS2812B_Transferring);

        // int brightness = 64;
        // for(int i = 0; i < WS2812B_LEDCount; i++)
        // {
        //     WS2812B_Buffer[i].r = ((i + offset) % WS2812B_LEDCount) * brightness
        //         / (WS2812B_LEDCount - 1);
        //     WS2812B_Buffer[i].g = 1;
        //     WS2812B_Buffer[i].b = offset* brightness
        //         / (WS2812B_LEDCount - 1);
        // }
        // offset++;
        // offset %= WS2812B_LEDCount;
        WS2812B_StartTransfer();
    }
}

