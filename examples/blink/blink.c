#include "ch32fun.h"

int main() {
	SystemInit();
	funGpioInitAll();

	// PA4 is left led, PB11 is right
	GPIOA->CFGLR &= ~(0xf << (4 * 4));
	GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);
	GPIOB->CFGHR &= ~(0xf << (4 * 3));
	GPIOB->CFGHR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3);

	// Temp for nanoCH32V203 (PA15)
	GPIOA->CFGHR &= ~(0xf << (4 * 7));
	GPIOA->CFGHR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 7);

	while (1) {
		// BSHR: Port Set/Reset Register
		// bits 0-15 is set
		// bits 16-31 is reset
		GPIOA->BSHR = (1 << 4);
		GPIOB->BSHR = (1 << (11 + 16));

		GPIOA->BSHR = (1 << (15 + 16));
		Delay_Ms(250);
		GPIOA->BSHR = (1 << (4 + 16));
		GPIOB->BSHR = (1 << 11);

		GPIOA->BSHR = (1 << 15);
		Delay_Ms(250);
	}
}
