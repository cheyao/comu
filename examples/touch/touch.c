#include "ch32fun.h"

// Buttons (left to right):
// A9 (PB1)
// A8 (PB0)
// A7 (PA7)
// A5 (PA5)

// Our clock runs at 8*18=144MHz
// PB1 CLK is 72MHz, PB2 is 144MHz
uint16_t sample_touch() {
	TKey1->SAMPTR2 = (ADC_SampleTime_7Cycles5 << (3 * 9));

	TKey1->IDATAR1 = 0x25; // CHGOFFSET
	TKey1->RDATAR = 0x8;   // ACT_DCG -> Activates
	while (!(ADC1->STATR & ADC_FLAG_EOC))
		;

	return TKey1->RDATAR;
}

int main() {
	SystemInit();
	RCC->APB2PCENR |= (RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1);
	RCC->CFGR0 |= (0b11 << 14); // Max 14MHz. PCLK2 is 144Mhz by default so /8

	// PA4 is left led, PB11 is right
	GPIOA->CFGLR &= ~(0xf << (4 * 4));
	GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);
	GPIOB->CFGHR &= ~(0xf << (4 * 3));
	GPIOB->CFGHR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3);

	GPIOB->CFGLR &= ~(0xf << (4 * 1));
	GPIOB->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4 * 1);

	// TODO: Calibrate

	// One converted channel
	ADC1->RSQR1 = ((1 - 1) << 20);
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 9;

	ADC1->CTLR2 |= ADC_ADON;
	TKey1->CTLR1 |= ADC_BUFEN | (1 << 24); // Enable TKey

	uint16_t value = 0;
	while (1) {
		GPIOA->BSHR = (1 << (4 + 16 * (value & 1)));
		GPIOB->BSHR = (1 << (11 + 16 * ((value & 2) >> 1)));
		value = sample_touch();
		Delay_Ms(100);
	}
}
