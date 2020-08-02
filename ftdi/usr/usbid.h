// SPDX-License-Identifier: GPL-2.0
#ifndef __USBID_H__
#define __USBID_H__

// parse_device_id parses a string in format vid:pid, where vid
// and pid are integer numbers in the range [0; 0xffff]. vid and
// pid can be specified using bases 8, 10 and 16. For base 16 you
// have to specify prefix 0x and for base 8 use prefix 0.
// 
// If the function parsed the string successfully it will return
// 0 and the parsed vid and pid will be stored in places specified
// by the vid and pid pointers.
//
// If the function didn't parse the string successfully it will
// return a non-zero number.
int parse_device_id(const char *id, unsigned *vid, unsigned *pid);

#endif  // __USBID_H__
