#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "funconfig.h"
#include "ch32fun.h"

#define FUSB_BUFFERS_NUMBER   6 // Number of EP buffers (one for EP0, one per each IN/OUT, two for double)
#define FUSB_EP1_MODE         USBD_EP_MODE_TX // IN
#define FUSB_EP2_MODE         USBD_EP_MODE_RX // OUT
#define FUSB_EP3_MODE         USBD_EP_MODE_TX // IN
#define FUSB_EP5_MODE         USBD_EP_MODE_TX // IN
#define FUSB_EP6_MODE         USBD_EP_MODE_RX // OUT
#define FUSB_SUPPORTS_SLEEP   0
#define FUSB_HID_INTERFACES   0
#define FUSB_CURSED_TURBO_DMA 0 // Hacky, but seems fine, shaves 2.5us off filling 64-byte buffers.
#define FUSB_HID_USER_REPORTS 0
#define FUSB_IO_PROFILE       0
#define FUSB_USE_HPE          FUNCONF_ENABLE_HPE
#define FUSB_USER_HANDLERS    1
#define FUSB_USE_DMA7_COPY    0
#define FUSB_VDD_5V           FUNCONF_USE_5V_VDD

#include "usb_defines.h"

#define FUSB_USB_VID          0x1209
#define FUSB_USB_PID          0xd035
#define FUSB_USB_REV          0x0007
#define FUSB_STR_MANUFACTURER u"ch32fun"
#define FUSB_STR_PRODUCT      u"Mass Storage"
#define FUSB_STR_SERIAL       u"001"

static const uint8_t device_descriptor[] = {
	0x12,       // bLength
	0x01,       // bDescriptorType (Device)
	0x10, 0x01, // bcdUSB 1.10
	0xEF,       // bDeviceClass    <-- Composite Device (Miscellaneous)
	0x02,       // bDeviceSubClass <-- Common Class
	0x01,       // bDeviceProtocol <-- Interface Association Descriptor
	0x40,       // bMaxPacketSize0
	(uint8_t)(FUSB_USB_VID), (uint8_t)(FUSB_USB_VID >> 8), //idVendor - ID Vendor
	(uint8_t)(FUSB_USB_PID), (uint8_t)(FUSB_USB_PID >> 8), //idProduct - ID Product
	(uint8_t)(FUSB_USB_REV), (uint8_t)(FUSB_USB_REV >> 8), //bcdDevice - Device Release Number
	0x01,       // iManufacturer
	0x02,       // iProduct
	0x03,       // iSerialNumber
	0x01        // bNumConfigurations
};

static const uint8_t config_descriptor[ ] = {
	// -------------------------------------------------------------------------
	// Configuration Descriptor
	// -------------------------------------------------------------------------
	0x09,       // bLength
	0x02,       // bDescriptorType (Configuration)
	0x62, 0x00, // wTotalLength (98 bytes)
	0x03,       // bNumInterfaces (3: CDC Comm, CDC Data, MSC)
	0x01,       // bConfigurationValue
	0x00,       // iConfiguration
	0x80,       // bmAttributes (Bus Powered)
	0x32,       // bMaxPower (100mA)

	// -------------------------------------------------------------------------
	// Interface Association Descriptor (IAD) for CDC
	// -------------------------------------------------------------------------
	// Required for Composite devices so Windows knows Intf 0 & 1 allow to UART
	0x08,       // bLength
	0x0B,       // bDescriptorType (Interface Association)
	0x00,       // bFirstInterface
	0x02,       // bInterfaceCount (CDC uses 2 interfaces)
	0x02,       // bFunctionClass (CDC Control)
	0x02,       // bFunctionSubClass (ACM)
	0x01,       // bFunctionProtocol (AT)
	0x00,       // iFunction

	// -------------------------------------------------------------------------
	// Interface 0: CDC Communication Class
	// -------------------------------------------------------------------------
	0x09,       // bLength
	0x04,       // bDescriptorType (Interface)
	0x00,       // bInterfaceNumber (0)
	0x00,       // bAlternateSetting
	0x01,       // bNumEndpoints (1 interrupt endpoint)
	0x02,       // bInterfaceClass (CDC Control)
	0x02,       // bInterfaceSubClass (ACM)
	0x01,       // bInterfaceProtocol (AT)
	0x00,       // iInterface

	// CDC Functional Descriptors
	0x05, 0x24, 0x00, 0x10, 0x01,       // Header: CDC 1.10
	0x05, 0x24, 0x01, 0x00, 0x01,       // Call Management
	0x04, 0x24, 0x02, 0x02,             // ACM: Support line coding
	0x05, 0x24, 0x06, 0x00, 0x01,       // Union: Master=0, Slave=1

	// Endpoint 1: Interrupt IN (Notification)
	0x07,       // bLength
	0x05,       // bDescriptorType (Endpoint)
	0x81,       // bEndpointAddress (IN Endpoint 1)
	0x03,       // bmAttributes (Interrupt)
	0x08, 0x00, // wMaxPacketSize (8 bytes)
	0xFF,       // bInterval (255ms)

	// -------------------------------------------------------------------------
	// Interface 1: CDC Data Class
	// -------------------------------------------------------------------------
	0x09,       // bLength
	0x04,       // bDescriptorType (Interface)
	0x01,       // bInterfaceNumber (1)
	0x00,       // bAlternateSetting
	0x02,       // bNumEndpoints (2 bulk endpoints)
	0x0A,       // bInterfaceClass (CDC Data)
	0x00,       // bInterfaceSubClass
	0x00,       // bInterfaceProtocol
	0x00,       // iInterface

	// Endpoint 2: Bulk OUT
	0x07,       // bLength
	0x05,       // bDescriptorType (Endpoint)
	0x02,       // bEndpointAddress (OUT Endpoint 2)
	0x02,       // bmAttributes (Bulk)
	0x40, 0x00, // wMaxPacketSize (64 bytes)
	0x01,       // bInterval

	// Endpoint 3: Bulk IN
	0x07,       // bLength
	0x05,       // bDescriptorType (Endpoint)
	0x83,       // bEndpointAddress (IN Endpoint 3)
	0x02,       // bmAttributes (Bulk)
	0x40, 0x00, // wMaxPacketSize (64 bytes)
	0x01,       // bInterval

	// -------------------------------------------------------------------------
	// Interface 2: Mass Storage Class (MSC) - Bulk Only Transport (BOT)
	// -------------------------------------------------------------------------
	0x09,       // bLength
	0x04,       // bDescriptorType (Interface)
	0x02,       // bInterfaceNumber (2)
	0x00,       // bAlternateSetting
	0x02,       // bNumEndpoints (2 bulk endpoints)
	0x08,       // bInterfaceClass (Mass Storage)
	0x06,       // bInterfaceSubClass (SCSI Transparent)
	0x50,       // bInterfaceProtocol (Bulk-Only)
	0x00,       // iInterface

	// Endpoint 6: Bulk OUT (MSC Data from PC)
	0x07,       // bLength
	0x05,       // bDescriptorType (Endpoint)
	0x06,       // bEndpointAddress (OUT Endpoint 6)
	0x02,       // bmAttributes (Bulk)
	0x40, 0x00, // wMaxPacketSize (64 bytes)
	0x01,       // bInterval

	// Endpoint 5: Bulk IN (MSC Data to PC)
	0x07,       // bLength
	0x05,       // bDescriptorType (Endpoint)
	0x85,       // bEndpointAddress (IN Endpoint 5)
	0x02,       // bmAttributes (Bulk)
	0x40, 0x00, // wMaxPacketSize (64 bytes)
	0x01,       // bInterval
};

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct language __attribute__((section(".rodata"))) = {
	4,
	3,
	{0x0409}  // Language ID - English US (look in USB_LANGIDs)
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata")))  = {
	sizeof(FUSB_STR_MANUFACTURER),
	3,  // bDescriptorType - String Descriptor (0x03)
	FUSB_STR_MANUFACTURER
};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata")))  = {
	sizeof(FUSB_STR_PRODUCT),
	3,
	FUSB_STR_PRODUCT
};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata")))  = {
	sizeof(FUSB_STR_SERIAL),
	3,
	FUSB_STR_SERIAL
};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct {
	uint32_t	lIndexValue;  // (uint16_t)Index of a descriptor in config or Language ID for string descriptors | (uint8_t)Descriptor type | (uint8_t)Type of string descriptor
	const uint8_t	*addr;
	uint8_t		length;
} descriptor_list[] = {
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	// {0x00002100, config_descriptor + 18, 9 }, // Not sure why, this seems to be useful for Windows + Android.

	{0x00000300, (const uint8_t *)&language, 4},
	{0x04090301, (const uint8_t *)&string1, string1.bLength},
	{0x04090302, (const uint8_t *)&string2, string2.bLength},
	{0x04090303, (const uint8_t *)&string3, string3.bLength}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )


#endif

