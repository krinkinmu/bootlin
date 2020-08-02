// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>


#define VENDOR_ID  0x0005
#define PRODUCT_ID 0x0001


static const struct usb_device_id ftdi_id_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ }
};
MODULE_DEVICE_TABLE(usb, ftdi_id_table);

static int ftdi_usb_probe(struct usb_interface *interface,
			  const struct usb_device_id *id)
{
	(void) interface;
	(void) id;
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
