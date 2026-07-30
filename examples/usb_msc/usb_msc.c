#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "usbd.h"

#define PIN_LED PA15
#define LED_ON 1

#define BLINK_DELAY 100

#define EP_CDC_IRQ 1
#define EP_CDC_OUT 2
#define EP_CDC_IN  3
#define EP_MSC_OUT 6
#define EP_MSC_IN  5

#define MSC_RAM_DISK_SIZE   4 * 1024
#define MSC_BLOCK_SIZE      512
#define MSC_BLOCK_COUNT     (MSC_RAM_DISK_SIZE / MSC_BLOCK_SIZE)
#define MSC_TOTAL_SECTORS   0x4000
#define FILE_CLUSTER        2

// RAM Disk Storage
uint8_t msc_ram_disk[MSC_RAM_DISK_SIZE] __attribute__((aligned(4)));

// -----------------------------------------------------------------------------
// SECTOR 0: FAT16 Boot Sector (BPB) for an 8MB Drive
// -----------------------------------------------------------------------------
const uint8_t BootSector[512] = {
	0xEB, 0x3C, 0x90,                               // Jump Instruction
	'c','h','3','2','f','u','n',' ',                // OEM Name
	0x00, 0x02,                                     // Bytes per sector (512)
	0x08,                                           // Sectors per cluster (4KB clusters)
	0x01, 0x00,                                     // Reserved sectors (1)
	0x02,                                           // Number of FATs (2)
	0x00, 0x02,                                     // Root dir entries (512)
	0x00, 0x40,                                     // Total sectors small (16384 = 8MB)
	0xF8,                                           // Media descriptor
	0x10, 0x00,                                     // Sectors per FAT (16)
	0x3F, 0x00,                                     // Sectors per track
	0xFF, 0x00,                                     // Heads
	0x00, 0x00, 0x00, 0x00,                         // Hidden sectors
	0x00, 0x00, 0x00, 0x00,                         // Large sectors
	0x80,                                           // Drive number
	0x00,                                           // Reserved
	0x29,                                           // Ext boot signature
	0x12, 0x34, 0x56, 0x78,                         // Serial Number
	'C','H','3','2','F','U','N',' ',' ',' ',' ',    // Volume Label
	'F', 'A', 'T', '1', '6', ' ', ' ', ' ',         // FS Type
	// Boot code and signature 0x55 0xAA follow...
	[510] = 0x55,
	[511] = 0xAA
};

// -----------------------------------------------------------------------------
// Root Directory Entry for "INDEX.HTM"
// -----------------------------------------------------------------------------
// 8.3 Filename, Attr 0x20 (Archive), Cluster High 0, Time/Date, Cluster Low 2
const uint8_t INDEX_HTM[] = "<!doctype html>\n<html><body><script>location.replace(\"https://github.com/cnlohr/ch32fun\")</script></body></html>\n";
static volatile int file_changed;
static volatile uint16_t active_file_cluster = FILE_CLUSTER;
static volatile uint32_t active_file_size = sizeof(INDEX_HTM) -1;
uint8_t RootDirEntry[32] = {
	'I', 'N', 'D', 'E', 'X', ' ', ' ', ' ', 'H', 'T', 'M',      // 0x00: Name (11)
	0x20,                                                       // 0x0B: Attributes
	0x00,                                                       // 0x0C: Reserved (NT)
	0x00,                                                       // 0x0D: CrtTimeTenth
	0x00, 0x00,                                                 // 0x0E: CrtTime
	0x00, 0x00,                                                 // 0x10: CrtDate
	0x00, 0x00,                                                 // 0x12: Last Access Date
	0x00, 0x00,                                                 // 0x14: High Cluster
	0x21, 0x00,                                                 // 0x16: WrtTime
	0x21, 0x00,                                                 // 0x18: WrtDate
	(FILE_CLUSTER & 0xFF),
	(FILE_CLUSTER >> 8),                                        // 0x1A: Low Cluster (2)
	((sizeof(INDEX_HTM) -1) & 0xFF),
	((sizeof(INDEX_HTM) -1) >> 8),
	((sizeof(INDEX_HTM) -1) >> 16),
	((sizeof(INDEX_HTM) -1) >> 24),                             // 0x1C: Size (Little Endian)
};

// -----------------------------------------------------------------------------
// Disk Layout Constants (Based on BPB above)
// -----------------------------------------------------------------------------
#define START_FAT1      1
#define START_FAT2      17
#define START_ROOT      33  // 1 + 16 + 16
#define START_DATA      65  // 33 + 32 sectors for root dir (512*32/512 = 32)


volatile char cdc_input;

// MSC State Machine
typedef enum {
	MSC_IDLE,       // Waiting for CBW
	MSC_DATA_OUT,   // Receiving data from PC (Write)
	MSC_DATA_IN,    // Sending data to PC (Read/Inquiry)
	MSC_SEND_CSW    // Sending Status Wrapper
} msc_state_t;

volatile msc_state_t msc_state = MSC_IDLE;

// Command Block Wrapper (31 bytes)
struct CBW {
	uint32_t Signature;          // "USBC" (0x43425355)
	uint32_t Tag;
	uint32_t DataTransferLength;
	uint8_t  Flags;
	uint8_t  LUN;
	uint8_t  CBLength;
	uint8_t  CB[16];             // SCSI Command Block
} __attribute__((packed));

// Command Status Wrapper (13 bytes)
struct CSW {
	uint32_t Signature;          // "USBS" (0x53425355)
	uint32_t Tag;
	uint32_t DataResidue;
	uint8_t  Status;             // 0=Pass, 1=Fail, 2=Phase Error
} __attribute__((packed));

volatile struct CBW cbw;
volatile struct CSW csw;

// Transfer tracking
volatile uint32_t msc_current_offset = 0;
volatile uint32_t msc_bytes_remaining = 0;



void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
		funDigitalWrite( PIN_LED, LED_ON ); // Turn on LED
		Delay_Ms(BLINK_DELAY);
		funDigitalWrite( PIN_LED, !LED_ON ); // Turn off LED
		if(i) Delay_Ms(BLINK_DELAY);
	}
}

// -----------------------------------------------------------------------------
// Helper: Logic to determine WHAT to send next
// -----------------------------------------------------------------------------
void MSC_PrepareDataIn(void) {
	uint8_t msc_response_buffer[64]  __attribute__((aligned(4))) = {0}; // Scratch buffer for Inquiry/Sense headers
	uint32_t len_to_send;
	uint8_t *src_ptr = NULL;
	int is_short_transfer = 0;

	// If we are already sending CSW, don't try to send data
	if (msc_state == MSC_SEND_CSW) return;

	// 1. Check if we are done
	if (msc_bytes_remaining == 0) {
		// We are done sending data. Send CSW.
		msc_state = MSC_SEND_CSW;
		while(USBD_SendEndpointNEW(EP_MSC_IN, (uint8_t*)&csw, sizeof(csw), 1) == -1); // -1 == busy
		return;
	}

	// 2. Determine Data Source based on Command
	switch(cbw.CB[0]) {
	// --- STREAMING DATA COMMANDS (From RAM/Flash) ---
	case 0x28: // READ 10
		uint32_t lba_start = (cbw.CB[2] << 24) | (cbw.CB[3] << 16) | (cbw.CB[4] << 8) | cbw.CB[5];

		// Offset relative to current READ_10 command start
		uint32_t lba_offset = (cbw.DataTransferLength - msc_bytes_remaining) / 512;
		uint32_t current_lba = lba_start + lba_offset;

		len_to_send = (msc_bytes_remaining > 64) ? 64 : msc_bytes_remaining;

		// --- 1. Boot Sector ---
		if (current_lba == 0) {
			// Copy slice of BootSector based on byte offset within the sector
			uint32_t byte_offset_in_sector = (cbw.DataTransferLength - msc_bytes_remaining) % 512;
			memcpy(msc_response_buffer, &BootSector[byte_offset_in_sector], len_to_send);
		}

		// --- 2. FAT Tables (Sectors 1..32) ---
		else if (current_lba >= START_FAT1 && current_lba < START_ROOT) {
			// In FAT16, Cluster 0 and 1 are reserved (0xFFF8, 0xFFFF).
			// Cluster 2 is our file. We mark it as EOF (0xFFFF).
			// All other clusters are 0 (Free).

			// We only need to generate data for the very beginning of the FAT sector
			uint32_t byte_offset_in_sector = (cbw.DataTransferLength - msc_bytes_remaining) % 512;

			// Construct the first few bytes of the FAT table on the fly
			// Entry 0: F8 FF, Entry 1: FF FF, Entry 2: FF FF (EOF for INDEX.HTM)
			const uint8_t fat_head[] = { 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

			// Only relevant if we are reading the very first sector of FAT1 or FAT2
			if ((current_lba == START_FAT1 || current_lba == START_FAT2) && byte_offset_in_sector < 6) {
				for(int i=0; i<len_to_send; i++) {
					if (byte_offset_in_sector + i < 6) {
						msc_response_buffer[i] = fat_head[byte_offset_in_sector + i];
					}
				}
			}
		}

		// --- 3. Root Directory (Sectors 33..64) ---
		else if (current_lba >= START_ROOT && current_lba < START_DATA) {
			// First entry is Volume Label (Optional), Second is INDEX.HTM
			uint32_t byte_offset_in_sector = (cbw.DataTransferLength - msc_bytes_remaining) % 512;

			// We are sending the Root Directory with active file cluster = 2
			if (current_lba == START_ROOT) {
				active_file_cluster = FILE_CLUSTER;
				*(uint32_t*)(RootDirEntry +28) = active_file_size;
			}

			// Only the first sector of Root Dir contains our entry
			if (current_lba == START_ROOT) {
				// If requesting bytes 0..31 -> INDEX.HTM entry
				for(int i = 0; i < len_to_send; i++) {
					int pos = byte_offset_in_sector + i;
					if (pos < 32) {
						msc_response_buffer[i] = RootDirEntry[pos];
					}
				}
			}
		}

		// --- 4. Data Area (Cluster 2 starts at START_DATA) ---
		else if (current_lba >= START_DATA) {
			// Map LBA to RAM buffer offset
			// LBA 65 -> RAM 0
			uint32_t ram_sector_idx = current_lba - START_DATA;
			uint32_t ram_offset = (ram_sector_idx * 512) + ((cbw.DataTransferLength - msc_bytes_remaining) % 512);

			if (ram_offset + len_to_send <= MSC_RAM_DISK_SIZE) {
				memcpy(msc_response_buffer, &msc_ram_disk[ram_offset], len_to_send);
			}
            //printf("A: %c %c\n", msc_ram_disk[0], msc_ram_disk[1]);
		}

		// Point to stack buffer
		src_ptr = msc_response_buffer;
		break;

	// --- ONE-SHOT COMMANDS (Generated Headers) ---
	// Note: We only construct these ONCE. If requested len > 64, we send what we have
	// and tell the state machine we are done.
	case 0x03: // REQUEST SENSE
		// The OS expects 18 bytes of status data.
		// We return "No Sense" (All zeros), meaning everything is fine.
		msc_response_buffer[0] = 0x70; // Response Code (Current, Fixed)
		msc_response_buffer[2] = 0x00; // Sense Key (No Sense)
		msc_response_buffer[7] = 0x0A; // Additional Sense Length (10)
		// The rest are 0x00, which is correct for "No Error"

		len_to_send = 18;
		src_ptr = msc_response_buffer;
		is_short_transfer = 1;
		break;

	case 0x12: // INQUIRY
		msc_response_buffer[0] = 0x00; // Direct Access Device
		msc_response_buffer[1] = 0x80; // Removable
		msc_response_buffer[2] = 0x02; // Version
		msc_response_buffer[3] = 0x02; // Format
		msc_response_buffer[4] = 32;   // Additional Length
		memcpy(&msc_response_buffer[8],  "WCH     ", 8);
		memcpy(&msc_response_buffer[16], "ch32fun_MSC     ", 16);
		memcpy(&msc_response_buffer[32], "1.00", 4);

		uint32_t actual_len = 37;
		len_to_send = (msc_bytes_remaining < actual_len) ? msc_bytes_remaining : actual_len;
		src_ptr = msc_response_buffer;
		is_short_transfer = 1; // We only have 36 bytes. Stop after this.
		break;

	case 0x25: // READ CAPACITY 10
		// Big Endian conversion
		*(uint32_t*)&msc_response_buffer[0] = __builtin_bswap32(MSC_TOTAL_SECTORS - 1);
		*(uint32_t*)&msc_response_buffer[4] = __builtin_bswap32(MSC_BLOCK_SIZE);

		len_to_send = 8;
		src_ptr = msc_response_buffer;
		is_short_transfer = 1;
		break;

	case 0x1A: // MODE SENSE 6
		memset(msc_response_buffer, 0, 4);
		msc_response_buffer[0] = 3;
		len_to_send = 4;
		src_ptr = msc_response_buffer;
		is_short_transfer = 1;
		break;

	case 0x5A: // MODE SENSE 10
		memset(msc_response_buffer, 0, 8);
		msc_response_buffer[1] = 6;
		len_to_send = 8;
		src_ptr = msc_response_buffer;
		is_short_transfer = 1;
		break;

	default:
		// Unknown or unhandled read - Send Zeros or Stall
		// For safety, send CSW
		len_to_send = 0;
		break;
	}

	// 3. Send the Packet
	if (len_to_send > 0 && src_ptr != NULL) {
		while(USBD_SendEndpointNEW(EP_MSC_IN, src_ptr, len_to_send, 1) == -1); // -1 == busy

		// 4. Update State
		if (is_short_transfer) {
			// If we sent all REAL data we have, but Host wanted more (e.g. Inquiry 255 bytes),
			// We set Residue to what we didn't send, and set remaining to 0 so next IRQ sends CSW.
			csw.DataResidue = msc_bytes_remaining - len_to_send;
			msc_bytes_remaining = 0; 
		}
		else {
			// Streaming (READ_10): Just subtract what we sent.
			// If remaining becomes 0, next IRQ sends CSW.
			msc_bytes_remaining -= len_to_send;
			msc_current_offset += len_to_send;
		}
	}
	else {
		// Fallback: Done/Error, send CSW immediately
		msc_state = MSC_SEND_CSW;
		while(USBD_SendEndpointNEW(EP_MSC_IN, (uint8_t*)&csw, sizeof(csw), 1) == -1); // -1 == busy
	}
}

// -----------------------------------------------------------------------------
// IN Handler (Called by IRQ when Packet Sent to PC)
// -----------------------------------------------------------------------------
int HandleInRequest(struct _USBState *ctx, int endp, uint8_t *data, int len) {
	if ((endp == EP_MSC_IN) && (msc_state == MSC_SEND_CSW)) {
		// Host grabbed the CSW. We are IDLE.
		msc_state = MSC_IDLE;
	}
	return 0;
}

// -----------------------------------------------------------------------------
// OUT Handler (Called by IRQ when Packet Received from PC)
// -----------------------------------------------------------------------------
void HandleDataOut(struct _USBState *ctx, int endp, uint8_t *data, int len) {
	if (endp == 0) {
		ctx->USBD_SetupReqLen = 0;
	}
	else if( endp == EP_CDC_OUT ) {
		// cdc tty input
		int headroom = (sizeof(usb_inputbuffer) - usb_inbuf_idx) - len;
		if(headroom < 0) {
			// not enough space left, free up some
			int offset = -headroom;
			for(int i = offset; i < sizeof(usb_inputbuffer); i++) {
				usb_inputbuffer[i -offset] = usb_inputbuffer[i];
			}
			usb_inbuf_idx -= offset;
		}

		for(int i = 0; i < len; i++) {
			usb_inputbuffer[usb_inbuf_idx++] = data[i];
		}
	}
	else if (endp == EP_MSC_OUT) {
		
		// --- 1. IDLE State: Expecting CBW ---
		if (msc_state == MSC_IDLE) {
			if (len != 31) return; // Invalid CBW

			memcpy((void*)&cbw, data, 31);
			if (cbw.Signature != 0x43425355) return; // Invalid Sig

			// Initialize CSW
			csw.Signature = 0x53425355;
			csw.Tag = cbw.Tag;
			csw.DataResidue = 0;
			csw.Status = 0;

			uint32_t lba = (cbw.CB[2] << 24) | (cbw.CB[3] << 16) | (cbw.CB[4] << 8) | cbw.CB[5];
			uint32_t blocks = (cbw.CB[7] << 8) | cbw.CB[8];

			switch (cbw.CB[0]) {
			// -- DATA IN COMMANDS --
			case 0x03: // REQUEST SENSE
			case 0x12: // INQUIRY
			case 0x25: // READ CAPACITY
			case 0x1A: // MODE SENSE 6
			case 0x5A: // MODE SENSE 10
				msc_state = MSC_DATA_IN;
				msc_current_offset = 0; // Not used for these, but good practice
				msc_bytes_remaining = cbw.DataTransferLength;
				MSC_PrepareDataIn(); // Send First Packet
				break;

			case 0x28: // READ 10
				msc_state = MSC_DATA_IN;
				msc_current_offset = lba * MSC_BLOCK_SIZE;
				msc_bytes_remaining = blocks * MSC_BLOCK_SIZE;
				MSC_PrepareDataIn(); // Send First Packet
				break;

			// -- DATA OUT COMMANDS --
			case 0x2A: // WRITE 10
				msc_state = MSC_DATA_OUT;
				msc_current_offset = lba * MSC_BLOCK_SIZE;
				msc_bytes_remaining = blocks * MSC_BLOCK_SIZE;
				break;

			// -- NO DATA COMMANDS --
			case 0x00: // TEST UNIT READY
			case 0x1E: // PREVENT_ALLOW_REMOVAL
			default:
				// If Host expects data (Flags 0x80) but we don't support it, send Zero/Stall.
				// Simplified: Just send CSW immediately.
				if(cbw.DataTransferLength > 0 && (cbw.Flags & 0x80)) {
					 // Some hosts hate stalls, so we can send a zero packet then status?
					 // For now, strict CSW failure:
					 csw.Status = 1; 
				}
				msc_state = MSC_SEND_CSW;
				while(USBD_SendEndpointNEW(EP_MSC_IN, (uint8_t*)&csw, sizeof(csw), 1) == -1); // -1 == busy
				break;
			}
		} 

		// --- 2. DATA OUT State: Receiving Data from PC ---
		else if (msc_state == MSC_DATA_OUT) {
			uint32_t write_len = (len < msc_bytes_remaining) ? len : msc_bytes_remaining;

			uint32_t current_lba = msc_current_offset / 512;

			// --- CASE A: Root Directory Update (Snoop for Filename) ---
			if (current_lba >= START_ROOT && current_lba < START_DATA) {
				// The OS is writing to the Directory. We need to see if it's moving our file.
				// Each entry is 32 bytes.
				for (int i = 0; i < write_len; i += 32) {
					// Check for "INDEX   HTM" (11 chars) at offset 0
					if (memcmp(data + i, "INDEX   HTM", 11) == 0) {
						// Found our file! Extract the Starting Cluster (Offset 0x1A / 26)

						uint16_t new_cluster = data[i + 26] | (data[i + 27] << 8);
						// If the OS sets cluster to 0, it's deleting/truncating. Ignore that.
						if (new_cluster != 0) {
							active_file_cluster = new_cluster;

							// Snoop Size (Offset 0x1C)
							uint32_t new_size = *(uint32_t*)&data[i + 0x1C];
							active_file_size = (new_size > MSC_RAM_DISK_SIZE) ? MSC_RAM_DISK_SIZE : new_size;
							file_changed = 1;
						}
					}
				}
				// We don't actually store directory writes in this Ghost FS, we just observe them.
			}

			// --- CASE B: Data Area Write (Filter by Cluster) ---
			else if (current_lba >= START_DATA) {
				// Calculate which cluster this write belongs to.
				// BPB says 8 sectors per cluster.
				// Cluster 2 starts at START_DATA.
				uint32_t target_cluster = 2 + (current_lba - START_DATA) / 8;

				// FILTER: Only allow write if it matches the cluster the OS assigned to our file.
				if (target_cluster >= active_file_cluster) {
					// Calculate offset within the 4KB RAM buffer
					// (LBA % 8) gives sector offset within the cluster
					uint32_t sector_offset = (current_lba - START_DATA) % 8;
					uint32_t byte_offset = (sector_offset * 512) + (msc_current_offset % 512);

					if (byte_offset + write_len <= MSC_RAM_DISK_SIZE) {
						memcpy(&msc_ram_disk[byte_offset], data, write_len);
					}
                    printf("B: %c %c\n", msc_ram_disk[0], msc_ram_disk[1]);
				}
				// If target_cluster != active_file_cluster, it's garbage (metadata), ignore it.
			}

			msc_current_offset += write_len;
			msc_bytes_remaining -= write_len;

			if (msc_bytes_remaining == 0) {
				msc_state = MSC_SEND_CSW;
				while(USBD_SendEndpointNEW(EP_MSC_IN, (uint8_t*)&csw, sizeof(csw), 1) == -1); // -1 == busy
			}
		}
	}
}

int HandleSetupCustom( struct _USBState *ctx, int setup_code ) {
	int ret = -1;
	if ( ctx->USBD_SetupReqType & USB_REQ_TYP_CLASS ) {
		switch ( setup_code ) {
		case CDC_SET_LINE_CODING:
		case CDC_SET_LINE_CTLSTE:
		case CDC_SEND_BREAK:
			ret = ( ctx->USBD_SetupReqLen ) ? ctx->USBD_SetupReqLen : -1;
			break;
		case CDC_GET_LINE_CODING:
			ret = ctx->USBD_SetupReqLen;
			break;
		case 0xFE: // MSC_GET_MAX_LUN
			ctx->pCtrlPayloadPtr = CTRL0BUFF; // should not be needed, but it is for now
			ctx->pCtrlPayloadPtr[0] = 0;
			ret = 1;
			break;
		default:
			ret = 0;
			break;
		}
	}
	else {
		ret = 0; // Go to STALL
	}
	return ret;
}

void GPIOSetup() {
	funPinMode( PIN_LED,    GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_LED to output
}

void HandleUSBInput(int numbytes, uint8_t *data) {
	if(numbytes == 1) {
		putchar(data[0]);
		if(data[0] > '0' && data[0] <= '9') {
			blink(data[0] -'0');
		}
		else if(data[0] == '?') {
			printf("\n\r");
			printf("Active file (len=%ld) on cluster %d:\n\r", active_file_size, active_file_cluster);
			for(int i = 0; i < active_file_size; i += 64) {
				_write(0, (char*)msc_ram_disk +i, (((active_file_size -i) > 64) ? 64 : (active_file_size -i)));
			}
			printf("\n\r");
		}
		data[0] = 0;
	} else {
		_write(0, (const char*)data, numbytes);
	}
}

int main() {
	SystemInit();

	funGpioInitAll();
	GPIOSetup();

	memcpy(msc_ram_disk, INDEX_HTM, sizeof(INDEX_HTM));
	USBDSetup();
	blink(5);

	while(1) {
		poll_input();

		if (msc_state == MSC_DATA_IN) {
			// Host grabbed the previous packet. Load the next one.
			MSC_PrepareDataIn();
		}

		if (file_changed) {
			printf("Active file (len=%ld) on cluster %d:\n\r", active_file_size, active_file_cluster);
			for(int i = 0; i < active_file_size; i += 64) {
				_write(0, (char*)msc_ram_disk +i, (((active_file_size -i) > 64) ? 64 : (active_file_size -i)));
			}
			printf("\n\r");
			blink(2);
			file_changed = 0;
		}
	}
}
