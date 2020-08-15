// SPDX-License-Identifier: GPL-2.0
#ifndef __MPSSE_H__
#define __MPSSE_H__

#include "ftdi.h"

// MPSSE IO buffer just manages memory for the commands we want to send to
// the MPSSE and for the expected results.
// 
// One way to look at the MPSSE IO buffer is that it's a collection of commands
// that we want to send to the MPSSE in one batch.
struct mpsse_io_buffer {
	void *cmd;
	unsigned cmd_size;
	unsigned cmd_capacity;

	void *data;
	unsigned data_size;
	unsigned data_capacity;
};

void mpsse_io_buffer_setup(struct mpsse_io_buffer *io);
void mpsse_io_buffer_reset(struct mpsse_io_buffer *io);
void mpsse_io_buffer_release(struct mpsse_io_buffer *io);


struct mpsse {
	FT_HANDLE handle;
};

int mpsse_open(const struct serial *serial, struct mpsse *mpsse);
int mpsse_close(struct mpsse *mpsse);

// Reset will reset the device to it's initial state and drain all the device
// buffer. The current version of the library doesn't provde many error
// handling capabilities, so in most cases once you encountered an error you
// should reset the device and start over.
int mpsse_reset(struct mpsse *mpsee);

// Checks that MPSSE device is responding to the MPSSE commands as expected.
int mpsse_verify(struct mpsse *mpsse);

// Executes the commands described by the MPSSE IO buffer. If the function
// completes successfully it returns 0, otherwise a non-0 value is returned.
// If the execution fails you should not expect that the device will be in
// any particular state, so if you want to continue using it you should reset.
int mpsse_submit(struct mpsse *mpsse, struct mpsse_io_buffer *io);


// MPSSE command is a handler that allows access to the command data itself
// as well as the data that was read from the device if any after the command
// has been executed.
//
// Never create MPSSE command manually and never use MPSSE command after the
// MPSSE IO buffer associated with it has been reset or released, since the
// MPSSE IO buffer is managing the memory.
struct mpsse_cmd {
	struct mpsse_io_buffer *io;
	unsigned cmd_offset;
	unsigned cmd_size;
	unsigned data_offset;
	unsigned data_size;
};

// Accessor function to the MPSSE command buffer as well as to the buffer
// for the data read as part of executing the command.
void *mpsse_cmd(struct mpsse_cmd *cmd);
void *mpsse_data(struct mpsse_cmd *cmd);

// Most of these functions are direct or almost direct wrappers around the
// MPSSE command set. You can get familiar with the list of the available
// commands and their meaning through the documentation for MPSSE command
// processor on the FTDI site.
// 
// None of the functions actually executes MPSSE commands, they just update
// the provided MPSSE IO buffer. To actually execute the commands you should
// call mpsse_submit function with the MPSSE IO buffer.
//
// All of the function take a pointer to MPSSE command structure as the last
// argument. The pointer may be NULL, but in this case you will not have access
// to any data read as part of executing the command. If you actually want to
// read some data from MPSSE then you need to provide a valid MPSSE command
// pointer as the last argument. The data could be access by using mpsse_data
// function on the MPSSE command after it has been executed.
void mpsse_disable_freq_div5(
	struct mpsse_io_buffer *io, struct mpsse_cmd *cmd);
void mpsse_disable_adaptive_clocking(
	struct mpsse_io_buffer *io, struct mpsse_cmd *cmd);
void mpsse_disable_loopback(
	struct mpsse_io_buffer *io, struct mpsse_cmd *cmd);
void mpsse_enable_3phase_clocking(
	struct mpsse_io_buffer *io, struct mpsse_cmd *cmd);
void mpsse_set_drive0_pins(
	struct mpsse_io_buffer *io, unsigned pinmask, struct mpsse_cmd *cmd);
void mpsse_set_freq_divisor(
	struct mpsse_io_buffer *io, unsigned divisor, struct mpsse_cmd *cmd);
void mpsse_set_output(
	struct mpsse_io_buffer *io,
	unsigned pinmask,
	unsigned pinvals,
	struct mpsse_cmd *cmd);
void mpsse_write_bytes(
	struct mpsse_io_buffer *io,
	const void *data,
	unsigned size,
	struct mpsse_cmd *cmd);
void mpsse_read_bytes(
	struct mpsse_io_buffer *io, unsigned size, struct mpsse_cmd *cmd);
void mpsse_write_bits(
	struct mpsse_io_buffer *io,
	unsigned char data,
	unsigned bits,
	struct mpsse_cmd *cmd);
void mpsse_read_bits(
	struct mpsse_io_buffer *io, unsigned bits, struct mpsse_cmd *cmd);

#endif  // __MPSSE_H__
