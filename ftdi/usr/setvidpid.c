// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>

#include "ftd2xx.h"
#include "usbid.h"

static int open_device(const char *serial, FT_HANDLE *handle)
{
	FT_STATUS status;

	status = FT_OpenEx((void *)serial, FT_OPEN_BY_SERIAL_NUMBER, handle);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

static int set_device_id(FT_HANDLE handle, unsigned vid, unsigned pid)
{
	FT_STATUS status;
	FT_PROGRAM_DATA data;
	char m[32];
	char mid[16];
	char desc[64];
	char serial[16];

	data.Signature1 = 0x0;
	data.Signature2 = 0xffffffff;
	data.Version = 0x5;
	data.Manufacturer = m;
	data.ManufacturerId = mid;
	data.Description = desc;
	data.SerialNumber = serial;

	status = FT_EE_Read(handle, &data);
	if (!FT_SUCCESS(status))
		return -1;

	data.VendorId = vid;
	data.ProductId = pid;

	status = FT_EE_Program(handle, &data);
	if (!FT_SUCCESS(status))
		return -1;

	return 0;
}

static int close_device(FT_HANDLE handle)
{
	FT_STATUS status;

	status = FT_Close(handle);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

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

	if (!device_id) {
		fprintf(stderr, "Expect exactly one -i argument\n");
		return 1;
	}

	if (parse_device_id(device_id, &vid, &pid) != 0) {
		fprintf(stderr, "Invalid device id: %s\n", device_id);
		return 1;
	}

	if (open_device(serial, &handle) != 0) {
		fprintf(stderr, "Failed to open the device %s\n", serial);
		return 1;
	}

	if (set_device_id(handle, vid, pid) != 0) {
		fprintf(stderr, "Failed to override vid:pid: %s\n", device_id);
		close_device(handle);
		return 1;
	}

	if (close_device(handle) != 0) {
		fprintf(stderr, "Failed to close the device %s\n", serial);
		return 1;
	}

	return 0;
}
