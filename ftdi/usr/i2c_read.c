// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ftdi.h"
#include "mpsse.h"

static const unsigned device_vid = 0x0005;
static const unsigned device_pid = 0x0001;

static int i2c_setup(struct mpsse *mpsse)
{
	struct mpsse_io_buffer io;
	int ret;

	mpsse_io_buffer_setup(&io);
	mpsse_disable_freq_div5(&io, NULL);
	mpsse_disable_adaptive_clocking(&io, NULL);
	mpsse_enable_3phase_clocking(&io, NULL);
	mpsse_set_drive0_pins(&io, 0x7, NULL);
	mpsse_disable_loopback(&io, NULL);
	mpsse_set_freq_divisor(&io, 0xffff, NULL);
	ret = mpsse_submit(mpsse, &io);
	mpsse_io_buffer_release(&io);
	return ret;
}

static int i2c_idle(struct mpsse *mpsse)
{
	struct mpsse_io_buffer io;
	int ret;

	mpsse_io_buffer_setup(&io);
	mpsse_set_output(&io, 0x40fb, 0xffff, NULL);
	ret = mpsse_submit(mpsse, &io);
	mpsse_io_buffer_release(&io);
	return ret;
}

static int i2c_start(struct mpsse *mpsse)
{
	struct mpsse_io_buffer io;
	int ret;

	mpsse_io_buffer_setup(&io);
	for (int i = 0; i < 5; ++i)
		mpsse_set_output(&io, 0x00fb, 0x00fd, NULL);
	for (int i = 0; i < 5; ++i)
		mpsse_set_output(&io, 0x40fb, 0x00fc, NULL);
	ret = mpsse_submit(mpsse, &io);
	mpsse_io_buffer_release(&io);
	return ret;
}

static int i2c_stop(struct mpsse *mpsse)
{
	struct mpsse_io_buffer io;
	int ret;

	mpsse_io_buffer_setup(&io);
	for (int i = 0; i < 5; ++i)
		mpsse_set_output(&io, 0x00fb, 0x00fc, NULL);
	for (int i = 0; i < 5; ++i)
		mpsse_set_output(&io, 0x00fb, 0x00fd, NULL);
	for (int i = 0; i < 5; ++i)
		mpsse_set_output(&io, 0x40fb, 0xffff, NULL);
	ret = mpsse_submit(mpsse, &io);
	mpsse_io_buffer_release(&io);
	return ret;
}

static int i2c_send_byte(struct mpsse *mpsse, unsigned char byte)
{
	struct mpsse_io_buffer io;
	struct mpsse_cmd cmd;
	int ret;

	mpsse_io_buffer_setup(&io);
	mpsse_write_bytes(&io, &byte, sizeof(byte), NULL);
	mpsse_set_output(&io, 0x00fb, 0x00fe, NULL);
	mpsse_read_bits(&io, 1, &cmd);
	ret = mpsse_submit(mpsse, &io);
	if (ret == 0) {
		const unsigned char *data = mpsse_data(&cmd);

		if ((*data & 0x1) != 0x0)
			ret = -1;
	}
	mpsse_io_buffer_release(&io);
	return ret;
}

static int i2c_send_addr(struct mpsse *mpsse, unsigned char i2c_addr, int read)
{
	const unsigned char byte =
		read ? ((i2c_addr << 1) | 1) : (i2c_addr << 1);
	return i2c_send_byte(mpsse, byte);
}

static int i2c_read_bytes(struct mpsse *mpsse, void *data, unsigned size)
{
	struct mpsse_io_buffer io;
	struct mpsse_cmd cmd;
	int ret;

	mpsse_io_buffer_setup(&io);
	mpsse_read_bytes(&io, 1, &cmd);
	for (unsigned i = 1; i < size; ++i) {
		mpsse_write_bits(&io, 0x00, 1, NULL);
		mpsse_set_output(&io, 0x00fb, 0x00fe, NULL);
		mpsse_read_bytes(&io, 1, NULL);
	}
	mpsse_write_bits(&io, 0xff, 1, NULL);
	mpsse_set_output(&io, 0x00fb, 0x00fe, NULL);
	ret = mpsse_submit(mpsse, &io);
	if (ret == 0)
		memcpy(data, mpsse_data(&cmd), size);
	mpsse_io_buffer_release(&io);
	return ret;
}

static void hexdump(FILE *output, const void *data, unsigned size)
{
	const unsigned char *b = data;

	for (unsigned i = 0; i < size; ++i)
		fprintf(output, "0x%02x ", b[i]);
}

static void usage(const char *name)
{
	fprintf(stdout,
		"%s -s serial -a i2c_addr -r i2c_reg -l size [-h]\n\n"
		"\t-h           print the usage information.\n"
		"\t-s serial    specify the FTDI serial number of the device "
		"               used as I2C bridge.\n"
		"\t-a i2c_addr  I2C address of the device to talk to.\n"
		"\t-r i2c_ref   register of the I2C device to read.\n"
		"\t-l size      amount of data to be read, it's possible "
		"               that the device will return less than that "
		"               in which case a warning message would be "
		"               printed to indicate that.\n",
		name);
}

int main(int argc, char **argv)
{
	const char *serial = NULL;
	const char *address = NULL;
	const char *reg = NULL;
	const char *len = NULL;
	char *endptr;
	void *data;

	struct mpsse mpsse;
	struct serial s;
	unsigned i2c_addr;
	unsigned i2c_reg;
	unsigned read_size;
	int opt;

	while ((opt = getopt(argc, argv, "s:a:r:l:h")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			break;
		case 's':
			if (serial) {
				fprintf(stderr,
					"Expect exactly one -s argument\n");
				return 1;
			}
			serial = optarg;
			break;
		case 'a':
			if (address) {
				fprintf(stderr,
					"Expect exactly one -a argument\n");
				return 1;
			}
			address = optarg;
			break;
		case 'r':
			if (reg) {
				fprintf(stderr,
					"Expect exactly one -r argument\n");
				return 1;
			}
			reg = optarg;
			break;
		case 'l':
			if (len) {
				fprintf(stderr,
					"Expect exactly one -l argument\n");
				return 1;
			}
			len = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (!serial) {
		fprintf(stderr, "Expect exactly one -s argument\n");
		return 1;
	}
	if (strlen(serial) > sizeof(s.serial) - 1) {
		fprintf(stderr, "Serial number %s is too long\n", serial);
		return -1;
	}
	strncpy(s.serial, serial, sizeof(s.serial));

	if (!address) {
		fprintf(stderr, "Expect exactly one -a argument\n");
		return 1;
	}
	i2c_addr = strtoul(address, &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr, "Failed to parse i2c address %s\n", address);
		return 1;
	}

	if (!reg) {
		fprintf(stderr, "Expect exactly one -r argument\n");
		return 1;
	}
	i2c_reg = strtoul(reg, &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr, "Failed to parse i2c register %s\n", reg);
		return 1;
	}
	
	if (!len) {
		fprintf(stderr, "Expect exactly one -l argument\n");
		return 1;
	}
	read_size = strtoul(len, &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr,
			"Failed to parse how many bytes to read: %s\n",
			len);
		return 1;
	}

	if (ftdi_register_device_id(device_vid, device_pid) != 0) {
		fprintf(stderr,
			"Failed to register VID:PID 0x%04x:0x%04x\n",
			device_vid,
			device_pid);
		return 1;
	}

	if (mpsse_open(&s, &mpsse) != 0) {
		fprintf(stderr, "Failed to enable MPSSE on %s\n", serial);
		return 1;
	}

	if (mpsse_verify(&mpsse) != 0) {
		fprintf(stderr, "Failed to verify MPSSE mode on %s\n", serial);
		mpsse_close(&mpsse);
		return 1;
	}

	assert((data = malloc(read_size)) != NULL);

	assert(i2c_setup(&mpsse) == 0);
	assert(i2c_idle(&mpsse) == 0);

	assert(i2c_start(&mpsse) == 0);
	assert(i2c_send_addr(&mpsse, i2c_addr, 0) == 0);
	assert(i2c_send_byte(&mpsse, 0xf0) == 0);
	assert(i2c_send_byte(&mpsse, 0x55) == 0);
	assert(i2c_stop(&mpsse) == 0);

	assert(i2c_start(&mpsse) == 0);
	assert(i2c_send_addr(&mpsse, i2c_addr, 0) == 0);
	assert(i2c_send_byte(&mpsse, 0xfb) == 0);
	assert(i2c_send_byte(&mpsse, 0x00) == 0);
	assert(i2c_stop(&mpsse) == 0);

	assert(i2c_start(&mpsse) == 0);
	assert(i2c_send_addr(&mpsse, i2c_addr, 0) == 0);
	assert(i2c_send_byte(&mpsse, i2c_reg) == 0);
	assert(i2c_idle(&mpsse) == 0);
	assert(i2c_start(&mpsse) == 0);
	assert(i2c_send_addr(&mpsse, i2c_addr, 1) == 0);
	assert(i2c_read_bytes(&mpsse, data, read_size) == 0);
	assert(i2c_stop(&mpsse) == 0);

	fprintf(stdout, "Read data:\n");
	hexdump(stdout, data, read_size);
	fprintf(stdout, "\n");

	free(data);
	if (mpsse_close(&mpsse) != 0) {
		fprintf(stderr, "Failed to close %s\n", serial);
		return 1;
	}

	return 0;
}
