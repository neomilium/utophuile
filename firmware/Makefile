CC=avr-gcc
OBJCOPY=avr-objcopy

MCU=atmega8535
CFLAGS=-W -Wall -ansi -pedantic -std=c99 -funsigned-bitfields -fpack-struct -fshort-enums -mmcu=$(MCU) -fno-jump-tables -mcall-prologues -Os -DF_CPU=8000000UL
LDFLAGS=-mmcu=$(MCU)

#CFLAGS+=-DSIMULATE_TEMP=1

SRCS=	adc.c \
	beep.c \
	buttons.c \
	leds.c \
	rgb.c \
	relay.c \
	scheduler.c \
	twi.c \
	uart.c \
	utophuile.c

OBJS=	$(SRCS:.c=.o)

HEX=	utophuile.hex

all: $(HEX)


utophuile.out: version.h $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

%.hex: %.out
	$(OBJCOPY) -R .eeprom -O ihex $< $@

version.h: update-version

clean:
	rm -rf *.out $(OBJS) $(HEX)

GIT_DESCRIBE=$(shell git describe --tags)
COMPILE_DATE=$(shell date +"%Y-%m-%d %H:%M:%S")
PACKAGE_VERSION="$(GIT_DESCRIBE) (compiled: $(COMPILE_DATE))"
PACKAGE_STRING="UtopHuile $(PACKAGE_VERSION)"

update-version:
	echo "#define PACKAGE_STRING \"$(PACKAGE_STRING)\"" > version.h

AVRDUDE_PROGRAMMER=stk500v2
AVRDUDE_MCU=m8535
AVRDUDE_PORT=/dev/ttyACM0
SUDO=

flash: $(HEX)
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P $(AVRDUDE_PORT) -U flash:w:$(HEX):i

mrproper: clean
	rm -rf *.o *~

cppcheck:
	cppcheck --quiet \
	    --check-config .
	cppcheck --quiet --enable=all --std=posix --std=c99 \
	    --force --inconclusive .

reset:
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P $(AVRDUDE_PORT)

