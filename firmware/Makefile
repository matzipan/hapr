PKG=/usr/local/pkg

ARCH=arm-none-eabi
CC=$(ARCH)-gcc
HCC	= /usr/bin/gcc
OBJCOPY=$(ARCH)-objcopy

USER:=$(shell whoami)

SOURCERY=$(PKG)/sourcery-g++-lite-arm-eabi-2010.09.51-i686-1
GNU_VERSION=4.5.0
THUMB2GNULIB=$(SOURCERY)/$(ARCH)/lib/$(GNU_VERSION)/thumb2
THUMB2GNULIB2=$(SOURCERY)/$(ARCH)/lib/thumb2

# "Cortex Microcontroller Software Interface Standard" Startup files
CMSIS=$(PKG)/lpc1700-cmsis-lite-2011.01.26-i686-1
CMSISINCLUDES=-I$(CMSIS)/include
CMSISFL=$(CMSIS)/lib/core_cm3.o \
	$(CMSIS)/lib/system_LPC17xx.o \
	$(CMSIS)/lib/startup_LPC17xx.o
LDSCRIPT = $(CMSIS)/lib/ldscript_rom_gnu.ld

CFLAGS=-mcpu=cortex-m3  -mthumb  -Wall  -O0  -mapcs-frame  -D__thumb2__=1 \
  -msoft-float  -gdwarf-2  -mno-sched-prolog  -fno-hosted  -mtune=cortex-m3 \
  -march=armv7-m  -mfix-cortex-m3-ldrd   -ffunction-sections  -fdata-sections \
          -D__RAM_MODE__=0 $(CMSISINCLUDES) -I.

LDFLAGS=$(CMSISFL) -static -mcpu=cortex-m3 -mthumb -mthumb-interwork \
	   -Wl,--start-group -L$(THUMB2GNULIB) -L$(THUMB2GNULIB2) \
           -lc -lg -lstdc++ -lsupc++  -lgcc -lm  -Wl,--end-group \
	   -Xlinker -Map -Xlinker bin/lpc1700.map -Xlinker -T $(LDSCRIPT)
	   
LDFLAGS+=-L$(CMSIS)/lib -lDriversLPC17xxgnu

EXECNAME	= bin/serial

OBJ		= main.o

all: 	serial
	@echo "Build finished"


serial: $(OBJ)
	$(CC) -o $(EXECNAME) $(OBJ) $(LDFLAGS)
	$(OBJCOPY) -I elf32-little -O binary $(EXECNAME) $(EXECNAME).bin

# clean out the source tree ready to re-build
clean:
	rm -f `find . | grep \~`
	rm -f *.swp *.o */*.o */*/*.o  *.log
	rm -f *.d */*.d *.srec */*.a bin/*.map
	rm -f *.elf *.wrn bin/*.bin log *.hex
	rm -f $(EXECNAME)
# install software to board, remember to sync the file systems
install:
	@echo "Copying " $(EXECNAME) "to the MBED file system"
	cp $(EXECNAME).bin /media/MBED &
	sync
	@echo "Now press the reset button on all MBED file systems"

