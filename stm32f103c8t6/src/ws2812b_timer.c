#include "ws2812b_timer.h"

WS2812B_Colour_t WS2812B_BackBuffer[WS2812B_MAX_LED_COUNT];
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

    memcpy(WS2812B_Buffer, WS2812B_BackBuffer, sizeof(WS2812B_Buffer));

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
    // Output is PB4, TIM3_CH1
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    //// ---- GPIO ---- ////
    // PB4 in alternate function mode, full speed open-drain output
    GPIOB->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOB->CRL |= GPIO_CRL_CNF4 | GPIO_CRL_MODE4;
    // Remap TIM3 IOs
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_PARTIALREMAP;

    //// ---- TIMER ---- ////
    // Set prescaler
    TIM3->PSC = 0;
    TIM3->ARR = WS2812B_ZERO_LENGTH + WS2812B_ONE_LENGTH;
    TIM3->CCR1 = 0;
    // Enable  Output compare
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |\
        TIM_CCMR1_OC1FE;
#if WS2812B_INVERT_DO
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P;
#else
    TIM3->CCER = TIM_CCER_CC1E;
#endif
    // Enable update DMA request
    TIM3->DIER = TIM_DIER_UDE;
    // DMA base address
    TIM3->DCR = ((uint32_t)&(TIM3->CCR1) - (uint32_t)&(TIM3->CR1)) / 4;
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
    if(WS2812B_CurrentLED == WS2812B_LEDCount + 3)
    {
        DMA1_Channel3->CCR = 0;
        TIM3->CCR1 = 0;
        // Send the last pulse before stopping the timer
        TIM3->CR1 |= TIM_CR1_OPM;
        WS2812B_Transferring = false;
    }
    else
    {
        WS2812B_UpdateBuffer();
    }
    DMA1->IFCR = DMA_IFCR_CGIF3;
}