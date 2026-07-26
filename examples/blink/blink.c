#include "ch32fun.h"

int main() {
	SystemInit();
	funGpioInitAll();

	// PA4 is left led, PB11 is right
	GPIOA->CFGLR &= ~(0xf << (4 * 4));
	GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);
	GPIOB->CFGHR &= ~(0xf << (4 * 3));
	GPIOB->CFGHR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3);

	GPIOA->OUTDR = (1 << 4);
	GPIOB->OUTDR = 0;

	while (1) {
		// BSHR: Port Set/Reset Register
		// bits 0-15 is set
		// bits 16-31 is reset
		GPIOA->OUTDR ^= (1 << 4);
		GPIOB->OUTDR ^= (1 << 11);
		Delay_Ms(250);
	}
}
