all : flash

TARGET:=bootloader
TARGET_MCU:=CH32V203
TARGET_MCU_PACKAGE:=CH32V203C8T6
LINKER_SCRIPT:=./bootloader.ld

include ../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

