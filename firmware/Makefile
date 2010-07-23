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


utophuile.out: config.h $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

%.hex: %.out
	$(OBJCOPY) -R .eeprom -O ihex $< $@

config.h: update-config

clean:
	rm -rf *.out $(OBJS) $(HEX)

PACKAGE_VERSION=$(shell date +%Y-%m-%d)
PACKAGE_STRING="UtopHuile $(PACKAGE_VERSION)"
update-config:
	echo "#define PACKAGE_STRING \"$(PACKAGE_STRING)\"" > config.h

AVRDUDE_PROGRAMMER=stk500v2
AVRDUDE_MCU=m8535
SUDO=

flash: $(HEX)
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P /dev/ttyACM0 -U flash:w:$(HEX):i 

mrproper: clean
	rm -rf *.o *~
