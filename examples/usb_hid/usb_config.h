#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "ch32fun.h"
#include "funconfig.h"

#define FUSB_BUFFERS_NUMBER 4	      // Number of EP buffers (one for EP0, one per each IN/OUT, two for double)
#define FUSB_EP1_MODE USBD_EP_MODE_TX // IN
#define FUSB_EP2_MODE USBD_EP_MODE_RX // OUT
#define FUSB_EP3_MODE USBD_EP_MODE_TX // IN
#define FUSB_SUPPORTS_SLEEP 0
#define FUSB_HID_INTERFACES 2
#define FUSB_CURSED_TURBO_DMA 0 // Hacky, but seems fine, shaves 2.5us off filling 64-byte buffers.
#define FUSB_HID_USER_REPORTS 1
#define FUSB_IO_PROFILE 0
#define FUSB_USE_HPE FUNCONF_ENABLE_HPE
#define FUSB_USER_HANDLERS 1
#define FUSB_USE_DMA7_COPY 0
#define FUSB_VDD_5V FUNCONF_USE_5V_VDD
#define FUSB_FROM_RAM 0

#include "usb_defines.h"

#define FUSB_USB_VID 0x1209
#define FUSB_USB_PID 0xd035
#define FUSB_USB_REV 0x0007
#define FUSB_STR_MANUFACTURER u"cheyao"
#define FUSB_STR_PRODUCT u"USB TTY"
#define FUSB_STR_SERIAL u"007"

// Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, // bLength - Length of this descriptor
	1,  // bDescriptorType - Type (Device)
	0x10,
	0x01, // bcdUSB - The highest USB spec version this device supports (USB1.1)
	0x0,  // bDeviceClass - Device Class
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
	1,			      // bNumConfigurations - Max number of configurations (if more then 1, you can switch between them)
};

/* Keyboard Report Descriptor */
static const uint8_t KeyRepDesc[] = {
	0x05, 0x01,	  // Usage Page (Generic Desktop)
	0x09, 0x06,	  // Usage (Keyboard)
	0xA1, 0x01,	  // Collection (Application)
	0x05, 0x07,	  // Usage Page (Key Codes)
	0x19, 0xE0,	  // Usage Minimum (224)
	0x29, 0xE7,	  // Usage Maximum (231)
	0x15, 0x00,	  // Logical Minimum (0)
	0x25, 0x01,	  // Logical Maximum (1)
	0x75, 0x01,	  // Report Size (1)
	0x95, 0x08,	  // Report Count (8)
	0x81, 0x02,	  // Input (Data,Variable,Absolute)
	0x95, 0x01,	  // Report Count (1)
	0x75, 0x08,	  // Report Size (8)
	0x81, 0x01,	  // Input (Constant)
	0x95, 0x03,	  // Report Count (3)
	0x75, 0x01,	  // Report Size (1)
	0x05, 0x08,	  // Usage Page (LEDs)
	0x19, 0x01,	  // Usage Minimum (1)
	0x29, 0x03,	  // Usage Maximum (3)
	0x91, 0x02,	  // Output (Data,Variable,Absolute)
	0x95, 0x05,	  // Report Count (5)
	0x75, 0x01,	  // Report Size (1)
	0x91, 0x01,	  // Output (Constant,Array,Absolute)
	0x95, 0x06,	  // Report Count (6)
	0x75, 0x08,	  // Report Size (8)
	0x26, 0xFF, 0x00, // Logical Maximum (255)
	0x05, 0x07,	  // Usage Page (Key Codes)
	0x19, 0x00,	  // Usage Minimum (0)
	0x29, 0x91,	  // Usage Maximum (145)
	0x81, 0x00,	  // Input(Data,Array,Absolute)
	0xC0		  // End Collection
};

/* Mouse Report Descriptor */
static const uint8_t MouseRepDesc[] = {
	0x05, 0x01, // Usage Page (Generic Desktop)
	0x09, 0x02, // Usage (Mouse)
	0xA1, 0x01, // Collection (Application)
	0x09, 0x01, // Usage (Pointer)
	0xA1, 0x00, // Collection (Physical)
	0x05, 0x09, // Usage Page (Button)
	0x19, 0x01, // Usage Minimum (Button 1)
	0x29, 0x03, // Usage Maximum (Button 3)
	0x15, 0x00, // Logical Minimum (0)
	0x25, 0x01, // Logical Maximum (1)
	0x75, 0x01, // Report Size (1)
	0x95, 0x03, // Report Count (3)
	0x81, 0x02, // Input (Data,Variable,Absolute)
	0x75, 0x05, // Report Size (5)
	0x95, 0x01, // Report Count (1)
	0x81, 0x01, // Input (Constant,Array,Absolute)
	0x05, 0x01, // Usage Page (Generic Desktop)
	0x09, 0x30, // Usage (X)
	0x09, 0x31, // Usage (Y)
	0x09, 0x38, // Usage (Wheel)
	0x15, 0x81, // Logical Minimum (-127)
	0x25, 0x7F, // Logical Maximum (127)
	0x75, 0x08, // Report Size (8)
	0x95, 0x03, // Report Count (3)
	0x81, 0x06, // Input (Data,Variable,Relative)
	0xC0,	    // End Collection
	0xC0	    // End Collection
};

static const uint8_t HIDAPIRepDesc[] = {
	HID_USAGE_PAGE(0xff), // Vendor-defined page.
	HID_USAGE(0x00),
	HID_REPORT_SIZE(64),
	HID_COLLECTION(HID_COLLECTION_LOGICAL),
	HID_REPORT_COUNT(254),
	HID_REPORT_ID(0xaa) HID_USAGE(0x01),
	HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
	HID_REPORT_COUNT(63), // For use with `hidapitester --vidpid 1209/D003 --open --read-feature 171`
	HID_REPORT_ID(0xab) HID_USAGE(0x01),
	HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
	HID_COLLECTION_END,
};

/* Configuration Descriptor Set */
static const uint8_t config_descriptor[] = {
	0x09, // bLength
	0x02, // bDescriptorType (Configuration)
	0x8E,
	0x00, // wTotalLength 142
	0x05, // bNumInterfaces 5
	0x01, // bConfigurationValue
	0x00, // iConfiguration (String Index)
	0xA0, // bmAttributes
	0x32, // bMaxPower 100mA

	// CDC Interface
	0x09, // bLength
	0x04, // bDescriptorType - Interface
	0x00, // bInterfaceNumber - 0
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints - 1
	0x02, // bInterfaceClass - CDC
	0x02, // bInterfaceSubClass - Abstract Control Model (Table 4 in CDC120.pdf)
	0x01, // bInterfaceProtocol - AT Commands: V.250 etc (Table 5)
	0x00, // iInterface (String Index)

	// Setting up CDC interface (Table 18)
	0x05, // bLength
	0x24, // bDescriptorType - CS_INTERFACE (Table 12)
	0x00, // bDescriptorSubType - Header Functional Descriptor (Table 13)
	0x10,
	0x01, // bcdCDC - USB version - USB1.1
	// Call Management Functional Descriptor
	0x05, // bLength
	0x24, // bDescriptorType - CS_INTERFACE
	0x01, // bDescriptorSubType - Call Management Functional Descriptor (Table 13)
	0x00, // bmCapabilities: (Table 3 in PSTN120.pdf)
	// Bit 0 — Device handles call management itself:
	//  1 = device handles call management (e.g. call setup, termination, etc.)
	//  0 = host handles it
	// Bit 1 — Device can send/receive call management information over a Data Class interface:
	//  1 = can use the Data Class interface for call management
	//  0 = must use the Communication Class interface
	0x01, // bDataInterface - Indicates that multiplexed commands are handled via data interface 01h (same value as used in the UNION Functional Descriptor)
	// Abstract Control Management Functional Descriptor
	0x04, // bLength
	0x24, // bDescriptorType - CS_INTERFACE
	0x02, // bDescriptorSubType - Abstract Control Management Functional Descriptor (Table 13)
	0x02, // bmCapabilities - Device supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification
	      // Serial_State (Table 4 in PSTN120.pdf)
	// Union Descriptor Functional Descriptor
	0x05, // bLength
	0x24, // bDescriptorType - CS_INTERFACE
	0x06, // bDescriptorSubType - Union Descriptor Functional Descriptor (Table 13)
	0x00, // bControlInterface (Interface number of the control (Communications Class) interface)
	0x01, // bSubordinateInterface0 (Interface number of the subordinate (Data Class) interface)
	// Setting up EP1 for CDC config interface
	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x81, // bEndpointAddress (IN/D2H)
	0x03, // bmAttributes (Interrupt)
	0x40,
	0x00, // wMaxPacketSize 64
	0x01, // bInterval 1 (unit depends on device speed)

	// Transmission interface with two bulk endpoints
	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x01, // bInterfaceNumber 1
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints 2
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0x00, // bInterfaceProtocol - Transparent
	0x00, // iInterface (String Index)
	// EP2 - device to host
	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x02, // bEndpointAddress (OUT/H2D)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)
	// EP3 - host to device
	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x83, // bEndpointAddress (IN/D2H)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)

    // TODO: Change EPs
	// Interface Descriptor (HIDAPI)
	0x09, // bLength
	0x04, // bDescriptorType
	0x00, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x03, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0xff, // bInterfaceProtocol: OTher
	0x02, // iInterface

	// HID Descriptor (HIDAPI)
	0x09, // bLength
	0x21, // bDescriptorType
	0x10,
	0x01, // bcdHID
	0x00, // bCountryCode
	0x01, // bNumDescriptors
	0x22, // bDescriptorType
	sizeof(HIDAPIRepDesc),
	0x00, // wDescriptorLength

	// Endpoint Descriptor (HIDAPI)
	0x07, // bLength
	0x05, // bDescriptorType
	0x83, // bEndpointAddress: IN Endpoint 2
	0x03, // bmAttributes
	0x40,
	0x00, // wMaxPacketSize
	0x01, // bInterval: 255mS

	// Interface Descriptor (Keyboard)
	0x09, // bLength
	0x04, // bDescriptorType
	0x01, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x03, // bInterfaceClass
	0x01, // bInterfaceSubClass
	0x01, // bInterfaceProtocol: Keyboard
	0x00, // iInterface

	// HID Descriptor (Keyboard)
	0x09, // bLength
	0x21, // bDescriptorType
	0x10,
	0x01, // bcdHID
	0x00, // bCountryCode
	0x01, // bNumDescriptors
	0x22, // bDescriptorType
	sizeof(KeyRepDesc),
	0x00, // wDescriptorLength

	// Endpoint Descriptor (Keyboard)
	0x07, // bLength
	0x05, // bDescriptorType
	0x81, // bEndpointAddress: IN Endpoint 1
	0x03, // bmAttributes
	0x08,
	0x00, // wMaxPacketSize
	0xff, // bInterval: 255mS

	// Interface Descriptor (Mouse)
	0x09, // bLength
	0x04, // bDescriptorType
	0x02, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x03, // bInterfaceClass
	0x01, // bInterfaceSubClass
	0x02, // bInterfaceProtocol: Mouse
	0x00, // iInterface

	// HID Descriptor (Mouse)
	0x09, // bLength
	0x21, // bDescriptorType
	0x10,
	0x01, // bcdHID
	0x00, // bCountryCode
	0x01, // bNumDescriptors
	0x22, // bDescriptorType
	sizeof(MouseRepDesc),
	0x00, // wDescriptorLength

	// Endpoint Descriptor (Mouse)
	0x07, // bLength
	0x05, // bDescriptorType
	0x82, // bEndpointAddress: IN Endpoint 2
	0x03, // bmAttributes
	0x08,
	0x00, // wMaxPacketSize
	0xff, // bInterval: 255mS
};

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct language __attribute__((section(".rodata"))) = {
	4, 3, {0x0409} // Language ID - English US (look in USB_LANGIDs)
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata"))) = {sizeof(FUSB_STR_MANUFACTURER),
												3, // bDescriptorType - String Descriptor (0x03)
												FUSB_STR_MANUFACTURER};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata"))) = {sizeof(FUSB_STR_PRODUCT), 3, FUSB_STR_PRODUCT};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata"))) = {sizeof(FUSB_STR_SERIAL), 3, FUSB_STR_SERIAL};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct {
	uint32_t lIndexValue; // (uint16_t)Index of a descriptor in config or Language ID for string descriptors | (uint8_t)Descriptor type | (uint8_t)Type of
			      // string descriptor
	const uint8_t* addr;
	uint8_t length;
} descriptor_list[] = {
	// List of descriptors
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	// interface number // 2200 for hid descriptors.
	{0x00002200, HIDAPIRepDesc, sizeof(HIDAPIRepDesc)},
	{0x00012200, KeyRepDesc, sizeof(KeyRepDesc)},
	{0x00022200, MouseRepDesc, sizeof(MouseRepDesc)},

	{0x00002100, config_descriptor + 18, 9}, // Not sure why, this seems to be useful for Windows + Android.

	{0x00000300, (const uint8_t*)&language, 4},
	{0x04090301, (const uint8_t*)&string1, string1.bLength},
	{0x04090302, (const uint8_t*)&string2, string2.bLength},
	{0x04090303, (const uint8_t*)&string3, string3.bLength}};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list)) / (sizeof(struct descriptor_list_struct)))

#endif
