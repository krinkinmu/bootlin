// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>

#include "ftdi.h"
#include "usbid.h"

static void usage(const char *name)
{
	fprintf(stdout,
		"%s [-h] [-i vid:pid]\n\n"
		"\t-h          print the usage information.\n"
		"\t-i vid:pid  by default the tool only reports devices with "
		"              FTDI VID:PID, this flag allows to include into "
		"              consideration FTDI devices that do not use the "
		"              default VID:PID. For example, FTDI devices "
		"              overriden VID:PID.",
		name);
}

int main(int argc, char **argv)
{
	unsigned vid, pid;
	int device_count;
	int opt;

	while ((opt = getopt(argc, argv, "i:h")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			break;
		case 'i':
			if (parse_device_id(optarg, &vid, &pid) != 0) {
				fprintf(stderr,
					"failed to parse device id %s\n",
					optarg);
				return 1;
			}
			if (ftdi_register_device_id(vid, pid) != 0) {
				fprintf(stderr,
					"failed to register device id %s\n",
					optarg);
				return 1;
			}
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (ftdi_devices(&device_count) != 0) {
		fprintf(stderr, "failed to enumerate FTDI devices\n");
		return 1;
	}

	fprintf(stdout, "Found %d FTDI devices in the system:\n", device_count);
	for (int i = 0; i < device_count; ++i) {
		struct serial serial;

		if (ftdi_serial(i, &serial) != 0)
			fprintf(stdout, "%d: failed to get serial number\n", i);
		else
			fprintf(stdout, "%d: %s\n", i, serial.serial);
	}

	return 0;
}
