// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <string.h>

#include "ftdi.h"

static const unsigned device_vid = 0x0005;
static const unsigned device_pid = 0x0001;
static const char serial[] = "FT3ELPXS";

int main(void)
{
	struct serial s;
	FT_HANDLE handle;

	if (ftdi_register_device_id(device_vid, device_pid) != 0) {
		fprintf(stderr,
			"Failed to register VID:PID 0x%04x:0x%04x\n",
			device_vid,
			device_pid);
		return 1;
	}

	strncpy(s.serial, serial, sizeof(s.serial));
	if (ftdi_open(&s, &handle) != 0) {
		fprintf(stderr, "Failed to open %s\n", serial);
		return 1;
	}

	fprintf(stdout, "Resetting the device...\n");
	fflush(stdout);
	if (ftdi_reset(handle) != 0) {
		fprintf(stderr, "Failed to reset %s\n", serial);
		ftdi_close(handle);
		return 1;
	}
	fprintf(stdout, "Device has been reset\n");
	fflush(stdout);

	if (ftdi_close(handle) != 0) {
		fprintf(stderr, "Failed to close %s\n", serial);
		return 1;
	}

	return 0;
}
