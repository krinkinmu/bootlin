// SPDX-License-Identifier: GPL-2.0
#include "mpsse.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


void mpsse_io_buffer_setup(struct mpsse_io_buffer *io)
{
	io->cmd = NULL;
	io->cmd_size = 0;
	io->cmd_capacity = 0;
	io->data = NULL;
	io->data_size = 0;
	io->data_capacity = 0;
}

void mpsse_io_buffer_reset(struct mpsse_io_buffer *io)
{
	io->cmd_size = 0;
	io->data_size = 0;
}

void mpsse_io_buffer_release(struct mpsse_io_buffer *io)
{
	free(io->cmd);
	free(io->data);
}

void *mpsse_cmd(struct mpsse_cmd *cmd)
{
	assert(cmd);
	assert(cmd->io);
	assert(cmd->io->cmd);
	return (unsigned char *)cmd->io->cmd + cmd->cmd_offset;
}

void *mpsse_data(struct mpsse_cmd *cmd)
{
	assert(cmd);
	assert(cmd->io);
	assert(cmd->io->data);
	return (unsigned char *)cmd->io->data + cmd->data_offset;
}

static unsigned mpsse_io_buffer_reserve_cmd(
	struct mpsse_io_buffer *io, unsigned size)
{
	const unsigned offset = io->cmd_size;

	if (io->cmd_size + size > io->cmd_capacity) {
		unsigned new_capacity = 2 * io->cmd_capacity / 3;
		void *new_cmd;

		if (new_capacity < io->cmd_size + size)
			new_capacity = io->cmd_size + size;

		assert((new_cmd = realloc(io->cmd, new_capacity)) != NULL);
		io->cmd = new_cmd;
		io->cmd_capacity = new_capacity;
	}

	io->cmd_size += size;
	return offset;
}

static unsigned mpsse_io_buffer_reserve_data(
	struct mpsse_io_buffer *io, unsigned size)
{
	const unsigned offset = io->data_size;

	if (io->data_size + size > io->data_capacity) {
		unsigned new_capacity = 2 * io->data_capacity / 3;
		void *new_data;

		if (new_capacity < io->data_size + size)
			new_capacity = io->data_size + size;

		assert((new_data = realloc(io->data, new_capacity)) != NULL);
		io->data = new_data;
		io->data_capacity = new_capacity;
	}

	io->data_size += size;
	return offset;
}

int mpsse_open(const struct serial *serial, struct mpsse *mpsse)
{
	FT_HANDLE handle;

	if (ftdi_open(serial, &handle) != 0)
		return -1;

	mpsse->handle = handle;
	if (mpsse_reset(mpsse) != 0) {
		mpsse_close(mpsse);
		return -1;
	}

	return 0;
}

int mpsse_close(struct mpsse *mpsse)
{
	int ret = ftdi_close(mpsse->handle);

	memset(mpsse, 0, sizeof(*mpsse));
	return ret;
}

int mpsse_reset(struct mpsse *mpsse)
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

static void mpsse_cmd_prepare(
	struct mpsse_io_buffer *io,
	struct mpsse_cmd *cmd,
	unsigned cmd_size,
	unsigned data_size)
{
	cmd->io = io;
	cmd->cmd_offset = mpsse_io_buffer_reserve_cmd(io, cmd_size);
	cmd->cmd_size = cmd_size;
	cmd->data_offset = mpsse_io_buffer_reserve_data(io, data_size);
	cmd->data_size = data_size;
}

static void mpsse_incorrect_command(
	struct mpsse_io_buffer *io, unsigned char op, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;

	mpsse_cmd_prepare(io, &cmd, 1, 2);
	*((unsigned char *)mpsse_cmd(&cmd)) = op;
	if (out)
		*out = cmd;
}

int mpsse_verify(struct mpsse *mpsse)
{
	struct mpsse_io_buffer io;
	struct mpsse_cmd cmd;
	unsigned char *data;

	mpsse_io_buffer_setup(&io);
	mpsse_incorrect_command(&io, 0xaa, &cmd);
	if (mpsse_submit(mpsse, &io) != 0)
		goto err;

	data = mpsse_data(&cmd);
	if (data[0] != 0xfa || data[1] != 0xaa)
		goto err;

	mpsse_io_buffer_reset(&io);
	mpsse_incorrect_command(&io, 0xab, &cmd);
	if (mpsse_submit(mpsse, &io) != 0)
		goto err;

	data = mpsse_data(&cmd);
	if (data[0] != 0xfa || data[1] != 0xab)
		goto err;

	return 0;

err:
	mpsse_io_buffer_release(&io);
	return -1;
}

int mpsse_submit(struct mpsse *mpsse, struct mpsse_io_buffer *io)
{
	if (io->cmd_size != 0 &&
	    ftdi_write_exactly(mpsse->handle, io->cmd, io->cmd_size) < 0)
		return -1;

	// Basically no error handling here. YOLO!
	if (io->data_size != 0 &&
	    ftdi_read_exactly(mpsse->handle, io->data, io->data_size) < 0)
		return -1;

	return 0;
}


void mpsse_disable_freq_div5(struct mpsse_io_buffer *io, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;

	mpsse_cmd_prepare(io, &cmd, 1, 0);
	*((unsigned char *)mpsse_cmd(&cmd)) = 0x8a;
	if (out)
		*out = cmd;
}


void mpsse_disable_adaptive_clocking(
	struct mpsse_io_buffer *io, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;

	mpsse_cmd_prepare(io, &cmd, 1, 0);
	*((unsigned char *)mpsse_cmd(&cmd)) = 0x97;
	if (out)
		*out = cmd;
}

void mpsse_disable_loopback(struct mpsse_io_buffer *io, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;

	mpsse_cmd_prepare(io, &cmd, 1, 0);
	*((unsigned char *)mpsse_cmd(&cmd)) = 0x85;
	if (out)
		*out = cmd;
}

void mpsse_enable_3phase_clocking(
	struct mpsse_io_buffer *io, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;

	mpsse_cmd_prepare(io, &cmd, 1, 0);
	*((unsigned char *)mpsse_cmd(&cmd)) = 0x8c;
	if (out)
		*out = cmd;
}

void mpsse_set_drive0_pins(
	struct mpsse_io_buffer *io, unsigned pinmask, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	mpsse_cmd_prepare(io, &cmd, 3, 0);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x9e;
	buf[1] = pinmask & 0xff;
	buf[2] = (pinmask >> 8) & 0xff;
	if (out)
		*out = cmd;
}

void mpsse_set_freq_divisor(
	struct mpsse_io_buffer *io, unsigned divisor, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	assert(divisor <= 0xffff);
	mpsse_cmd_prepare(io, &cmd, 3, 0);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x86;
	buf[1] = divisor & 0xff;
	buf[2] = (divisor >> 8) & 0xff;
	if (out)
		*out = cmd;
}

void mpsse_set_output(
	struct mpsse_io_buffer *io,
	unsigned pinmask,
	unsigned pinvals,
	struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	mpsse_cmd_prepare(io, &cmd, 6, 0);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x80;
	buf[1] = pinvals & 0xff;
	buf[2] = pinmask & 0xff;
	buf[3] = 0x82;
	buf[4] = pinvals >> 8 & 0xff;
	buf[5] = (pinmask >> 8) & 0xff;
	if (out)
		*out = cmd;
}

void mpsse_write_bytes(
	struct mpsse_io_buffer *io,
	const void *data,
	unsigned size,
	struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	if (size == 0) {
		if (out) mpsse_cmd_prepare(io, out, 0, 0);
		return;
	}

	assert(size <= 0xffff);
	mpsse_cmd_prepare(io, &cmd, 3 + size, 0);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x11;
	buf[1] = (size - 1) & 0xff;
	buf[2] = ((size - 1) >> 8) & 0xff;
	memcpy(&buf[3], data, size);
	if (out)
		*out = cmd;
}

void mpsse_read_bytes(
	struct mpsse_io_buffer *io, unsigned size, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	if (size == 0) {
		if (out) mpsse_cmd_prepare(io, out, 0, 0);
		return;
	}

	assert(size <= 0xffff);
	mpsse_cmd_prepare(io, &cmd, 4, size);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x20;
	buf[1] = (size - 1) & 0xff;
	buf[2] = ((size - 1) >> 8) & 0xff;
	buf[3] = 0x87;
	if (out)
		*out = cmd;
}

void mpsse_write_bits(
	struct mpsse_io_buffer *io,
	unsigned char data,
	unsigned bits,
	struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	if (bits == 0) {
		if (out) mpsse_cmd_prepare(io, out, 0, 0);
		return;
	}

	assert(bits <= 8);
	mpsse_cmd_prepare(io, &cmd, 3, 0);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x13;
	buf[1] = (bits - 1) & 0xff;
	buf[2] = data;
	if (out)
		*out = cmd;
}

void mpsse_read_bits(
	struct mpsse_io_buffer *io, unsigned bits, struct mpsse_cmd *out)
{
	struct mpsse_cmd cmd;
	unsigned char *buf;

	if (bits == 0) {
		if (out) mpsse_cmd_prepare(io, out, 0, 0);
		return;
	}

	assert(bits <= 8);
	mpsse_cmd_prepare(io, &cmd, 3, 1);
	buf = mpsse_cmd(&cmd);
	buf[0] = 0x22;
	buf[1] = (bits - 1) & 0xff;
	buf[2] = 0x87;
	if (out)
		*out = cmd;
}

