// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#define VENDOR_ID  0x0005
#define PRODUCT_ID 0x0001

// Timeout in milliseconds for USB IO operations
#define TIMEOUT 5000

#define FTDI_BIT_MODE_RESET 0x0000u
#define FTDI_BIT_MODE_MPSSE 0x0200u


static const struct usb_device_id ftdi_id_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ }
};
MODULE_DEVICE_TABLE(usb, ftdi_id_table);

static int ftdi_usb_set_bit_mode(struct usb_device *dev, u16 mode)
{
	int ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x0b,
		/* bRequestType = */0x40,
		/* wValue = */mode,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	return 0;
}

static int ftdi_usb_disable_special_characters(struct usb_device *dev)
{
	int ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x06,
		/* bRequestType = */0x40,
		/* wValue = */0x0000,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x07,
		/* bRequestType = */0x40,
		/* wValue = */0x0000,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	return 0;
}

// The FTDI interface is not described in any publicly available document.
// Instead of the documentation I looked at what the FTDI userspace driver does
// and replicated it here. I didn't give names to the contstants used below
// because for me they are as much magic numbers as they are for the reader of
// the code. That being said in drivers/usb/serial/ftdi_sio.c we can find that:
//
//   * bRequestType for reset is 0x40 (FTDI_SIO_RESET_REQUEST_TYPE)
//   * bRequest for reset is 0x00 (FTDI_SIO_RESET_REQUEST)
//   * wIndex is the port number and on F232H it's always 0
//
// The meaning of wValue is not clear to me still. drivers/usb/serial/ftdi_sio.c
// uses only value 0 (FTDI_SIO_RESET_SIO), but the userspace driver when doing
// reset uses 0, 1 and 2.
static int ftdi_usb_reset(struct usb_device *dev)
{
	int ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x00,
		/* bRequestType = */0x40,
		/* wValue = */0x0000,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x00,
		/* bRequestType = */0x40,
		/* wValue = */0x0001,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	ret = usb_control_msg(
		dev, usb_sndctrlpipe(dev, 0),
		/* bRequest = */0x00,
		/* bRequestType = */0x40,
		/* wValue = */0x0002,
		/* wIndex =  */0x0000,
		/* data = */NULL,
		/* size = */0,
		TIMEOUT);
	if (ret < 0)
		return ret;

	ret = ftdi_usb_disable_special_characters(dev);
	if (ret < 0)
		return ret;

	ret = ftdi_usb_set_bit_mode(dev, FTDI_BIT_MODE_RESET);
	if (ret < 0)
		return ret;

	ret = ftdi_usb_set_bit_mode(dev, FTDI_BIT_MODE_MPSSE);
	if (ret < 0)
		return ret;

	return 0;
}

static int ftdi_usb_probe(struct usb_interface *interface,
			  const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(interface);
	int ret;

	(void) id;
	ret = ftdi_usb_reset(dev);
	if (ret < 0) {
		pr_err("Failed to reset FTDI-based device: %d\n", ret);
		return ret;
	}

	pr_alert("Our FTDI-based device has been connected\n");
	return 0;
}

static void ftdi_usb_disconnect(struct usb_interface *interface)
{
	(void) interface;
	pr_alert("Our FTDI-based device has been disconnected\n");
}

static struct usb_driver ftdi_usb_driver = {
	.name = "ftdi_usb",
	.probe = ftdi_usb_probe,
	.disconnect = ftdi_usb_disconnect,
	.id_table = ftdi_id_table,
};

module_usb_driver(ftdi_usb_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FTDI USB-to-serial based something");
MODULE_AUTHOR("Krinkin Mike <krinkin.m.u@gmail.com>");
