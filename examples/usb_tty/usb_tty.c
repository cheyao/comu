#include "ch32fun.h"
#include "usbd.h"
#include <stdio.h>
#include <string.h>

#define LED_L PA4
#define LED_R PB11

#define BLINK_DELAY 100

uint8_t run = 0; // print stuff or not

void blink(int n) {
	for (int i = n - 1; i >= 0; i--) {
		funDigitalWrite(LED_L, 0);
		funDigitalWrite(LED_R, 0);
		Delay_Ms(BLINK_DELAY);
		funDigitalWrite(LED_L, 1);
		funDigitalWrite(LED_R, 1);
		if (i) {
			Delay_Ms(BLINK_DELAY);
		}
	}
}

// this callback is mandatory when FUNCONF_USE_USBPRINTF is defined,
// can be empty though
void HandleUSBInput(int numbytes, uint8_t* data) {
	if (numbytes == 1) {
		switch (data[0]) {
			case '1':
				blink(1);
				break;
			case '2':
				blink(2);
				break;
			case '3':
				blink(3);
				break;
			case 'c':
				run = 1;
				break;
			case 'p':
				run = 0;
				break;
		}
	} else {
		_write(0, (const char*)data, numbytes);
	}
}

int main() {
	SystemInit();

	funGpioInitAll();

	funPinMode(LED_L, GPIO_CFGLR_OUT_10Mhz_PP);
	funPinMode(LED_R, GPIO_CFGLR_OUT_10Mhz_PP);

	USBDSetup();
	blink(1);

	int i = 0;
	while (1) {
		poll_input(); // check if there is input from the tty

		if (run) {
			printf("Counting: %d\r", i++);
		}

		Delay_Ms(100);
	}
}
