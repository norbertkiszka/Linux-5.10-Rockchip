// SPDX-License-Identifier: GPL-2.0-only
/*
 * hdcode_gpio - read HW number (byte)
 *
 * Copyright (c) 2024 Norbert Kiszka <linux@elektrykplakal.pl>
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>

#define MODULE_NAME "hdcode_gpio"

#define GPIO_HD_CODE0 4
#define GPIO_HD_CODE1 8
#define GPIO_HD_CODE2 11
#define GPIO_HD_CODE3 12

MODULE_AUTHOR("Norbert Kiszka");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_LICENSE("GPL");

static uint8_t data = 0;
static dev_t dev = 0;
static int dev_major_number = 0;
static struct class *hdcode_class = NULL;
static struct cdev hdcode_cdev;

static int gpio_hdcode_drv_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long gpio_hdcode_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static ssize_t gpio_hdcode_drv_read(struct file *file, char __user *buf, size_t len, loff_t *f_pos)
{
	if(*f_pos > 0)
		return 0;

	if(copy_to_user(buf, &data, 1))
		return -EFAULT;

	*f_pos += 1;
	return 1;
}

static ssize_t gpio_hdcode_drv_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	if(count <= 0)
	{
		printk(KERN_ERR "Tried to write hdcode_gpio with empty data\n");
		return 0;
	}
	
	//if(count > 1)
	//	printk(KERN_WARNING "hdcode_gpio written with %zu bytes. Should be one byte only.\n", count);
	
	memcpy(&data, (const void __force *)buf, 1);
	printk("hdcode_gpio changed to: %u\n", data);
	
	return count;
}

static int gpio_hdcode_drv_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations gpio_hdcode_fops = {
	.owner           = THIS_MODULE,
	.open            = gpio_hdcode_drv_open,
	.release         = gpio_hdcode_drv_release,
	.unlocked_ioctl  = gpio_hdcode_drv_ioctl,
	.read            = gpio_hdcode_drv_read,
	.write           = gpio_hdcode_drv_write
};

static int hdcode_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static int __init gpio_hdcode_init(void)
{
	int val;
	struct gpio_desc *desc;
	
	data = 0;
	
	if(alloc_chrdev_region(&dev, 0, 1, "hdcode_device") < 0)
	{
		printk(KERN_ERR "hdcode_gpio: can't allocate major number\n");
		return -1;
	}
	
	dev_major_number = MAJOR(dev);
	
	hdcode_class = class_create(THIS_MODULE, "hdcode_device");
	hdcode_class->dev_uevent = hdcode_uevent;
	
	cdev_init(&hdcode_cdev, &gpio_hdcode_fops);
	hdcode_cdev.owner = THIS_MODULE;
	
	if((cdev_add(&hdcode_cdev, MKDEV(dev_major_number, 0), 1)) < 0)
	{
		printk(KERN_ERR "Can't add the device hdcode_gpio to the system\n");
		goto r_class;
	}
	
	if((device_create(hdcode_class, NULL, MKDEV(dev_major_number, 0), NULL, "hdcode_gpio")) < 0)
	{
		printk(KERN_ERR "Register hdcode_gpio device failed!\n");
		goto r_device;
	}
	
	if(gpio_request(GPIO_HD_CODE0, "hd_code0"))
	{
		printk(KERN_ERR "Failed to register gpio hd_code0 %d\n", GPIO_HD_CODE0);
		goto r_device;
	}
	desc = gpio_to_desc(GPIO_HD_CODE0);
	if(desc == NULL)
	{
		printk(KERN_ERR "Failed to get gpio hd_code0 descriptor\n");
		goto free_0;
	}
	gpiod_direction_input(desc);
	val = gpiod_get_raw_value(desc);
	printk("hd_code0 = %d\n", val);
	if(val)
		data = 1;
	
	if(gpio_request(GPIO_HD_CODE1, "hd_code1"))
	{
		printk(KERN_ERR "Failed to register gpio hd_code1 %d\n", GPIO_HD_CODE1);
		goto free_0;
	}
	desc = gpio_to_desc(GPIO_HD_CODE1);
	if(desc == NULL)
	{
		printk(KERN_ERR "Failed to get gpio hd_code1 descriptor\n");
		goto free_1;
	}
	gpiod_direction_input(desc);
	val = gpiod_get_raw_value(desc);
	printk("hd_code1 = %d\n", val);
	if(val)
		data = data | 2;
	
	if(gpio_request(GPIO_HD_CODE2, "hd_code2"))
	{
		printk(KERN_ERR "Failed to register gpio hd_code2 %d\n", GPIO_HD_CODE1);
		goto free_1;
	}
	desc = gpio_to_desc(GPIO_HD_CODE2);
	if(desc == NULL)
	{
		printk(KERN_ERR "Failed to get gpio hd_code2 descriptor\n");
		goto free_2;
	}
	gpiod_direction_input(desc);
	val = gpiod_get_raw_value(desc);
	printk("hd_code2 = %d\n", val);
	if(val)
		data = data | 4;
	
	if(gpio_request(GPIO_HD_CODE3, "hd_code3"))
	{
		printk(KERN_ERR "Failed to register gpio hd_code3 %d\n", GPIO_HD_CODE1);
		goto free_2;
	}
	desc = gpio_to_desc(GPIO_HD_CODE3);
	if(desc == NULL)
	{
		printk(KERN_ERR "Failed to get gpio hd_code3 descriptor\n");
		goto free_3;
	}
	gpiod_direction_input(desc);
	val = gpiod_get_raw_value(desc);
	printk("hd_code3 = %d\n", val);
	if(val)
		data = data | 8;
	
	printk(KERN_INFO "hdcode_gpio loaded successfully. HW: %u\n", data);

	return 0;

free_3:
	gpio_free(GPIO_HD_CODE3);
free_2:
	gpio_free(GPIO_HD_CODE2);
free_1:
	gpio_free(GPIO_HD_CODE1);
free_0:
	gpio_free(GPIO_HD_CODE0);
r_device:
	class_destroy(hdcode_class);
r_class:
	unregister_chrdev_region(MKDEV(dev_major_number, 0), 1);
	return -1;
}

static void __exit gpio_hdcode_exit(void)
{
	printk(KERN_WARNING "hdcode_gpio removed\n");
	
	gpio_free(GPIO_HD_CODE0);
	gpio_free(GPIO_HD_CODE1);
	gpio_free(GPIO_HD_CODE2);
	gpio_free(GPIO_HD_CODE3);
	
	device_destroy(hdcode_class, MKDEV(dev_major_number, 0));

	class_unregister(hdcode_class);
	class_destroy(hdcode_class);

	unregister_chrdev_region(MKDEV(dev_major_number, 0), 1);
	return;
}

module_init(gpio_hdcode_init);
module_exit(gpio_hdcode_exit);
