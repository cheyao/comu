#include "bootloader.h"
#include "ch32fun.h"
#include "usb_config.h"

#include <stdio.h>

// TODO: Auto-reboot into flash after write
#define DISABLE_BOOTLOAD 0
#define BOOTLOADER_LED_POLARITY 1
#define BOOTLOADER_TIMEOUT_MS 5000 // If this is zero, we never run the code directly

#define DATA_SIZE 6144
#define SCRATCHPAD_SIZE DATA_SIZE + 128
#define BOOT_ADDRESS 0x1000
// TODO: RUN ADDR

// #define LED_L PA4
#define LED_L PA15
#define LED_R PB11

// TODO: Touch buttons
#define TOUCH_R1 PA9
#define TOUCH_R0 PA8
#define TOUCH_L0 PA7
#define TOUCH_L1 PA5

#define BOOT_LED(n) funDigitalWrite(LED_L, n)
#define BOOTLOADER_TIMEOUT_BASE 4294967295 - Ticks_from_Ms(BOOTLOADER_TIMEOUT_MS)

#define ENDPOINTS 2

__attribute__((section(".scratchpad"))) uint8_t scratchpad[SCRATCHPAD_SIZE];
__attribute__((section(".runwordpad"))) volatile int32_t runwordpad;

#define min(a, b) ((a < b) ? a : b)

#ifdef USB_DEBUG
#include <string.h>
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
	runwordpad = 0;
	SysTick->CNT = BOOTLOADER_TIMEOUT_BASE;
	funGpioInitAll();

	// Clear screen
	USB_DEBUG_PRINTF("\033[2JHello world!\n");
	funPinMode(LED_L, GPIO_CFGLR_OUT_10Mhz_PP);
	BOOT_LED(1);

	// Initialize ports
	GPIOA->CFGHR &= ~((0xf << (4 * 3)) | (0xf << (4 * 4)));
	GPIOA->CFGHR |= (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * 3) | (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * 4);

	// Set to pull-down to avoid mis-detection
	GPIOA->BSHR = (1 << (16 + 11)) | (1 << (16 + 12));

#if defined(CH32V203F8)
	// Sometimes USB shares pins w/ SWD
	Delay_Ms(250);
	AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;
#endif

	// 42MHz clock
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_USBPRE) | RCC_USBPRE_DIV3;
	RCC->APB2PCENR |= RCC_AFIOEN | RCC_IOPAEN;
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM;
	RCC->APB1PCENR |= RCC_USBEN;

	USBD->CNTR = USBD_FRES; // Suspend & disable all interrupts
	USBD->CNTR = 0;

	// Delay a tad (Slightly more optimized)
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

	EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;
	USBD->CNTR = USBD_CTRM | USBD_RESETM;
	USBD->ISTR = 0;
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	// GPIOA->BSHR = (1 << 31);

	// Process loop
	int32_t localpad = (int32_t)(SysTick->CNT);
	while (1) {
#if defined(BOOTLOADER_TIMEOUT_MS) && BOOTLOADER_TIMEOUT_MS
		if (localpad < 0) {
			localpad = (int32_t)(SysTick->CNT);
			if (localpad >= 0) {
#if !defined(DISABLE_BOOTLOAD) || !DISABLE_BOOTLOAD
				BOOT_LED(0);

				// Suspend USB interrupts etc.
				USBD->CNTR = USBD_FRES;

				// Boot to user program at 0x4000
				typedef void (*setype)(void);
				setype usercode = (setype)(BOOT_ADDRESS);
				usercode();
#else
				localpad = 0;
#endif
			}
		}
#endif

		if (localpad > 0) {
			if (--localpad == 0) {
				typedef void (*setype)(uint32_t*, volatile int32_t*);
				setype scratchexec = (setype)(scratchpad + 4);
				scratchexec((uint32_t*)&scratchpad[0], &runwordpad);
			}
		}

		volatile uint32_t commandpad = runwordpad;
		if (commandpad) {
			localpad = commandpad - 1;
			runwordpad = 0;
		}
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void) __attribute__((interrupt));
void USB_LP_CAN1_RX0_IRQHandler(void) {
	static const uint8_t zeros[] = {0, 0}; // Small macro to help reduce size

	// tx_buf and tx_pending gets executed
	// Using this to avoid code duplication
	// Saves some space
	static const uint8_t* tx_buf = NULL;
	static uint16_t tx_pending = 0;
	static uint8_t* rx_buf = NULL;
	static uint16_t rx_pending = 0;
	static uint8_t new_addr = 0;
	// Not 100% needed but I'd rather stay safe and define some usb commands
	static uint8_t usb_config = 0;

	const uint32_t istr = USBD->ISTR;

	// Correct transfer
	if (istr & USBD_CTR) {
		const int ep = istr & USBD_EP_ID;
		const int epr = USBD->EPR[ep];

#ifdef USB_DEBUG
		if (epr & USBD_CTR_RX && epr & USBD_CTR_TX) {
			USB_DEBUG_PRINTF("UB! Both RX & TX!\n");
		}
#endif

		if (epr & USBD_CTR_RX) {
			// Endpoint 0 - control EP
			if (ep == 0) {
				// Setup packet
				if (epr & USBD_SETUP) {
					// Memory in USBD is stored in ranks of 16 bits (even though they can store
					// 32 bits)
					// What a weird design
					const uint8_t request_type = USBD_EP[0][0] & 0xFF;
					const uint8_t request = USBD_EP[0][0] >> 8;
					const uint16_t value = USBD_EP[0][1];
					const uint16_t length = USBD_EP[0][3];

					// Request 0b00100001 0x0A = SET IDLE, can be ignored
					if ((request_type & USBD_REQ_TYP_MASK) == USBD_REQ_TYP_STANDARD) {
						// Standard setup
						switch (request) {
							case USB_GET_STATUS:
								// Non remote wakeup and non self powered -> 0
								tx_buf = zeros;
								tx_pending = 2;
								break;
							case USB_SET_ADDRESS:
								new_addr = value & 0xFF;
								tx_pending = 0xFFFF;
								break;
							case USB_GET_DESCRIPTOR:
								const uint8_t type = value >> 8;

								// 64 bytes TX
								if (type == USBD_DEVICE_DESCRIPTOR) {
									tx_buf = device_descriptor;
									tx_pending = sizeof(device_descriptor);
								} else if (type == USBD_CONFIG_DESCRIPTOR) {
									tx_buf = config_descriptor;
									tx_pending = sizeof(config_descriptor);
								} else if (type == USBD_HID_DESCRIPTOR) {
									tx_buf = special_hid_desc;
									tx_pending = sizeof(special_hid_desc);
								} else if (type == USBD_STRING_DESCRIPTOR) {
									tx_buf = usb_string_descriptors[value & 0xFF];
									tx_pending = tx_buf[0];

#ifdef USB_DEBUG
									if ((value & 0xFF) >=
									    sizeof(usb_string_descriptors)) {
										USB_DEBUG_PRINTF(
											"ERROR! STR NOTFOUND\n");
									}
#endif
								}

								tx_pending = min(tx_pending, length);

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
								tx_buf = zeros;
								tx_pending = 1;
								break;
							default:
								tx_pending = 0xFFFF;
								USB_DEBUG_PRINTF("EP0 SETUP UNHANDLED: %d\n", request);
								break;
						}
					} else {
						// Non-standard reqs
						switch (request) {
							case HID_SET_REPORT:
								rx_pending = min(length, SCRATCHPAD_SIZE);
								rx_buf = scratchpad;
								runwordpad = 1;

								SetEPR_Status(0, USBD_EPR_STAT_RX_MASK,
									      USBD_EPR_STAT_RX_VALID);
								break;
							case HID_GET_REPORT:
								tx_pending = min(length, SCRATCHPAD_SIZE);
								tx_buf = scratchpad;
								break;
							case HID_SET_IDLE:
								// Ignroe idle requests
								tx_pending = 0xFFFF;
								break;
							default:
								tx_pending = 0xFFFF;
								USB_DEBUG_PRINTF("EP0 HID UNHANDLED: %d\n", request);
								break;
						}
					}
				} else {
					// OUT RX packet
					const uint16_t len = USBD_BDT->EP[0].COUNTn_RX & 0x3FF;

					if (len != 0) {
						if (rx_pending > 0) {
							// We got a data packet!
							// Copy data over to scratchpad
							for (int i = 0; i < len; ++i) {
								rx_buf[i] =
									(USBD_EP[0][i / 2] >> ((i & 1) << 3)) & 0xFF;
							}

							// ACK packet (1 for each data packet)
							rx_buf += len;
							rx_pending -= len;

							if (rx_pending == 0) {
								// Finished transaction, do something
								uint32_t* last4 = (uint32_t*)(rx_buf - 4);
								// The code shall be run
								if (*last4 == 0x1234abcd) {
									*last4 = 0;
									runwordpad = 100;
									rx_buf = 0;
								}
							}
						}

						tx_pending = 0xFFFF;
					}

					// Ignore, we can continue to recieve
					// (Probably an 0 byte control packet)
					SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);
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
		if (ep == 0 && (epr & (USBD_CTR_RX | USBD_CTR_TX))) {
			if (tx_pending == 0) {
				tx_buf = NULL;
				// Enable recieving data
				SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);
			} else {
				// 0xFFFF is special variable to say 0 byte tx
				if (tx_pending == 0xFFFF) {
					tx_pending = 0;
				}

				const uint16_t tx_len = min(tx_pending, 64);
				for (int i = 0; i < tx_len; i += 2) {
					USBD_EP[0][i / 2] = *((const uint16_t*)(tx_buf + i));
				}
				USBD_BDT->EP[0].COUNTn_TX = tx_len;
				tx_buf += tx_len;
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
			} else {
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

		SetEPR_Status(0, USBD_EPR_EP_TYPE_MASK, USBD_EPR_EP_TYPE_CTRL);
		SetEPR_Status(0, USBD_EPR_STAT_RX_MASK, USBD_EPR_STAT_RX_VALID);
		SetEPR_Status(0, USBD_EPR_STAT_TX_MASK, USBD_EPR_STAT_TX_NAK);

		// This saved 4 bytes, equal to code above
		//const uint16_t to_toggle = (USBD->EPR[0] & (USBD_EPR_STAT_RX_MASK | USBD_EPR_STAT_TX_MASK)) ^
		//			   (USBD_EPR_STAT_RX_VALID | USBD_EPR_STAT_TX_NAK);
		//USBD->EPR[0] = 0b1'0'00'0'00'0'1'0'00'0000 | USBD_EPR_EP_TYPE_CTRL | to_toggle;

		// We already cleared all write-once bits so EP_KIND is 0

		// Enable USB function
		USBD->DADDR = USBD_EF;
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
