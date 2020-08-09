// SPDX-License-Identifier: GPL-2.0
#include "mpsse.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const int mpsse_cmd_buffer_size = 16384;

static int mpsse_setup(struct mpsse *mpsse)
{
	if (ftdi_reset(mpsse->handle) != 0)
		return -1;

	if (ftdi_drain(mpsse->handle) != 0)
		return -1;

	if (ftdi_disable_special_chars(mpsse->handle) != 0)
		return -1;

	if (ftdi_set_bit_mode(mpsse->handle, FTDI_BIT_MODE_RESET) != 0)
		return -1;

	if (ftdi_set_bit_mode(mpsse->handle, FTDI_BIT_MODE_MPSSE) != 0)
		return -1;

	return 0;
}

int mpsse_flush(struct mpsse *mpsse)
{
	if (mpsse->cmd_offset == 0)
		return 0;

	if (ftdi_write_exactly(
		mpsse->handle, mpsse->cmd_buffer, mpsse->cmd_offset) < 0)
		return -1;
	mpsse->cmd_offset = 0;
	return 0;
}

int mpsse_queue(struct mpsse *mpsse, unsigned char cmd)
{
	mpsse->cmd_buffer[mpsse->cmd_offset++] = cmd;
	if (mpsse->cmd_offset == mpsse->cmd_size)
		return mpsse_flush(mpsse);
	return 0;
}


int mpsse_verify(struct mpsse *mpsse)
{
	unsigned char buf[2];

	assert(mpsse_queue(mpsse, 0xaa) == 0);
	if (mpsse_flush(mpsse) != 0) {
		return -1;
	}

	if (ftdi_read_exactly(mpsse->handle, buf, sizeof(buf)) < 0)
		return -1;

	if (buf[0] != 0xfa || buf[1] != 0xaa)
		return -1;

	assert(mpsse_queue(mpsse, 0xab) == 0);
	if (mpsse_flush(mpsse) != 0)
		return -1;

	if (ftdi_read_exactly(mpsse->handle, buf, sizeof(buf)) < 0)
		return -1;

	if (buf[0] != 0xfa || buf[1] != 0xab)
		return -1;

	return 0;
}

int mpsse_open(const struct serial *serial, struct mpsse *mpsse)
{
	FT_HANDLE handle;
	unsigned char *cmd_buffer;

	if (ftdi_open(serial, &handle) != 0)
		return -1;

	cmd_buffer = malloc(mpsse_cmd_buffer_size);
	if (!cmd_buffer) {
		ftdi_close(handle);
		return -1;
	}

	mpsse->handle = handle;
	mpsse->cmd_buffer = cmd_buffer;
	mpsse->cmd_size = mpsse_cmd_buffer_size;
	mpsse->cmd_offset = 0;

	if (mpsse_setup(mpsse) != 0) {
		mpsse_close(mpsse);
		return -1;
	}

	return 0;
}

int mpsse_close(struct mpsse *mpsse)
{
	int ret = ftdi_close(mpsse->handle);

	free(mpsse->cmd_buffer);
	memset(mpsse, 0, sizeof(*mpsse));
	return ret;
}
