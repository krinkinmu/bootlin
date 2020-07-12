// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

static int wiichuk_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	(void) client;
	(void) id;

	pr_alert("Wiichuk device \"detected\".\n");
	return 0;
}

static int wiichuk_i2c_remove(struct i2c_client *client)
{
	(void) client;

	pr_alert("Wiichuk device \"removed\".\n");
	return 0;
}

static const struct of_device_id wiichuk_of_match[] = {
	{ .compatible = "nintendo,nunchuk" },
	{ },
};

MODULE_DEVICE_TABLE(of, wiichuk_of_match);

static const struct i2c_device_id wiichuk_i2c_id[] = {
	{ "wiichuk_i2c", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, wiichuk_i2c_id);

static struct i2c_driver wiichuk_i2c_driver = {
	.driver = {
		.name = "wiichuk_i2c",
		.of_match_table = wiichuk_of_match
	},
	.probe = wiichuk_i2c_probe,
	.remove = wiichuk_i2c_remove,
	.id_table = wiichuk_i2c_id,
};

module_i2c_driver(wiichuk_i2c_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Nintendo Wiichuk I2C driver");
MODULE_AUTHOR("Krinkin Mike <krinkin.m.u@gmail.com>");
