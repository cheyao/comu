#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "ch32fun.h"

typedef struct __attribute__((__packed__)) {
	USBD_BTABLE_TypeDef EP[8];
} USBD_BDT_TypeDef;

// 0x400 of shared memory

// Up to 8 endpoints (0x10 each)
// Used to 0x80
// Buffer Description Table
#define USBD_BDT ((USBD_BDT_TypeDef*)CAN_USBD_SHARED_BASE)

// Packet buffer (0x400 - 0x80 = 0x380) left
#define USBD_PMA_BASE 0x80 // Offset from USBD_BDT_BASE
#define USBD_EP ((uint32_t (*)[32])(CAN_USBD_SHARED_BASE + USBD_PMA_BASE * 2))

#define USBD_EPR_CTR_RX 0x8000
#define USBD_EPR_DTOG_RX 0x4000
#define USBD_EPR_STAT_RX_MASK 0x3000
#define USBD_EPR_STAT_RX_DIS 0x0000
#define USBD_EPR_STAT_RX_STALL 0x1000
#define USBD_EPR_STAT_RX_NAK 0x2000
#define USBD_EPR_STAT_RX_VALID 0x3000
#define USBD_EPR_SETUP 0x0800
#define USBD_EPR_EP_TYPE_MASK 0x0600
#define USBD_EPR_EP_TYPE_BULK 0x0000
#define USBD_EPR_EP_TYPE_CTRL 0x0200
#define USBD_EPR_EP_TYPE_ISO 0x0400
#define USBD_EPR_EP_TYPE_INT 0x0600
#define USBD_EPR_EP_KIND 0x0100
#define USBD_EPR_CTR_TX 0x0080
#define USBD_EPR_DTOG_TX 0x0040
#define USBD_EPR_STAT_TX_MASK 0x0030
#define USBD_EPR_STAT_TX_DIS 0x0000
#define USBD_EPR_STAT_TX_STALL 0x0010
#define USBD_EPR_STAT_TX_NAK 0x0020
#define USBD_EPR_STAT_TX_VALID 0x0030
#define USBD_EPR_EA 0x000F

#define USB_GET_STATUS 0x00
#define USB_GET_DESCRIPTOR 0x06

#endif
