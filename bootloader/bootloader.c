#include "ch32fun.h"
#include "usbd.h"

#include <stdio.h>
#include <string.h>

#define LED_L PA4
#define LED_R PB11

#define TOUCH_R1 A9
#define TOUCH_R0 A8
#define TOUCH_L0 A7
#define TOUCH_L1 A5

__attribute__((naked)) int main() {
	// Setup PLL
	SystemInit();
	funGpioInitAll();

	// Setup LED
	GPIOA->CFGHR &= ~(0xf << (4 * 7));
	GPIOA->CFGHR |= ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 7));
	GPIOA->BSHR = (1 << 15);
	// End debug

	// Initialize ports
	GPIOA->CFGHR &= ~((0xf << (4 * 3)) | (0xf << (4 * 4)));
	GPIOA->CFGHR |= (GPIO_CNF_IN_PUPD) << (4 * 3) | (GPIO_CNF_IN_PUPD) << (4 * 4);

	// Set to pull-down
	GPIOA->BSHR = (1 << (16 + 11)) | (1 << (16 + 12));

	// 42MHz clock
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_USBPRE) | RCC_USBPRE_DIV3;
	RCC->APB2PCENR |= RCC_AFIOEN;
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM;
	RCC->APB1PCENR |= RCC_USBEN;
	USBD->CNTR = USBD_FRES; // Suspend & disable all interrupts
	USBD->CNTR = 0;

	Delay_Us(1000);

	// Initialize required registers & packet buffer description table
	USBD->BTABLE = 0;

	// 64 bytes for each buffer
	// EP0 has to be RTX
	USBD_BDT->EP[0].ADDR_TX = USBD_PBA_BASE + 0x00;
	USBD_BDT->EP[0].COUNT_TX = 0x0; // Configured on the fly
	USBD_BDT->EP[0].ADDR_RX = USBD_PBA_BASE + 0x40;
	USBD_BDT->EP[0].COUNT_RX = 0x8400; // 2 blocks of 32 bytes
	USBD_BDT->EP[1].ADDR_TX = USBD_PBA_BASE + 0x80;
	USBD_BDT->EP[1].COUNT_TX = 0x0;
	USBD_BDT->EP[2].ADDR_RX = USBD_PBA_BASE + 0xC0;
	USBD_BDT->EP[2].COUNT_RX = 0x8400;

	EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;
	USBD->CNTR = USBD_CTRM | USBD_RESETM | USBD_SUSPM | USBD_WKUPM;
	USBD->ISTR = 0;
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	GPIOA->BSHR = (1 << 31);
	while (1) {
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void) __attribute__((interrupt));
void USB_LP_CAN1_RX0_IRQHandler(void) {
	const uint32_t istr = USBD->ISTR;

	// Correct transfer
	if (istr & USBD_CTR) {
		int ep = istr & USBD_EP_ID;
		printf("TR: %x\n", ep);
	}

	if (istr & USBD_PMAOVR) {
		printf("ERROR! USB overflow\n");
		USBD->ISTR = ~USBD_PMAOVR;
	}

	if (istr & USBD_ERR) {
		printf("ERROR! USB error\n");
		USBD->ISTR = ~USBD_ERR;
	}

	if (istr & USBD_WKUP) {
		printf("USB waky waky\n");
		USBD->ISTR = ~USBD_SUSP;
	}

	if (istr & USBD_SUSP) {
		printf("USB we go eep\n");
		USBD->ISTR = ~USBD_WKUP;
	}

	if (istr & USBD_SOF) {
		printf("USB SOF\n");
		USBD->ISTR = ~USBD_SOF;
	}

	if (istr & USBD_ESOF) {
		printf("USB ESOF\n");
		USBD->ISTR = ~USBD_ESOF;
	}
}
