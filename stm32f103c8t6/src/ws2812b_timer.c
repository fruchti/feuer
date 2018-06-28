#include "ws2812b_timer.h"

WS2812B_Colour_t WS2812B_Buffer[WS2812B_MAX_LED_COUNT];
int WS2812B_LEDCount = 484;
uint16_t WS2812B_DMABuffer[48];
volatile int WS2812B_CurrentLED;
volatile bool WS2812B_Transferring;

static void WS2812B_UpdateBuffer(void)
{
    int start = 0;
    int led = WS2812B_CurrentLED;

    // If the current LED is odd, the upper half of the buffer has to be filled
    if(led & 1)
    {
        start = 24;
    }

    if(led >= WS2812B_LEDCount)
    {
        // All data has been send => zero the buffer
        for(int i = 0; i < 24; i++)
        {
            WS2812B_DMABuffer[start + i] = 0;
        }
        return;
    }

    uint8_t *raw = (uint8_t*)(WS2812B_Buffer);
    for(int i = 0; i < 24; i++)
    {
        if(raw[3 * led + i / 8] & (0x80 >> (i % 8)))
        {
            WS2812B_DMABuffer[start + i] = WS2812B_ONE_LENGTH;
        }
        else
        {
            WS2812B_DMABuffer[start + i] = WS2812B_ZERO_LENGTH;
        }
    }
}

void WS2812B_StartTransfer(void)
{
    WS2812B_Transferring = true;

    // Fill buffer
    WS2812B_CurrentLED = 0;
    WS2812B_UpdateBuffer();
    WS2812B_CurrentLED = 1;
    WS2812B_UpdateBuffer();

    // Start DMA
    DMA1_Channel3->CMAR = (uint32_t)&(*WS2812B_DMABuffer);
    DMA1_Channel3->CCR = DMA_CCR_PL |           // Top priority
        DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0 |     // 16-bit transfers
        DMA_CCR_MINC | DMA_CCR_CIRC |           // Memory increment, circular
        DMA_CCR_DIR |                           // Memory to peripheral
        DMA_CCR_HTIE | DMA_CCR_TCIE |           // Interrupts
        DMA_CCR_EN;                             // Enable DMA

    // Start timer
    TIM3->CR1 = TIM_CR1_URS | TIM_CR1_CEN;
}

void WS2812B_Init(void)
{
    for(int i = 0; i < WS2812B_LEDCount; i++)
    {
        WS2812B_Buffer[i].r = 0;
        WS2812B_Buffer[i].g = 0;
        WS2812B_Buffer[i].b = 0;
    }

    //// ---- CLOCKS ---- ////
    // Output is PA7, TIM3_CH2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    //// ---- GPIO ---- ////
    // PA7 in alternate function mode, full speed output
    GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7;

    //// ---- TIMER ---- ////
    // Set prescaler
    TIM3->PSC = 0;
    TIM3->ARR = WS2812B_ZERO_LENGTH + WS2812B_ONE_LENGTH;
    TIM3->CCR2 = 0;
    // Enable  Output compare
    TIM3->CCMR1 = TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE |\
        TIM_CCMR1_OC2FE;
#if WS2812B_INVERT_DO
    TIM3->CCER = TIM_CCER_CC2E | TIM_CCER_CC2P;
#else
    TIM3->CCER = TIM_CCER_CC2E;
#endif
    // Enable update DMA request
    TIM3->DIER = TIM_DIER_UDE;
    // DMA base address
    TIM3->DCR = ((uint32_t)&(TIM3->CCR2) - (uint32_t)&(TIM3->CR1)) / 4;
    // Enabble TIM3
    TIM3->CR1 = TIM_CR1_URS | TIM_CR1_CEN;

    //// ---- DMA ---- ////
    // Timer 3 update is DMA1 channel 3
    for(int i = 0; i < sizeof(WS2812B_DMABuffer) / 2; i++)
    {
        WS2812B_DMABuffer[i] = WS2812B_ZERO_LENGTH;
    }
    DMA1_Channel3->CNDTR = sizeof(WS2812B_DMABuffer) / 2;
    DMA1_Channel3->CPAR = (uint32_t)&(TIM3->DMAR);

    NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    for(unsigned int i = 0; i < 1000; i++)
    {
        __asm__ volatile("nop");
    }

    WS2812B_StartTransfer();
}

void DMA1_Channel3_IRQHandler(void)
{
    WS2812B_CurrentLED++;
    if(WS2812B_CurrentLED == WS2812B_LEDCount + 2)
    {
        DMA1_Channel3->CCR = 0;
        TIM3->CCR2 = 0;
        // Send the last pulse before stopping the timer
        TIM3->CR1 |= TIM_CR1_OPM;
        WS2812B_Transferring = false;
    }
    else
    {
        WS2812B_UpdateBuffer();
    }

    // if(DMA1->ISR & DMA_ISR_HTIF3)
    // {
    //     // Half transfer interrupt
    // }
    // else
    // {
    //     // Transfer complete
    // }
    // Clear all interrupt flags
    DMA1->IFCR = DMA_IFCR_CGIF3;
}