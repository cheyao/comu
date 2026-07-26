#include "ch32fun.h"

// We skip the useless initializations from ch32fun
void Init() __attribute__((naked)) __attribute((section(".init"))) __attribute((naked));
void Init(void) {
	GPIOA->CFGLR = (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);

loop:
	GPIOA->OUTDR ^= (1 << 4);

	for (volatile int i = 0; i < 0x800000; i += 1)
		;
    goto loop;
}
