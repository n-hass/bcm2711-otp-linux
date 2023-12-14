#include <linux/module.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/platform_device.h>

#include "bcm28xx_otp.h"

#define MIN_OTP_ROWS 8
#define MAX_OTP_ROWS 66

static char command_buf[100]; // Buffer to store commands from 'in'
static char output_buf[100];  // Buffer to store data for 'out'

static struct platform_device *otp_platform_device;
static struct kobject *otp_kobject;

void __iomem *otp_base;


static ssize_t in_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%s\n", command_buf);
}

static ssize_t in_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
  char op;
  int temp_row_num;
  uint32_t new_val = 0;
  uint8_t row_num;

  int items_read = sscanf(buf, "%c %d %x", &op, &temp_row_num, &new_val);

  // validation
  if (op == 'c') {
    if (items_read != 1) {
      goto invalid;
    }
  }
  else if ( (op == 'r' && items_read == 2) || (op == 'w' && items_read == 3) ) {  
    if (temp_row_num < MIN_OTP_ROWS || temp_row_num > MAX_OTP_ROWS) {
      goto invalid;
    }
  }
  else {
    invalid:
    printk(KERN_WARNING "Invalid command format\n");
    return -EINVAL;
  }


  row_num = (uint8_t)temp_row_num;

  int res;
  switch (op) {
    case 'r':
      // Perform the read operation
      printk(KERN_INFO "Reading OTP row: %u\n", row_num);
      sprintf(output_buf, "%08x\n", otp_read(row_num));
      break;
    case 'w':
      // Perform the write operation
    #ifdef WRITE_ENABLED
      printk(KERN_INFO "Writing 0x%x to OTP row %u\n", new_val, row_num);
      res = otp_write(row_num, new_val);
      sprintf(output_buf, "w %02d : %d\n", row_num, res);
      if (res) {
        printk(KERN_WARNING "Write to OTP %02d failed, error: %d\n", row_num, res);
        return -EIO;
      }
      printk(KERN_INFO "Write to OTP %02d complete\n", row_num);
    #else
      printk(KERN_WARNING "Write operation not enabled\n");
      return -EPERM;
    #endif
      break;
    case 'c':
      // Clear the output buffer
      memset(output_buf, 0, sizeof(output_buf));
      break;
    default:
      printk(KERN_WARNING "Invalid command\n");
      return -EINVAL;
      break;
  }

  strncpy(command_buf, buf, sizeof(command_buf));
  return count;
}

static ssize_t out_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%s\n", output_buf);
}

static struct kobj_attribute in_attr = __ATTR(in, 0660, in_show, in_store);
static struct kobj_attribute out_attr = __ATTR(out, 0440, out_show, NULL);


// // //

static int __init init_mod(void) {
  otp_base = ioremap(OTP_BASE_ADDR, MEM_MAPPING_SIZE);
  if (!otp_base) {
    printk(KERN_ERR "ioremap failed\n");
    return -EIO;
  }
  // uint8_t row = 29;
  // printk(KERN_INFO "Value at OTP row %u: 0x%x\n", row, otp_read(row));

  int error = 0;
  otp_platform_device = platform_device_register_simple("otp", -1, NULL, 0);
  if (IS_ERR(otp_platform_device)) {
    printk(KERN_ALERT "Failed to register platform device\n");
    return PTR_ERR(otp_platform_device);
  }

  otp_kobject = kobject_create_and_add("interface", &otp_platform_device->dev.kobj);
  if (!otp_kobject) {
    platform_device_unregister(otp_platform_device);
    return -ENOMEM;
  }

  error = sysfs_create_file(otp_kobject, &in_attr.attr);
  if (error) {
    printk(KERN_ALERT "Failed to create the 'in' sysfs file\n");
    return error;
  }

  error = sysfs_create_file(otp_kobject, &out_attr.attr);
  if (error) {
    printk(KERN_ALERT "Failed to create the 'out' sysfs file\n");
    kobject_put(otp_kobject);
    return error;
  }

  return 0;
}

static void __exit exit_mod(void) {
  if (otp_base) {
    iounmap(otp_base);
  }
  kobject_put(otp_kobject);
  platform_device_unregister(otp_platform_device);
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicholas Hassan");
MODULE_DESCRIPTION("BCM2711 OTP reader");