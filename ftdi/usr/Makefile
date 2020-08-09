CC ?= cc
CFLAGS ?= -Wall -Werror -MD
LDFLAGS ?= -lftd2xx -lpthread -lrt

sources = usbid.c list.c setvidpid.c ftdi.c mpsse.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

list: list.o usbid.o ftdi.o
	$(CC) $^ $(LDFLAGS) -o $@

setvidpid: setvidpid.o usbid.o ftdi.o
	$(CC) $^ $(LDFLAGS) -o $@

-include $(sources:.c=.d)

.PHONY: clean all default

all: list setvidpid

clean:
	rm -rf list setvidpid *.o *.d
