#include "ch32fun.h"
#include "usbd.h"
#include <stdio.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

int lrx = 0;
int rx_off = 0;
uint8_t scratchpad[256] __attribute__((aligned(4)));

// 115200 8N1, reported back on CDC GET_LINE_CODING
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08};

// USB input goes here
void HandleUSBInput(int numbytes, uint8_t* data) {}

int HandleHidUserSetReportSetup(struct _USBState* ctx, tusb_control_request_t* req) {
	int id = req->wValue & 0xff;
	// Receive into scratchpad
	if (id == 0xaa && req->wLength <= sizeof(scratchpad)) {
		ctx->pCtrlPayloadPtr = scratchpad;
		lrx = req->wLength;
		rx_off = 0;
		return lrx;
	}
	return 0;
}

int HandleHidUserGetReportSetup(struct _USBState* ctx, tusb_control_request_t* req) {
	int id = req->wValue & 0xff;
	// Reply with scratchpad
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

// Must match bVendorCode / iLandingPage / bMS_VendorCode in bos_descriptor
// Vendor codes must not collide with the HID class request codes (0x01-0x0B), usbd.c handles those first
#define WEBUSB_VENDOR_CODE 0x40
#define WEBUSB_LANDING_PAGE 0x01
#define WEBUSB_REQUEST_GET_URL 0x02
#define MS_OS_20_VENDOR_CODE 0x41
#define MS_OS_20_DESCRIPTOR_INDEX 0x07

// Overrides the weak CDC-only handler in usbd.c to also answer the WebUSB GET_URL and MS OS 2.0 requests
// Return: >0 bytes to send from pCtrlPayloadPtr, -1 to ACK, 0 to STALL
int HandleSetupCustom(struct _USBState* ctx, int setup_code) {
	const uint8_t type = ctx->USBD_SetupReqType & USB_REQ_TYP_MASK;
	const uint16_t wValue = ctx->USBD_IndexValue & 0xffff;
	const uint16_t wIndex = ctx->USBD_IndexValue >> 16;

	if (type == USB_REQ_TYP_VENDOR) {
		if (setup_code == WEBUSB_VENDOR_CODE && wIndex == WEBUSB_REQUEST_GET_URL && wValue == WEBUSB_LANDING_PAGE) {
			ctx->pCtrlPayloadPtr = (uint8_t*)url_descriptor;
			return sizeof(url_descriptor);
		}
		if (setup_code == MS_OS_20_VENDOR_CODE && wIndex == MS_OS_20_DESCRIPTOR_INDEX) {
			ctx->pCtrlPayloadPtr = (uint8_t*)ms_os_20_descriptor;
			return sizeof(ms_os_20_descriptor);
		}
		return 0;
	}

	if (type == USB_REQ_TYP_CLASS) {
		switch (setup_code) {
		case CDC_SET_LINE_CODING:
		case CDC_SET_LINE_CTLSTE:
		case CDC_SEND_BREAK:
			return ctx->USBD_SetupReqLen ? ctx->USBD_SetupReqLen : -1;
		case CDC_GET_LINE_CODING:
			ctx->pCtrlPayloadPtr = line_coding;
			return sizeof(line_coding);
		}
	}

	return 0;
}

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
