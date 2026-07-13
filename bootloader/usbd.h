#ifndef USBD_H
#define USBD_H

#include "ch32fun.h"

typedef struct __attribute__((__packed__)) {
	__IO uint32_t ADDR_TX;
	__IO uint32_t COUNT_TX;
	__IO uint32_t ADDR_RX;
	__IO uint32_t COUNT_RX;
} USBD_DESC_TypeDef;

typedef struct __attribute__((__packed__)) {
	USBD_DESC_TypeDef EP[8];
} USBD_BDT_TypeDef;

// 0x400 of shared memory

// Up to 8 endpoints (0x10 each)
// Used to 0x80
// Buffer Description Table
#define USBD_BDT_BASE 0x40006000
#define USBD_BDT ((USBD_BDT_TypeDef*)USBD_BDT_BASE)

// Packet buffer (0x400 - 0x80 = 0x380) left
#define USBD_PBA_BASE 0x80 // Offset from USBD_BDT_BASE

#endif
