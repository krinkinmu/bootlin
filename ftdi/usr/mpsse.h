// SPDX-License-Identifier: GPL-2.0
#ifndef __MPSSE_H__
#define __MPSSE_H__

#include "ftdi.h"

struct mpsse {
	FT_HANDLE handle;
	unsigned char *cmd_buffer;
	int cmd_size;
	int cmd_offset;
};

int mpsse_open(const struct serial *serial, struct mpsse *mpsse);
int mpsse_verify(struct mpsse *mpsse);
int mpsse_queue(struct mpsse *mpsse, unsigned char cmd);
int mpsse_flush(struct mpsse *mpsse);
int mpsse_close(struct mpsse *mpsse);

#endif  // __MPSSE_H__
