// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ftdi.h"
#include "mpsse.h"

static const unsigned device_vid = 0x0005;
static const unsigned device_pid = 0x0001;

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

	struct mpsse mpsse;
	struct serial s;
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

	if (!reg) {
		fprintf(stderr, "Expect exactly one -r argument\n");
		return 1;
	}
	
	if (!len) {
		fprintf(stderr, "Expect exactly one -l argument\n");
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

	if (mpsse_verify(&mpsse) != 0)
		fprintf(stderr, "Failed to verify MPSSE mode on %s\n", serial);
	else
		fprintf(stdout, "MPSSE mode was actived on %s\n", serial);

	if (mpsse_close(&mpsse) != 0) {
		fprintf(stderr, "Failed to close %s\n", serial);
		return 1;
	}

	return 0;
}
