BINARY = trashcan

LDSCRIPT = STM32f103.ld
LDLIBS += -lm
#LDFLAGS += -L./lib/newlib/arm-none-eabi/armv7-m/newlib
#LDFLAGS += -L./lib/newlib/arm-none-eabi/armv7-m/libgloss/libnosys
OBJS += oled.o syscall.o isr.o core.o
CFLAGS += -std=c99 -D_COMPILING_NEWLIB -O0 -g
#CFLAGS += -I./lib/newlib/newlib/libc/include
#LDFLAGS += 

OPENCM3_DIR = ./lib/libopencm3
#OOCD_INTERFACE = altera-usb-blaster
#OOCD_TARGET = stm32f1x

include Makefile.stm32f1.include


flashme:
	sudo openocd -f interface/altera-usb-blaster.cfg -f target/stm32f1x.cfg -c init -c targets -c "halt" -c "flash write_image erase trashcan.elf" -c "reset run" -c shutdown

GDBHOST:
	sudo openocd -f interface/altera-usb-blaster.cfg -f target/stm32f1x.cfg -c init -c targets -c "halt"
