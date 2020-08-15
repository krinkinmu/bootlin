// SPDX-License-Identifier: GPL-2.0
#ifndef __FTDI_H__
#define __FTDI_H__

#include "ftd2xx.h"

#define FTDI_BIT_MODE_RESET 0x0u
#define FTDI_BIT_MODE_MPSSE 0x2u

struct serial {
	char serial[16];
};

struct manufacturer {
	char name[32];
};

struct manufacturer_id {
	char id[16];
};

struct description {
	char desc[64];
};

int ftdi_devices(int *devices);
int ftdi_serial(int device_index, struct serial *serial);
int ftdi_register_device_id(unsigned vid, unsigned pid);

int ftdi_open(const struct serial *serial, FT_HANDLE *handle);
int ftdi_reset(FT_HANDLE handle);
int ftdi_close(FT_HANDLE handle);

int ftdi_set_device_id(FT_HANDLE handle, unsigned vid, unsigned pid);
int ftdi_disable_special_chars(FT_HANDLE handle);
int ftdi_set_bit_mode(FT_HANDLE handle, unsigned mode);

int ftdi_read(FT_HANDLE handle, void *buf, unsigned size);
int ftdi_read_exactly(FT_HANDLE handle, void *buf, unsigned size);
int ftdi_drain(FT_HANDLE handle);

int ftdi_write(FT_HANDLE handle, const void *buf, unsigned size);
int ftdi_write_exactly(FT_HANDLE handle, const void *buf, unsigned size);

#endif  // __FTDI_H__
