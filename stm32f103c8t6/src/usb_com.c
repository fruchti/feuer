#include "usb_com.h"
#include "ws2812b_timer.h"

unsigned USBCOM_Registers[USBCOM_N_REGISTERS];

uint8_t USBCOM_HandleSetupPacket(USB_SetupPacket_t *sp, const uint8_t **reply_data)
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
            USBCOM_Registers[sp->bRequest] = ((uint32_t)(sp->wIndex) << 16) | sp->wValue;
        }
    }

    return reply_length;
}

void USBCOM_HandleISO0OUT(void)
{
    int buffindex = USB->EP1R & USB_EP1R_DTOG_RX;
    int rcvlen = 0;
    uint8_t rcvbuf[64];
    if(buffindex)
    {
        rcvlen = USB_BTABLE_ENTRIES[1].COUNT_RX_0 & 0x3ff;
        USB_PMAToMemory(rcvbuf,
            USB_BTABLE_ENTRIES[1].ADDR_RX_0,
            rcvlen);
    }
    else
    {
        rcvlen = USB_BTABLE_ENTRIES[1].COUNT_RX_1 & 0x3ff;
        USB_PMAToMemory(rcvbuf,
            USB_BTABLE_ENTRIES[1].ADDR_RX_1,
            rcvlen);
    }

    if(rcvlen < 4)
    {
        return;
    }

    // First 4 bytes are the start LED
    int startled = *(uint32_t*)rcvbuf;

    if(startled >= WS2812B_LEDCount)
    {
        return;
    }

    memcpy(WS2812B_BackBuffer + startled, rcvbuf + 4, rcvlen - 4);
}