all : build

TARGET := blink
TARGET_MCU := CH32V203
TARGET_MCU_PACKAGE := CH32V203F8

LINKER_SCRIPT := ./comu.ld

include ../../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
