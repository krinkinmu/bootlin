// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

// Timeout in milliseconds for USB IO operations
const int FTDI_IO_TIMEOUT = 5000;
const size_t FTDI_IO_BUFFER_SIZE = 65536;
const u16 FTDI_BIT_MODE_RESET = 0x0000;
const u16 FTDI_BIT_MODE_MPSSE = 0x0200;


struct ftdi_usb {
	struct usb_device *udev;
	struct usb_interface *interface;
	void *buffer;
	size_t buffer_size;
	struct i2c_adapter adapter;
};

static int ftdi_usb_i2c_xfer(struct i2c_adapter *adapter,
			     struct i2c_msg *msg, int num)
{
	(void) adapter;
	(void) msg;
	(void) num;
	return -ENOSYS;
}

static u32 ftdi_usb_i2c_func(struct i2c_adapter *adapter)
{
	(void) adapter;
	return I2C_FUNC_I2C;
}

static const struct i2c_algorithm ftdi_usb_i2c_algo = {
	.master_xfer = ftdi_usb_i2c_xfer,
	.functionality = ftdi_usb_i2c_func,
};

static const struct usb_device_id ftdi_id_table[] = {
	{ USB_DEVICE(0x0005, 0x0001) },
	{ }
};
MODULE_DEVICE_TABLE(usb, ftdi_id_table);

static void ftdi_usb_delete(struct ftdi_usb *ftdi)
{
	usb_put_intf(ftdi->interface);
	usb_put_dev(ftdi->udev);
	kfree(ftdi->buffer);
	kfree(ftdi);
}

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
		FTDI_IO_TIMEOUT);
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
		FTDI_IO_TIMEOUT);
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
		FTDI_IO_TIMEOUT);
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
		FTDI_IO_TIMEOUT);
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
		FTDI_IO_TIMEOUT);
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
		FTDI_IO_TIMEOUT);
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
	struct ftdi_usb *ftdi;
	int ret;

	(void) id;
	ret = ftdi_usb_reset(dev);
	if (ret < 0) {
		dev_err(&interface->dev,
			"Failed to reset FTDI-based device: %d\n", ret);
		return ret;
	}

	ftdi = kzalloc(sizeof(*ftdi), GFP_KERNEL);
	if (!ftdi)
		return -ENOMEM;

	ftdi->udev = usb_get_dev(dev);
	ftdi->interface = usb_get_intf(interface);
	ftdi->buffer = kzalloc(FTDI_IO_BUFFER_SIZE, GFP_KERNEL);
	ftdi->buffer_size = FTDI_IO_BUFFER_SIZE;
	if (!ftdi->buffer) {
		dev_err(&interface->dev,
			"Failed to initialize the FTDI-based device: %d\n",
			-ENOMEM);
		ftdi_usb_delete(ftdi);
		return -ENOMEM;
	}

	ftdi->adapter.owner = THIS_MODULE;
	ftdi->adapter.algo = &ftdi_usb_i2c_algo;
	ftdi->adapter.algo_data = ftdi;
	ftdi->adapter.dev.parent = &interface->dev;
	snprintf(ftdi->adapter.name, sizeof(ftdi->adapter.name),
		 "FTDI USB-to-I2C at bus %03d device %03d",
		 dev->bus->busnum, dev->devnum);
	i2c_add_adapter(&ftdi->adapter);

	usb_set_intfdata(interface, ftdi);
	dev_info(&interface->dev, "Initialized FTDI-based device\n");
	return 0;
}

static void ftdi_usb_disconnect(struct usb_interface *interface)
{
	struct ftdi_usb *ftdi = usb_get_intfdata(interface);

	i2c_del_adapter(&ftdi->adapter);
	usb_set_intfdata(interface, NULL);
	ftdi_usb_delete(ftdi);
	dev_info(&interface->dev, "FTDI-based device has been disconnected\n");
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
