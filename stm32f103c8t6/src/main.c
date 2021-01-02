#include "main.h"

int main(void)
{
    USB_Init();
    WS2812B_Init();
    LED_Init();

    for(;;)
    {
        while(WS2812B_Transferring);

        WS2812B_StartTransfer();

        LED_ON();
    }
}

