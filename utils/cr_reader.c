#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "cr.h"

dev_t dev = 0;
static struct cdev etx_cdev;
static struct class *dev_class;

static int cr_open(struct inode *inode, struct file *file) { return 0; }
static int cr_release(struct inode *inode, struct file *file) { return 0; }
static long cr_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

  switch (cmd) {

  case CR_READ:
    crs values = {0};
    __asm__ __volatile__("mov %%cr0, %%rax\n\t"
                         "mov %%eax, %0\n\t"
                         "mov %%cr2, %%rax\n\t"
                         "mov %%eax, %1\n\t"
                         "mov %%cr3, %%rax\n\t"
                         "mov %%eax, %2\n\t"
                         : "=m"(values.cr0), "=m"(values.cr2), "=m"(values.cr3)
                         : /* no input */
                         : "%rax");

    if (copy_to_user((ssize_t *)arg, &values, sizeof(values))) {
      return -EINVAL;
    }
    break;

  default:
    break;
  }

  return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = NULL,
    .write = NULL,
    .open = cr_open,
    .release = cr_release,
    .unlocked_ioctl = cr_ioctl,
};

static int __init cr_reader_init(void) {
  /*Allocating Major number*/
  if ((alloc_chrdev_region(&dev, 0, 1, "cr_reader")) < 0) {
    pr_err("Cannot allocate major number for device\n");
    return -1;
  }

  cdev_init(&cr_dev, &fops);

  if (cdev_add(&cr_dev, dev, 1) < 0) {
    goto unreg_class;
  }

  if (IS_ERR(dev_class = class_create(THIS_MODULE, "cr_class")) {
    pr_err("Failed to register device class");
    goto unreg_class;
  }

  if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "etx_device"))) {
    pr_err("Cannot create the Device\n");
    goto r_device;
  }

  pr_info("Kernel Module Inserted Successfully...\n");
r_device:
  class_destroy(dev_class);

unreg_class:
  unregister_chrdev_region(dev, 1);
  return 0;
}

static void __exit cr_reader_exit(void) {
  device_destroy(dev_class, dev);
  class_destroy(dev_class);
  unregister_chrdev_region(dev, 1);
  pr_info("Kernel Module Removed Successfully...\n");
}

module_init(cr_reader_init);
module_exit(cr_reader_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sbancuz");
MODULE_DESCRIPTION("Read Control Registers");
