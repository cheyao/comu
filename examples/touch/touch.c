#include "ch32fun.h"
#include "usbd.h"
#include <stdio.h>

// USB input goes here
void HandleUSBInput(int numbytes, uint8_t* data) {}

// 4096 = max
// <1024 = pressed
// -> 2560 = threshold
#define THRESHOLD 2560

// Buttons (left to right):
// A9 (PB1)
// A8 (PB0)
// A7 (PA7)
// A5 (PA5)
#define BUTTONS 4
#define BUT_L1 9
#define BUT_L2 8
#define BUT_R2 7
#define BUT_R1 5

// Our clock runs at 8*18=144MHz
// PB1 CLK is 72MHz, PB2 is 144MHz
uint16_t sample_touch(const uint8_t key) {
	// Select converted channel
	ADC1->RSQR3 = key;

	TKey1->IDATAR1 = 0x10; // CHGOFFSET
	TKey1->RDATAR = 0x8;   // ACT_DCG -> Activates
	while (!(ADC1->STATR & ADC_FLAG_EOC))
		;

	return TKey1->RDATAR;
}

int main() {
	SystemInit();

	USBDSetup();

	RCC->APB2PCENR |= (RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1);
	RCC->CFGR0 |= (0b11 << 14); // Max 14MHz. PCLK2 is 144Mhz by default so /8

	// PA4 is left led, PB11 is right
	GPIOA->CFGLR &= ~(0xf << (4 * 4));
	GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);
	GPIOB->CFGHR &= ~(0xf << (4 * 3));
	GPIOB->CFGHR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3);

	// Tkeys
	GPIOB->CFGLR &= ~((0xf << (4 * 1)) | (0xf << (4 * 0)));
	GPIOB->CFGLR |= ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4 * 1)) | ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4 * 0));
	GPIOA->CFGLR &= ~((0xf << (4 * 5)) | (0xf << (4 * 7)));
	GPIOA->CFGLR |= ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4 * 5)) | ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4 * 7));

	// Reset ADC
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

	// Configure sampling times for channels
	TKey1->SAMPTR2 = (ADC_SampleTime_7Cycles5 << (3 * BUT_L1)) | (ADC_SampleTime_7Cycles5 << (3 * BUT_L2)) | (ADC_SampleTime_7Cycles5 << (3 * BUT_R2)) |
			 (ADC_SampleTime_7Cycles5 << (3 * BUT_R1));

	// Always one channel at a time
	ADC1->RSQR1 = (0 << 20);
	ADC1->RSQR2 = 0;

	ADC1->CTLR2 |= ADC_ADON;

	// The calibration isn't 100% needed
	// But we include it just in case

	// Reset calibration
	ADC1->CTLR2 |= ADC_RSTCAL;
	while (ADC1->CTLR2 & ADC_RSTCAL)
		;

	// Calibrate
	ADC1->CTLR2 |= ADC_CAL;
	while (ADC1->CTLR2 & ADC_CAL)
		;

	TKey1->CTLR1 |= ADC_BUFEN | (1 << 24); // Enable TKey
	while (1) {
		poll_input();

		// Left pressed
		if (sample_touch(BUT_L1) < THRESHOLD) {
			GPIOB->BSHR = (1 << (11 + 16));
        }
		if (sample_touch(BUT_L2) < THRESHOLD) {
			GPIOB->BSHR = (1 << (11 + 0));
		}

		// Right pressed
		if (sample_touch(BUT_R1) < THRESHOLD) {
			GPIOA->BSHR = (1 << (4 + 16));
        }
		if (sample_touch(BUT_R2) < THRESHOLD) {
			GPIOA->BSHR = (1 << (4 + 0));
		}

		Delay_Ms(10);
	}
}
