// SPDX-License-Identifier: GPL-2.0
#include "usbid.h"

#include <stdlib.h>

int parse_device_id(const char *id, unsigned *vid, unsigned *pid)
{
	unsigned long v, p;
	char *current = (char *)id;
	char *next;

	v = strtoul(current, &next, 0);
	if (next == current || *next != ':')
		return -1;

	current = ++next;
	p = strtoul(current, &next, 0);
	if (next == current || *next != '\0')
		return -1;

	if (v > 0xffff || p > 0xffff)
		return -1;

	*vid = v;
	*pid = p;
	return 0;
}
