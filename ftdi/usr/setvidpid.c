// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ftdi.h"
#include "usbid.h"

static void usage(const char *name)
{
	fprintf(stdout,
		"%s -s serial -i vid:pid [-h]\n\n"
		"\t-h          print the usage information.\n"
		"\t-s serial   specify the FTDI serial number of the device "
		"              to set VID and PID.\n"
		"\t-i vid:pid  VID and PID to set for the device.\n",
		name);
}

int main(int argc, char **argv)
{
	const char *device_id = NULL;
	const char *serial = NULL;
	struct serial s;
	unsigned vid, pid;
	FT_HANDLE handle;
	int opt;

	while ((opt = getopt(argc, argv, "s:i:h")) != -1) {
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
		case 'i':
			if (device_id) {
				fprintf(stderr,
					"Expect exactly one -i argument\n");
				return 1;
			}
			device_id = optarg;
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
		return 1;
	}
	strncpy(s.serial, serial, sizeof(s.serial) - 1);

	if (!device_id) {
		fprintf(stderr, "Expect exactly one -i argument\n");
		return 1;
	}

	if (parse_device_id(device_id, &vid, &pid) != 0) {
		fprintf(stderr, "Invalid device id: %s\n", device_id);
		return 1;
	}

	if (ftdi_open(&s, &handle) != 0) {
		fprintf(stderr, "Failed to open the device %s\n", serial);
		return 1;
	}

	if (ftdi_set_device_id(handle, vid, pid) != 0) {
		fprintf(stderr, "Failed to override vid:pid: %s\n", device_id);
		ftdi_close(handle);
		return 1;
	}

	if (ftdi_close(handle) != 0) {
		fprintf(stderr, "Failed to close the device %s\n", serial);
		return 1;
	}

	return 0;
}
