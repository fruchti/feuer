#include "usb_com.h"
#include "ws2812b_timer.h"
#include "led.h"

unsigned USBCOM_Registers[USBCOM_N_REGISTERS];

uint8_t USBCOM_HandleSetupPacket(USB_SetupPacket_t *sp,
    const uint8_t **reply_data)
{
    uint8_t reply_length = 0;

    if(sp->bmRequestType & USB_REQUEST_DIRECTION_IN)
    {
        // Read register
        if(sp->bRequest < USBCOM_N_REGISTERS)
        {
            *reply_data = (uint8_t*)&USBCOM_Registers[sp->bRequest];
            reply_length = sp->wLength;
        }
    }
    else
    {
        // Write register
        if(sp->bRequest < USBCOM_N_REGISTERS)
        {
            USBCOM_Registers[sp->bRequest] = ((uint32_t)(sp->wIndex) << 16)
                | sp->wValue;
        }
    }

    return reply_length;
}

void USBCOM_HandleISO0OUT(void)
{
    LED_OFF();

    int buffindex = USB->EP1R & USB_EP1R_DTOG_RX;
    int rcvlen = 0;
    uint16_t pmaoffset;

    if(buffindex)
    {
        rcvlen = USB_BTABLE_ENTRIES[1].COUNT_RX_0 & 0x3ff;
        pmaoffset = USB_BTABLE_ENTRIES[1].ADDR_RX_0;
    }
    else
    {
        rcvlen = USB_BTABLE_ENTRIES[1].COUNT_RX_1 & 0x3ff;
        pmaoffset = USB_BTABLE_ENTRIES[1].ADDR_RX_1;
    }

    if(rcvlen <= 4)
    {
        return;
    }

    // First 4 bytes are the start LED
    int startled = 0;
    USB_PMAToMemory((uint8_t*)&startled, pmaoffset, 4);

    if(startled >= WS2812B_LEDCount)
    {
        return;
    }

    // Rest of the packet is LED data
    USB_PMAToMemory((uint8_t*)(WS2812B_BackBuffer + startled), pmaoffset + 4,
        rcvlen - 4);
}
