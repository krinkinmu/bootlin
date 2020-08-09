CC ?= cc
CFLAGS ?= -Wall -Werror -fsanitize=undefined -MD
LDFLAGS ?= -lftd2xx -lpthread -lrt -fsanitize=undefined

sources = usbid.c list.c setvidpid.c ftdi.c mpsse.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

i2c_read: i2c_read.o ftdi.o mpsse.o
	$(CC) $^ $(LDFLAGS) -o $@

list: list.o usbid.o ftdi.o
	$(CC) $^ $(LDFLAGS) -o $@

setvidpid: setvidpid.o usbid.o ftdi.o
	$(CC) $^ $(LDFLAGS) -o $@

-include $(sources:.c=.d)

.PHONY: clean all default

all: list setvidpid i2c_read

clean:
	rm -rf list setvidpid i2c_read *.o *.d
