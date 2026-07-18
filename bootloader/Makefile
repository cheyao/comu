all : flash

TARGET := bootloader

TARGET_MCU := CH32V203
# TARGET_MCU_PACKAGE := CH32V203C8
TARGET_MCU_PACKAGE := CH32V203F8

LINKER_SCRIPT := ./bootloader.ld

include ../ch32fun/ch32fun/ch32fun.mk
CFLAGS += -D$(TARGET_MCU_PACKAGE)

flash : cv_flash
clean : cv_clean

