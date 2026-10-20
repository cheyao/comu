#include "ch32fun.h"
#include "usbd.h"
#include <stdio.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

int lrx = 0;
int rx_off = 0;
uint8_t scratchpad[256] __attribute__((aligned(4)));

// USB input goes here
int HandleHidUserSetReportSetup(struct _USBState* ctx, tusb_control_request_t* req) {
	int id = req->wValue & 0xff;
	// Reply with scratchpad
	if (id == 0xaa && req->wLength <= sizeof(scratchpad)) {
		ctx->pCtrlPayloadPtr = scratchpad;
		lrx = req->wLength;
		rx_off = 0;
		return lrx;
	}
	return 0;
}

int HandleHidUserGetReportSetup(struct _USBState* ctx, tusb_control_request_t* req) {
	// Recieve data?
	int id = req->wValue & 0xff;
	if (id == 0xaa) {

		ctx->pCtrlPayloadPtr = scratchpad;
		if (sizeof(scratchpad) - 1 < lrx) {
			return sizeof(scratchpad) - 1;
		} else {
			return lrx;
		}
	}

	// Return ACK
	return 0;
}

void HandleHidUserReportDataOut(struct _USBState* ctx, uint8_t* data, int len) {
	if (rx_off >= lrx)
		return;
	int n = min(len, (lrx - rx_off));
	memcpy(&scratchpad[rx_off], data, n);

	rx_off += n;
}

int HandleHidUserReportDataIn(struct _USBState* ctx, uint8_t* data, int len) { return len; }

void HandleHidUserReportOutComplete(struct _USBState* ctx) { return; }

int HandleInRequest(struct _USBState* ctx, int endp, uint8_t* data, int len) { return 0; }

int HandleSetupCustom(struct _USBState* ctx, int setup_code) { return 0; }

void HandleDataOut(struct _USBState* ctx, int endp, uint8_t* data, int len) {
	// Process incoming data from the host
	if (!endp)
		return;
	printf("EP%d OUT request data:\n", endp);
	for (int i = 0; i < len; i++) {
		printf("0x%02x ", data[i]);
	}
	printf("\n\n");
}

int main() {
	SystemInit();
	funGpioInitAll();

	USBDSetup();

	while (1) {
		// If EP is not busy send data to the host
		/*
		uint8_t* buffer = USBD_GetEPBufferIfAvailable(1);
		if (buffer) {
			buffer[0] = 'C';
			buffer[1] = 'o';
			buffer[2] = 'm';
			buffer[3] = 'u';
			USBD_SendEndpoint(1, 4);
		}
		*/

		Delay_Ms(100);
	}
}
