/*
 * scmcpld.c - The i2c driver for the CPLD on ELBERT
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <i2c_dev_sysfs.h>

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

static const i2c_dev_attr_st scmcpld_attr_table[] = {
  {
    "cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 8,
  },
  {
    "cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2, 0, 8,
  },
  {
    "scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x5, 0, 8,
  },
  {
    "flash_rate_upper",
    "Upper 8 bit of flash rate",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 8,
  },
  {
    "flash_rate_lower",
    "Lower 8 bit of flash rate",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 8,
  },
  {
    "system_led_amber",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 6, 1,
  },
  {
    "system_led_blue",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 5, 1,
  },
  {
    "system_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 4, 1,
  },
  {
    "system_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 3, 1,
  },
  {
    "system_led_flash",
    "0x1: Flash per flash_rate registers\n"
    "0x0: No flash",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 3,
  },
  {
    "fan_led_amber",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 6, 1,
  },
  {
    "fan_led_blue",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 5, 1,
  },
  {
    "fan_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 4, 1,
  },
  {
    "fan_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 3, 1,
  },
  {
    "fan_led_flash",
    "0x1: Flash per flash_rate registers\n"
    "0x0: No flash",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 3,
  },
  {
    "psu_led_amber",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 6, 1,
  },
  {
    "psu_led_blue",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 5, 1,
  },
  {
    "psu_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 4, 1,
  },
  {
    "psu_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 3, 1,
  },
  {
    "psu_led_flash",
    "0x1: Flash per flash_rate registers\n"
    "0x0: No flash",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 3,
  },
  {
    "sc_led_amber",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 6, 1,
  },
  {
    "sc_led_blue",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 5, 1,
  },
  {
    "sc_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 4, 1,
  },
  {
    "sc_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 3, 1,
  },
  {
    "sc_led_flash",
    "0x1: Flash per flash_rate registers\n"
    "0x0: No flash",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 0, 3,
  },
  {
    "beacon_len",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 5, 1,
  },
  {
    "switchcard_powergood",
    "0x1: Power is good\n"
    "0x0: Power is BAD",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "program_sel",
    "0x1: Select programming SMB Main FPGA\n"
    "0x0: Other devices",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "jtag_en",
    "0x1: Enable JTAG to SMB\n"
    "0x0: Disable JTAG to SMB",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "chassis_power_cycle",
    "Write 0xDE to power-cycle the chassis",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x60, 0, 8,
  },
  {
    "bios_select",
    "0x1: Enable BIOS SPI access\n"
    "0x0: Disable BIOS SPI access",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x70, 0, 1,
  },
  {
    "cpu_cat_err",
    "0x1: CPU has a CAT_ERR\n"
    "0x0: CPU is functional",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 2, 1,
  },
  {
    "cpu_ready",
    "0x1: CPU is ready\n"
    "0x0: CPU is not ready",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 1, 1,
  },
  {
    "cpu_control",
    "0x1: Latch all BMC CTRL signals to CPU\n"
    "0x0: Unlatch all BMC CTRL signals to CPU",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 0, 1,
  },
  {
    "chassis_eeprom_wp",
    "0x1: Disable IDPROM write(default)\n"
    "0x0: Enable IDPROM write",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 1, 1,
  },
  {
    "scm_eeprom_wp",
    "0x1: Disable IDPROM write(default)\n"
    "0x0: Enable IDPROM write",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 0, 1,
  },
  {
    "uart_selection",
    "Write 0xA5 to switch FP UART connection to CPU",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 0, 8,
  },
  {
    "oob_eeprom_cmd_3",
    "OOB EEPROM Command [31:24]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x90, 0, 8,
  },
  {
    "oob_eeprom_cmd_2",
    "OOB EEPROM Command [23:16]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x91, 0, 8,
  },
  {
    "oob_eeprom_cmd_1",
    "OOB EEPROM Command [15:8]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x92, 0, 8,
  },
  {
    "oob_eeprom_cmd_0",
    "OOB EEPROM Command [7:0]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x93, 0, 8,
  },
  {
    "oob_eeprom_resp_3",
    "OOB EEPROM Status/Response [31:24]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x94, 0, 8,
  },
  {
    "oob_eeprom_resp_2",
    "OOB EEPROM Status/Response [23:16]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x95, 0, 8,
  },
  {
    "oob_eeprom_resp_1",
    "OOB EEPROM Status/Response [15:8]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x96, 0, 8,
  },
  {
    "oob_eeprom_resp_0",
    "OOB EEPROM Status/Response [7:0]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x97, 0, 8,
  },
  {
    "oob_eeprom_resp_trigger",
    "OOB EEPROM Status/Response Trigger",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x98, 0, 8,
  },
  {
    "oob_switch_sel",
    "0x1: 53134P EEPROM (default)\n"
    "0x0: 53134P SPI",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x99, 1, 1,
  },
  {
    "oob_switch_reset",
    "0x1: 53134 in reset\n"
    "0x0: 53134 out of reset\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x99, 0, 1,
  },
};

static i2c_dev_data_st scmcpld_data;

/*
 * SCM CPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x43, I2C_CLIENT_END
};

/* ELBERTCPLD id */
static const struct i2c_device_id scmcpld_id[] = {
  { "scmcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, scmcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int scmcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the ELBERTCPLD
   */
  strlcpy(info->type, "scmcpld", I2C_NAME_SIZE);
  return 0;
}

static int scmcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(scmcpld_attr_table) / sizeof(scmcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &scmcpld_data,
                                 scmcpld_attr_table, n_attrs);
}

static int scmcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &scmcpld_data);
  return 0;
}

static struct i2c_driver scmcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "scmcpld",
  },
  .probe    = scmcpld_probe,
  .remove   = scmcpld_remove,
  .id_table = scmcpld_id,
  .detect   = scmcpld_detect,
  .address_list = normal_i2c,
};

static int __init scmcpld_mod_init(void)
{
  return i2c_add_driver(&scmcpld_driver);
}

static void __exit scmcpld_mod_exit(void)
{
  i2c_del_driver(&scmcpld_driver);
}

MODULE_AUTHOR("Dean Kalla");
MODULE_DESCRIPTION("ELBERT SCM CPLD Driver");
MODULE_LICENSE("GPL");

module_init(scmcpld_mod_init);
module_exit(scmcpld_mod_exit);
