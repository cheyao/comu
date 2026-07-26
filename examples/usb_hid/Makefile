all : flash

# Run make clangd and make clangd_clean for lsp support

TARGET := usb_hid
TARGET_MCU := CH32V203
TARGET_MCU_PACKAGE := CH32V203F8

LINKER_SCRIPT := ./comu.ld
WRITE_SECTION := flash+0x800
ADDITIONAL_C_FILES:=../../ch32fun/extralibs/usbd.c

include ../../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

