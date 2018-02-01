#include "main.h"

int main(void)
{
    // RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    USB_Init();

    // GPIOC->CRH &= ~GPIO_CRH_CNF13 & ~GPIO_CRH_MODE13;
    // GPIOC->CRH |= GPIO_CRH_MODE13_0;
    // GPIOC->BRR = (1 << PIN_LED);


    for(;;)
    {
        //GPIOC->BRR = (1 << PIN_LED);
    }
}

