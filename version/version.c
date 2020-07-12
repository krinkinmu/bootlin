// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

static int __init version_init(void)
{
	pr_alert("%s version %s: %s\n",
		utsname()->sysname,
		utsname()->release,
		utsname()->version);
	return 0;
}

static void __exit version_exit(void)
{
	pr_alert("Goodbye, World!\n");
}

module_init(version_init);
module_exit(version_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux Kernel Version module");
MODULE_AUTHOR("Krinkin Mike <krinkin.m.u@gmail.com>");
