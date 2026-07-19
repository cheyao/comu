all : flash

TARGET := bootloader

TARGET_MCU := CH32V203
TARGET_MCU_PACKAGE := CH32V203C8
# TARGET_MCU_PACKAGE := CH32V203F8

LINKER_SCRIPT := ./bootloader.ld

include ../ch32fun/ch32fun/ch32fun.mk
CFLAGS += -D$(TARGET_MCU_PACKAGE)

jumppad.elf: jumppad.asm
	$(PREFIX)-as -march=rv32imac -mabi=ilp32 jumppad.asm -o jumppad.elf

jumppad.bin: jumppad.elf
	$(PREFIX)-objcopy -O binary jumppad.bin

flash : cv_flash
clean : cv_clean
	rm jumppad.elf jumppad.bin

