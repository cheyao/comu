#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "ch32fun.h"
#include "funconfig.h"

#define FUSB_BUFFERS_NUMBER 5	      // Number of EP buffers (one for EP0, one per each IN/OUT, two for double)
#define FUSB_EP1_MODE USBD_EP_MODE_TX // IN  - CDC notifications
#define FUSB_EP2_MODE USBD_EP_MODE_RX // OUT - CDC data
#define FUSB_EP3_MODE USBD_EP_MODE_TX // IN  - CDC data (printf)
#define FUSB_EP4_MODE USBD_EP_MODE_TX // IN  - HID
#define FUSB_SUPPORTS_SLEEP 0
#define FUSB_HID_INTERFACES 3 // Indexed by interface number, HID is interface 2
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
#define FUSB_STR_MANUFACTURER u"Icy Elec"
#define FUSB_STR_PRODUCT u"Comu"
#define FUSB_STR_SERIAL u"001"

// Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, // bLength - Length of this descriptor
	1,  // bDescriptorType - Type (Device)
	0x10,
	0x02, // bcdUSB - USB2.1, required for the host to ask for the BOS descriptor
	0xEF, // bDeviceClass - Miscellaneous (composite device with IAD)
	0x02, // bDeviceSubClass - Common Class
	0x01, // bDeviceProtocol - Interface Association Descriptor
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

static const uint8_t bos_descriptor[] = {
	// BOS Header
	0x05, // bLength
	0x0F, // bDescriptorType - BOS
	0x39,
	0x00, // wTotalLength 57
	0x02, // bNumDeviceCaps

	// WebUSB Platform Capability Descriptor
	0x18, // bLength
	0x10, // bDescriptorType - Device Capability
	0x05, // bDevCapabilityType - Platform
	0x00, // bReserved
	0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47, // PlatformCapabilityUUID {3408b638-09a9-47a0-8bfd-a0768815b665}
	0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
	0x00,
	0x01,		     // bcdVersion - WebUSB 1.0
	0x40,  // bVendorCode
	0x01, // iLandingPage

	// Microsoft OS 2.0 Platform Capability Descriptor - makes Windows load WinUSB, see ms_os_20_descriptor
	0x1C, // bLength
	0x10, // bDescriptorType - Device Capability
	0x05, // bDevCapabilityType - Platform
	0x00, // bReserved
	0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C, // PlatformCapabilityUUID {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F}
	0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
	0x00, 0x00, 0x03, 0x06, // dwWindowsVersion - Windows 8.1 (0x06030000)
	0xB2,
	0x00, // wMSOSDescriptorSetTotalLength 178
	0x41, // bMS_VendorCode
	0x00, // bAltEnumCode
};

// Sent in response to the WebUSB GET_URL vendor request, see HandleSetupCustom
static const uint8_t url_descriptor[] = {
	0x14, // bLength
	0x03, // bDescriptorType - WEBUSB_URL
	0x01, // bScheme - https://
	'i', 'c', 'y', '.', 'c', 'o', 'm', 'p', 'u', 't', 'e', 'r', '/', 'c', 'o', 'm', 'u',
};

// Microsoft OS 2.0 Descriptor Set, sent in response to the MS_OS_20_DESCRIPTOR_INDEX vendor request, see HandleSetupCustom
// Binds WinUSB to the vendor interface (3) only, so CDC and HID keep their usual Windows drivers
// https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-os-2-0-descriptors-specification
static const uint8_t ms_os_20_descriptor[] = {
	// Descriptor Set Header
	0x0A,
	0x00, // wLength 10
	0x00,
	0x00, // wDescriptorType - MS_OS_20_SET_HEADER_DESCRIPTOR
	0x00, 0x00, 0x03, 0x06, // dwWindowsVersion - Windows 8.1 (0x06030000)
	0xB2,
	0x00, // wTotalLength 178

	// Configuration Subset Header
	0x08,
	0x00, // wLength 8
	0x01,
	0x00, // wDescriptorType - MS_OS_20_SUBSET_HEADER_CONFIGURATION
	0x00, // bConfigurationValue - index of the configuration, not bConfigurationValue
	0x00, // bReserved
	0xA8,
	0x00, // wTotalLength 168

	// Function Subset Header
	0x08,
	0x00, // wLength 8
	0x02,
	0x00, // wDescriptorType - MS_OS_20_SUBSET_HEADER_FUNCTION
	0x03, // bFirstInterface 3
	0x00, // bReserved
	0xA0,
	0x00, // wSubsetLength 160

	// Compatible ID Descriptor
	0x14,
	0x00, // wLength 20
	0x03,
	0x00, // wDescriptorType - MS_OS_20_FEATURE_COMPATBLE_ID
	'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, // CompatibleID
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SubCompatibleID

	// Registry Property Descriptor
	0x84,
	0x00, // wLength 132
	0x04,
	0x00, // wDescriptorType - MS_OS_20_FEATURE_REG_PROPERTY
	0x07,
	0x00, // wPropertyDataType - REG_MULTI_SZ
	0x2A,
	0x00, // wPropertyNameLength 42
	'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, // PropertyName - "DeviceInterfaceGUIDs"
	't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,
	'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
	0x50,
	0x00, // wPropertyDataLength 80
	'{', 0x00, '7', 0x00, '1', 0x00, 'F', 0x00, '6', 0x00, 'A', 0x00, 'C', 0x00, '1', 0x00, // PropertyData - "{71F6AC1F-275E-45C1-B10C-710A5E19FF89}"
	'F', 0x00, '-', 0x00, '2', 0x00, '7', 0x00, '5', 0x00, 'E', 0x00, '-', 0x00, '4', 0x00,
	'5', 0x00, 'C', 0x00, '1', 0x00, '-', 0x00, 'B', 0x00, '1', 0x00, '0', 0x00, 'C', 0x00,
	'-', 0x00, '7', 0x00, '1', 0x00, '0', 0x00, 'A', 0x00, '5', 0x00, 'E', 0x00, '1', 0x00,
	'9', 0x00, 'F', 0x00, 'F', 0x00, '8', 0x00, '9', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t HIDAPIRepDesc[] = {
	HID_USAGE_PAGE(0xff), // Vendor-defined page.
	HID_USAGE(0x00),
	HID_REPORT_SIZE(8),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	HID_REPORT_COUNT(255),
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
	0x6D,
	0x00, // wTotalLength 109
	0x04, // bNumInterfaces 4
	0x01, // bConfigurationValue
	0x00, // iConfiguration (String Index)
	0xA0, // bmAttributes
	0x32, // bMaxPower 100mA

	// Interface Association Descriptor - groups the two CDC interfaces together
	0x08, // bLength
	0x0B, // bDescriptorType - Interface Association
	0x00, // bFirstInterface 0
	0x02, // bInterfaceCount 2
	0x02, // bFunctionClass - CDC
	0x02, // bFunctionSubClass - Abstract Control Model
	0x01, // bFunctionProtocol - AT Commands
	0x00, // iFunction

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

	// Interface Descriptor (HIDAPI)
	0x09, // bLength
	0x04, // bDescriptorType
	0x02, // bInterfaceNumber 2
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
	0x84, // bEndpointAddress: IN Endpoint 4
	0x03, // bmAttributes (Interrupt)
	0x40,
	0x00, // wMaxPacketSize
	0x01, // bInterval: 1mS

	// Vendor Interface (WebUSB) - no kernel driver claims it, WinUSB on Windows via ms_os_20_descriptor
	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x03, // bInterfaceNumber 3
	0x00, // bAlternateSetting
	0x00, // bNumEndpoints 0 - control transfers only
	0xFF, // bInterfaceClass - Vendor Specific
	0x00, // bInterfaceSubClass
	0x00, // bInterfaceProtocol
	0x00, // iInterface
};
_Static_assert(sizeof(config_descriptor) == 109, "wTotalLength in config_descriptor is out of date");
_Static_assert(sizeof(bos_descriptor) == 57, "wTotalLength in bos_descriptor is out of date");
_Static_assert(sizeof(ms_os_20_descriptor) == 178, "wTotalLength in ms_os_20_descriptor is out of date");

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
	uint32_t lIndexValue; // (uint16_t)Index of a descriptor in config or Language ID for string descriptors | (uint8_t)Descriptor type | (uint8_t)Type of string descriptor
	const uint8_t* addr;
	uint8_t length;
} descriptor_list[] = {
	// List of descriptors
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	// interface number // 2200 for hid descriptors.
	{0x00022200, HIDAPIRepDesc, sizeof(HIDAPIRepDesc)},

	{0x00022100, config_descriptor + 84, 9}, // HID descriptor of interface 2. Not sure why, this seems to be useful for Windows + Android.

	{0x00000F00, bos_descriptor, sizeof(bos_descriptor)},

	{0x00000300, (const uint8_t*)&language, 4},
	{0x04090301, (const uint8_t*)&string1, string1.bLength},
	{0x04090302, (const uint8_t*)&string2, string2.bLength},
	{0x04090303, (const uint8_t*)&string3, string3.bLength},
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list)) / (sizeof(struct descriptor_list_struct)))

#endif
