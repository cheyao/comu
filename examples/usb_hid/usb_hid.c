#include "ch32fun.h"
#include "usbd.h"
#include <stdio.h>

// USB input goes here
void HandleUSBInput(int numbytes, uint8_t* data) {}

int main() {
	SystemInit();
	funGpioInitAll();

	USBDSetup();

	int i = 0;
	while (1) {
		poll_input(); // check if there is input from the tty

		printf("Hello #%d\r\n", i++);

		Delay_Ms(1000);
	}
}
