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
static struct cdev cr_cdev;
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
                         "mov %%cr4, %%rax\n\t"
                         "mov %%eax, %3\n\t"
                         : "=m"(values.cr0), "=m"(values.cr2), "=m"(values.cr3),
                           "=m"(values.cr4)
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
  int ret;

  // Allocate device numbers
  ret = alloc_chrdev_region(&dev, 0, 1, "cr_reader");
  if (ret < 0) {
    pr_err("Cannot allocate major number\n");
    return ret;
  }

  // Init and add cdev
  cdev_init(&cr_cdev, &fops);
  ret = cdev_add(&cr_cdev, dev, 1);
  if (ret < 0) {
    pr_err("cdev_add failed\n");
    goto unregister_dev;
  }

  // Create class
  dev_class = class_create("cr_class");
  if (IS_ERR(dev_class)) {
    pr_err("Failed to create class\n");
    ret = PTR_ERR(dev_class);
    goto del_cdev;
  }

  // Create device
  if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "cr_device"))) {
    pr_err("Failed to create device\n");
    ret = PTR_ERR(dev_class);
    goto destroy_class;
  }

  pr_info("Kernel Module Inserted Successfully\n");
  return 0;

destroy_class:
  class_destroy(dev_class);

del_cdev:
  cdev_del(&cr_cdev);

unregister_dev:
  unregister_chrdev_region(dev, 1);
  return ret;
}

static void __exit cr_reader_exit(void) {
  device_destroy(dev_class, dev);
  class_destroy(dev_class);
  cdev_del(&cr_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Kernel Module Removed Successfully\n");
}

module_init(cr_reader_init);
module_exit(cr_reader_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sbancuz");
MODULE_DESCRIPTION("Read Control Registers");
