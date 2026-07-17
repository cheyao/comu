#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "ch32fun.h"
#include "funconfig.h"

#define FUSB_CONFIG_EPS 4 // Include EP0 in this count
#define FUSB_EP1_MODE 1	  // TX (IN)
#define FUSB_EP2_MODE -1  // RX (OUT)
#define FUSB_EP3_MODE 1	  // TX (IN)
#define FUSB_USER_HANDLERS 1

#include "usb_defines.h"

#define FUSB_USB_VID 0x1209
#define FUSB_USB_PID 0xb003
#define FUSB_USB_REV 0x0007
#define FUSB_STR_MANUFACTURER u"cheyao"
#define FUSB_STR_PRODUCT u"USB Bootloader"
#define FUSB_STR_SERIAL u"BOOT"

// https://beyondlogic.org/usbnutshell/usb5.htm#DeviceDescriptors
static const uint8_t device_descriptor[] = {
	18,   // bLength - Length of this descriptor
	0x01, // bDescriptorType - Type (Device)
	0x10,
	0x01, // bcdUSB - The highest USB spec version this device supports (USB1.1)
	0x02, // bDeviceClass - Device Class
	0x0,  // bDeviceSubClass - Device Subclass
	0x0,  // bDeviceProtocol - Device Protocol  (000 = use config descriptor)
	64,   // bMaxPacketSize - Max packet size for EP0
	(uint8_t)(FUSB_USB_VID),
	(uint8_t)(FUSB_USB_VID >> 8), // idVendor - ID Vendor
	(uint8_t)(FUSB_USB_PID),
	(uint8_t)(FUSB_USB_PID >> 8), // idProduct - ID Product
	(uint8_t)(FUSB_USB_REV),
	(uint8_t)(FUSB_USB_REV >> 8), // bcdDevice - Device Release Number
	1,			      // iManufacturer - Index of Manufacturer string
	2,			      // iProduct - Index of Product string
	3,			      // iSerialNumber - Index of Serial string
	1, // bNumConfigurations - Max number of configurations (if more then 1, you can switch between them)
};

// clang-format off
// Copied from https://github.com/cnlohr/ch32fun/blob/master/examples_usb/bootloader/usb_config.h#L54-L92
// Multiple HID sizes is for performance optimizations (Thanks to @monte-monte for the info)
static const uint8_t special_hid_desc[] = {
	HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP ),
	HID_USAGE      ( 0xff ), // Needed?
	HID_REPORT_SIZE ( 8 ),
	HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
		HID_REPORT_COUNT ( 7 ),
		HID_REPORT_ID    ( 0xa8 )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT ( 127 ),
		HID_REPORT_ID    ( 0xaa )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 1024+127, 2 ),
		HID_REPORT_ID    ( 0xab )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 2048+127, 2 ),
		HID_REPORT_ID    ( 0xac )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 3072+127, 2 ),
		HID_REPORT_ID    ( 0xad )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 4095, 2 ), // Maximum allowed size in windows and macos
		HID_REPORT_ID    ( 0xae )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 5120+127, 2 ),
		HID_REPORT_ID    ( 0xaf )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_REPORT_COUNT_N ( 6144+127, 2 ),
		HID_REPORT_ID    ( 0xb0 )
		HID_USAGE        ( 0xff ),
		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
	HID_COLLECTION_END
};
// clang-format on

/* Configuration Descriptor Set */
static const uint8_t config_descriptor[] = {
	// Config
	0x09, // bLength
	0x02, // bDescriptorType (Configuration)
	0x22,
	0x00, // wTotalLength 67
	0x01, // bNumInterfaces
	0x01, // bConfigurationValue
	0x00, // iConfiguration (String Index)
	0x80, // bmAttributes
	0x32, // bMaxPower 100mA

	// Interface
	0x09, // bLength
	0x04, // bDescriptorType - Interface
	0x00, // bInterfaceNumber - 0
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints - 1
	0x03, // bInterfaceClass - HID
	0x00, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
	0x00, // iInterface (String Index)

	// HID Descriptor
	0x09, // bLength
	0x21, // bDescriptorType
	0x10,
	0x01, // bcdHID (1.1)
	0x00, // bCountryCode
	0x01, // xbNumDescriptors
	0x22, // bDescriptorType
	sizeof(special_hid_desc),
	0x00,

	// EP1 descriptor
	0x07, // bLength
	0x05, // bDescriptorType
	0x81, // bEndpointAddress -> EP1 OUT
	0x03, // bmAttributes -> interrupt
	0x08,
	0x00, // wMaxPacketSize TODO: Make this bigger?
	0xff, // bInterval
};

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct language __attribute__((section(".rodata"))) = {
	4, 3, {0x0409} // Language ID - English US (look in USB_LANGIDs)
};
const static struct usb_string_descriptor_struct string1
	__attribute__((section(".rodata"))) = {sizeof(FUSB_STR_MANUFACTURER),
					       3, // bDescriptorType - String Descriptor (0x03)
					       FUSB_STR_MANUFACTURER};
const static struct usb_string_descriptor_struct string2
	__attribute__((section(".rodata"))) = {sizeof(FUSB_STR_PRODUCT), 3, FUSB_STR_PRODUCT};
const static struct usb_string_descriptor_struct string3
	__attribute__((section(".rodata"))) = {sizeof(FUSB_STR_SERIAL), 3, FUSB_STR_SERIAL};

const static uint8_t* usb_string_descriptors[] = {
	(const uint8_t*)&language,
	(const uint8_t*)&string1,
	(const uint8_t*)&string2,
	(const uint8_t*)&string3,
};

#endif
