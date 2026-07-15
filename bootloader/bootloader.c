#include "bootloader.h"
#include "ch32fun.h"
#include "usb_config.h"

#include <stdio.h>
#include <string.h>

#define LED_L PA4
#define LED_R PB11

#define TOUCH_R1 PA9
#define TOUCH_R0 PA8
#define TOUCH_L0 PA7
#define TOUCH_L1 PA5

#define ENDPOINTS 3

#define min(a, b) ((a < b) ? a : b)

#ifdef USB_DEBUG
#define USB_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define USB_DEBUG_PRINTF(...)
#endif

static inline void SetEPR_Status(const int ep, const uint16_t mask, const uint16_t value) {
	const uint16_t reg = USBD->EPR[ep];
	const uint16_t current_stat = reg & mask;
	// Which bits we need to toggle
	const uint16_t toggle = current_stat ^ value;

	// Conserve EA, TYPE and KIND (Non-toggle)
	uint16_t write_val = (reg & (USBD_EPR_EA | USBD_EPR_EP_TYPE_MASK | USBD_EPR_EP_KIND));
	write_val |= toggle;
	// Write 1 has no effect for CTR_RX and CTR_TX (0 clears)
	write_val |= (USBD_EPR_CTR_RX | USBD_EPR_CTR_TX);

	USBD->EPR[ep] = write_val;
}

int main() {
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
	GPIOA->CFGHR |= (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * 3) | (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * 4);

	// Set to pull-down
	GPIOA->BSHR = (1 << (16 + 11)) | (1 << (16 + 12));

	// 42MHz clock
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_USBPRE) | RCC_USBPRE_DIV3;
	RCC->APB2PCENR |= RCC_AFIOEN | RCC_IOPAEN;
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM;
	RCC->APB1PCENR |= RCC_USBEN;

	USBD->CNTR = USBD_FRES; // Suspend & disable all interrupts
	USBD->CNTR = 0;

	// Delay a tad
	for (volatile int i = 0; i < 1000; i++)
		;

	// Initialize required registers & packet buffer description table
	USBD->BTABLE = 0;

	// 64 bytes for each buffer
	// EP0 has to be RTX
	USBD_BDT->EP[0].ADDn_TX = USBD_PMA_BASE + 0x00;
	USBD_BDT->EP[0].COUNTn_TX = 0x0; // Configured on the fly
	USBD_BDT->EP[0].ADDn_RX = USBD_PMA_BASE + 0x00;
	USBD_BDT->EP[0].COUNTn_RX = 0x8400; // 2 blocks of 32 bytes
	USBD_BDT->EP[1].ADDn_TX = USBD_PMA_BASE + 0x40;
	USBD_BDT->EP[1].COUNTn_TX = 0x0;
	USBD_BDT->EP[2].ADDn_RX = USBD_PMA_BASE + 0x80;
	USBD_BDT->EP[2].COUNTn_RX = 0x8400;

	EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;
	USBD->CNTR = USBD_CTRM | USBD_RESETM | USBD_SUSPM | USBD_WKUPM;
	USBD->ISTR = 0;
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	GPIOA->BSHR = (1 << 31);

	while (1)
		;
}

void USB_LP_CAN1_RX0_IRQHandler(void) __attribute__((interrupt));
void USB_LP_CAN1_RX0_IRQHandler(void) {
	static uint8_t const* tx_buf = NULL;
	static uint16_t tx_pending = 0;
	static uint8_t new_addr = 0;
	static uint8_t usb_config = 0;

	const uint32_t istr = USBD->ISTR;

	// Correct transfer
	if (istr & USBD_CTR) {
		const int ep = istr & USBD_EP_ID;
		const int epr = USBD->EPR[ep];

		if (epr & USBD_CTR_RX && epr & USBD_CTR_TX) {
			USB_DEBUG_PRINTF("UB! Both RX & TX!");
		}

		if (epr & USBD_CTR_RX) {
			// Endpoint 0 - control EP
			if (ep == 0) {

				// Setup packet
				if (epr & USBD_SETUP) {
					// Memory in USBD is stored in ranks of 16 bits (even though they can store 32
					// bits)???
					const uint8_t request_type = USBD_EP[0][0] & 0xFF;
					const uint8_t request = USBD_EP[0][0] >> 8;
					const uint16_t value = USBD_EP[0][1];
					// const uint16_t index = USBD_EP[0][2]; (Unused for the moment)
					const uint16_t length = USBD_EP[0][3];

					USB_DEBUG_PRINTF("EP0 SETUP: %d %d %d %d\n", request_type, request, value,
							 length);

					if (request_type & 0b01100000) {
						USB_DEBUG_PRINTF("Recieved non-standard USB request!\n");
					}

					switch (request) {
						case USB_GET_STATUS:
							// Non remote wakeup and non self powered -> 0
							USBD_EP[0][0] = 0x00; // 2 bytes
							USBD_BDT->EP[0].COUNTn_TX = 2;
							SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_VALID);
							break;
						case USB_SET_ADDRESS:
							new_addr = value & 0xFF;
							tx_pending = 0xFFFF;
							break;
						case USB_GET_DESCRIPTOR:
							uint8_t type = value >> 8;

							// 64 bytes TX
							if (type == USBD_DEVICE_DESCRIPTOR) {
								tx_buf = device_descriptor;
								tx_pending = min(sizeof(device_descriptor), length);
							} else if (type == USBD_CONFIG_DESCRIPTOR) {
								tx_buf = config_descriptor;
								tx_pending = min(sizeof(config_descriptor), length);
							} else if (type == USBD_STRING_DESCRIPTOR) {
								tx_buf = usb_string_descriptors[value & 0xFF];
								tx_pending = min(tx_buf[0], length);

								// TODO: Language ID with index?
								// FIXME: Remove when non-debug
								if ((value & 0xFF) >= sizeof(usb_string_descriptors)) {
									USB_DEBUG_PRINTF(
										"ERROR! USB STR BUF OVERFLOW\n");
								}
							}

							break;
						case USB_GET_CONFIG:
							tx_buf = &usb_config;
							tx_pending = 1;
							break;
						case USB_SET_CONFIG:
							usb_config = value & 0xFF;
							tx_pending = 0xFFFF;
							break;
						case USB_GET_INTERFACE:
							USBD_EP[0][0] = 0x00; // 2 bytes
							USBD_BDT->EP[0].COUNTn_TX = 1;
							SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_VALID);
							break;
						default:
							tx_pending = -1;
							USB_DEBUG_PRINTF("EP0 SETUP: %d %d\n", request_type, request);
							break;
					}
				} else {
					// OUT RX packet
					// const uint16_t len = USBD_BDT->EP[0].COUNTn_RX & 0x3FF;

					// USB_DEBUG_PRINTF("EP0 OUT: %d bytes\n", len);

					// Ignore, we can continue to recieve
					// (Probably an 0 byte control packet)
					SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);

					// Maybe change this up for error handling?
				}
			} else {
				USB_DEBUG_PRINTF("EPn OUT\n");
			}
		}

		// length < sizeof -> Return start
		// length = sizeof -> Return all
		// length > sizeof -> Return partial then 0

		// 1st load for RX and auto-reload for TX
		// We need to send something:
		// - Set tx_buf and tx_pending to non-null and set TX to VALID
		// - This piece of code shall handle loading to buffer
		// - TX part shall start next transaction if we need to
		// TX detects if the operation should be continued by checking
		// if the tx_buf is null
		// TODO: Other EPs
		if (ep == 0 && (epr & USBD_CTR_RX || epr & USBD_CTR_TX)) {
			if (tx_pending == 0) {
				tx_buf = NULL;
				// Enable recieving data
				SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);
			} else if (tx_pending == 0xFFFF) {
				// ACK packet
				USBD_BDT->EP[0].COUNTn_TX = 0;
				SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_VALID);
			} else {
				const uint16_t tx_len = min(tx_pending, 64);
				for (int i = 0; i < tx_len; i += 2) {
					USBD_EP[0][i / 2] = *((uint16_t*)(tx_buf + i));
				}
				USBD_BDT->EP[0].COUNTn_TX = tx_len;
				tx_pending -= tx_len;
				SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_VALID);
			}
		}

		// Moved here because this flag is needed for above
		if (istr & USBD_CTR_RX) {
			// Clear RX (Toggle, 1 conserves bit)
			USBD->EPR[ep] = (USBD->EPR[ep] & (USBD_EA | USBD_EPKIND | USBD_EPTYPE)) | USBD_CTR_TX;
		}

		if (epr & USBD_CTR_TX) {
			if (ep == 0) {
				if (new_addr != 0) {
					USBD->DADDR = 0x80 | new_addr;
					new_addr = 0;
				}

				// Mostly handled by freestanding func above
				// USB_DEBUG_PRINTF("EP0 IN\n");
			} else {
				// TODO: Other EPs
				USB_DEBUG_PRINTF("EPn TX\n");
			}

			// Clear TX (Toggle)
			USBD->EPR[ep] = (USBD->EPR[ep] & (USBD_EA | USBD_EPKIND | USBD_EPTYPE)) | USBD_CTR_RX;
		}

		USBD->ISTR = ~USBD_CTR;
	}

	if (istr & USBD_RESET) {
		USB_DEBUG_PRINTF("\033[93mUSB RESET\033[0m\n");

		USBD->ISTR = ~USBD_RESET;
		USBD->BTABLE = 0;

		for (int i = 0; i < ENDPOINTS; ++i) {
			USBD->EPR[i] = i;
		}

		// TODO: Write directly to save bytes?
		SetEPR_Status(0, USBD_EPR_EP_TYPE_MASK, USBD_EPR_EP_TYPE_CTRL);
		SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);
		SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_NAK);

		/* This saved 4 bytes, but not using to conserve readability
		 * const uint16_t to_toggle = (USBD->EPR[0] & (USBD_EPR_STAT_RX_MASK | USBD_EPR_STAT_TX_MASK)) ^
		 * 			      (USBD_EPR_STAT_RX_VALID | USBD_EPR_STAT_TX_NAK);
		 * USBD->EPR[0] = 0b1'0'00'0'00'0'1'0'00'0000 | USBD_EPR_EP_TYPE_CTRL | to_toggle;
		 */

		// We already cleared all write-once bits so EP_KIND is 0
		// Maybe we need to toggle DTOG here

		// TODO: Other EPs

		// Enable USB function
		USBD->DADDR = USBD_EF;
	}

	if (istr & USBD_PMAOVR) {
		USBD->ISTR = ~USBD_PMAOVR;
	}

	if (istr & USBD_WKUP) {
		USBD->ISTR = ~USBD_SUSP;
	}

	if (istr & USBD_SUSP) {
		USBD->ISTR = ~USBD_WKUP;
	}

	if (istr & USBD_ESOF) {
		USBD->ISTR = ~USBD_ESOF;
	}

	if (istr & USBD_SOF) {
		USBD->ISTR = ~USBD_SOF;
	}

	if (istr & USBD_ERR) {
		USB_DEBUG_PRINTF("ERROR! USB error\n");

		USBD->ISTR = ~USBD_ERR;
	}
}
