// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>

struct wiichuk_state {
	char data[6];
};

struct wiichuk_registers {
	unsigned x;
	unsigned y;
	unsigned x_acc;
	unsigned y_acc;
	unsigned z_acc;
	bool c_pressed;
	bool z_pressed;
};

static int wiichuk_i2c_handshake(const struct i2c_client *client)
{
	const char msg1[] = {0xf0, 0x55};
	const char msg2[] = {0xfb, 0x00};

	int ret = i2c_master_send(client, msg1, sizeof(msg1));
	if (ret < 0)
		return ret;

	mdelay(1);

	ret = i2c_master_send(client, msg2, sizeof(msg2));
	if (ret < 0)
		return ret;

	return 0;
}

static int wiichuk_i2c_read_state(
	const struct i2c_client *client, struct wiichuk_state *state)
{
	const char msg[] = {0x00};
	int ret;

	usleep_range(10000, 20000);
	ret = i2c_master_send(client, msg, sizeof(msg));
	if (ret < 0)
		return ret;

	usleep_range(10000, 20000);
	ret = i2c_master_recv(client, state->data, sizeof(state->data));
	if (ret < 0)
		return ret;

	return 0;
}

// Refer to https://bootlin.com/labs/doc/nunchuk.pdf for details of the
// response format.
static void wiichuk_parse_registers(
	const struct wiichuk_state *state, struct wiichuk_registers *regs)
{
	regs->x = (unsigned)state->data[0];
	regs->y = (unsigned)state->data[1];
	regs->x_acc =
		((unsigned)state->data[2] << 2)
		| ((state->data[5] & GENMASK(3, 2)) >> 2);
	regs->y_acc =
		((unsigned)state->data[3] << 2)
		| ((state->data[5] & GENMASK(5, 4)) >> 4);
	regs->z_acc =
		((unsigned)state->data[4] << 2)
		| ((state->data[5] & GENMASK(7, 6)) >> 6);
	regs->z_pressed = ((BIT(0) & state->data[5]) == 0) ? true : false;
	regs->c_pressed = ((BIT(1) & state->data[5]) == 0) ? true : false;
}

static void wiichuk_registers_dump(const struct wiichuk_registers *regs)
{
	pr_alert("Nintendo Wiichuk Z button is %s\n",
		 regs->z_pressed ? "pressed" : "not pressed");
	pr_alert("Nintendo Wiichuk C button is %s\n",
		 regs->c_pressed ? "pressed" : "not pressed");

	pr_alert("Nintendo Wiichuk position (x, y): (%u, %u)\n",
		 regs->x, regs->y);
	pr_alert("Nintendo Wiichuk acceleration (x, y, z): (%u, %u, %u)\n",
		 regs->x_acc, regs->y_acc, regs->z_acc);
}

static int wiichuk_i2c_read_registers(
	const struct i2c_client *client, struct wiichuk_registers *regs)
{
	struct wiichuk_state state;
	int err;

	// Bootlin training material claim that Nintendo Wiichuk only updates
	// internal state after it's been read. Therefore to get the latest
	// state we have to read it twice: first time to cause an update and
	// the second time to read the updated state.

	err = wiichuk_i2c_read_state(client, &state);
	if (err)
		return err;

	err = wiichuk_i2c_read_state(client, &state);
	if (err)
		return err;

	wiichuk_parse_registers(&state, regs);

	return 0;
}

static int wiichuk_i2c_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct wiichuk_registers regs;
	int err = wiichuk_i2c_handshake(client);

	(void) id;

	if (err) {
		pr_err("Nintendo Wiichuk handshake failed: %d\n", err);
		return err;
	}

	err = wiichuk_i2c_read_registers(client, &regs);
	if (err) {
		pr_err("Failed to read Nintendo Wiichuck registers: %d\n", err);
		return err;
	}

	wiichuk_registers_dump(&regs);

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
