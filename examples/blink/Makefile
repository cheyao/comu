all : flash

# Run make clangd and make clangd_clean for lsp support

TARGET := blink
TARGET_MCU := CH32V203
TARGET_MCU_PACKAGE := CH32V203F8

LINKER_SCRIPT := ./comu.ld
# Do NOT delete
# First 2k bytes contains the bootloader
WRITE_SECTION := flash+0x800

include ../../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

