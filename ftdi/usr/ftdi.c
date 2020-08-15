// SPDX-License-Identifier: GPL-2.0
#include "ftdi.h"

int ftdi_devices(int *devices)
{
	FT_STATUS status;
	DWORD n;

	status = FT_ListDevices(&n, NULL, FT_LIST_NUMBER_ONLY);
	if (!FT_SUCCESS(status))
		return -1;
	*devices = n;
	return 0;
}

int ftdi_serial(int device_index, struct serial *serial)
{
	FT_STATUS status;

	status = FT_ListDevices(
		(void *)((unsigned long)device_index),
		serial->serial,
		FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_register_device_id(unsigned vid, unsigned pid)
{
	FT_STATUS status;

	status = FT_SetVIDPID(vid, pid);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_open(const struct serial *serial, FT_HANDLE *handle)
{
	FT_STATUS status;

	status = FT_OpenEx(
		(void *)serial->serial, FT_OPEN_BY_SERIAL_NUMBER, handle);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_reset(FT_HANDLE handle)
{
	FT_STATUS status;

	status = FT_ResetDevice(handle);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_close(FT_HANDLE handle)
{
	FT_STATUS status;

	status = FT_Close(handle);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_set_device_id(FT_HANDLE handle, unsigned vid, unsigned pid)
{
	FT_STATUS status;
	FT_PROGRAM_DATA data;
	struct manufacturer m;
	struct manufacturer_id mid;
	struct description desc;
	struct serial serial;

	data.Signature1 = 0x0;
	data.Signature2 = 0xffffffff;
	data.Version = 0x5;
	data.Manufacturer = m.name;
	data.ManufacturerId = mid.id;
	data.Description = desc.desc;
	data.SerialNumber = serial.serial;

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

int ftdi_disable_special_chars(FT_HANDLE handle)
{
	FT_STATUS status;

	status = FT_SetChars(handle, 0, 0, 0, 0);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_set_bit_mode(FT_HANDLE handle, unsigned mode)
{
	FT_STATUS status;

	status = FT_SetBitMode(handle, 0x0, mode);
	if (!FT_SUCCESS(status))
		return -1;
	return 0;
}

int ftdi_read(FT_HANDLE handle, void *buf, unsigned size)
{
	FT_STATUS status;
	DWORD read, pending, to_read;

	status = FT_GetQueueStatus(handle, &pending);
	if (!FT_SUCCESS(status))
		return -1;

	if (pending == 0)
		return 0;

	if (size < pending)
		to_read = size;
	else
		to_read = pending;

	status = FT_Read(handle, buf, to_read, &read);
	if (!FT_SUCCESS(status))
		return -1;
	return read;
}

int ftdi_read_exactly(FT_HANDLE handle, void *buf, unsigned size)
{
	unsigned char *b = buf;
	DWORD total = 0;

	while (total < size) {
		FT_STATUS status;
		DWORD read;

		status = FT_Read(
			handle, (void *)(b + total), size - total, &read);
		if (!FT_SUCCESS(status))
			return -1;
		total += read;
	}
	return total;
}

int ftdi_drain(FT_HANDLE handle)
{
	FT_STATUS status;
	DWORD pending;
	DWORD total = 0;
	char buf[64];

	status = FT_GetQueueStatus(handle, &pending);
	if (!FT_SUCCESS(status))
		return -1;

	while (total < pending) {
		DWORD read;
		int to_read;

		if (sizeof(buf) < pending - total)
			to_read = sizeof(buf);
		else
			to_read = pending - total;

		status = FT_Read(handle, (void *)buf, to_read, &read);
		if (!FT_SUCCESS(status))
			return -1;
		total += read;
	}
	return 0;
}

int ftdi_write(FT_HANDLE handle, const void *buf, unsigned size)
{
	FT_STATUS status;
	DWORD written;

	status = FT_Write(handle, (void *)buf, size, &written);
	if (!FT_SUCCESS(status))
		return -1;
	return written;
}

int ftdi_write_exactly(FT_HANDLE handle, const void *buf, unsigned size)
{
	const unsigned char *b = buf;
	DWORD total = 0;

	while (total < size) {
		FT_STATUS status;
		DWORD written;

		status = FT_Write(
			handle, (void *)(b + total), size - total, &written);
		if (!FT_SUCCESS(status))
			return -1;
		total += written;
	}
	return total;
}
