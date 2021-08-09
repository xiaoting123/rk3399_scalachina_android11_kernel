/*
 * at24.h - platform_data for the at24 (generic eeprom) driver
 * (C) Copyright 2008 by Pengutronix
 * (C) Copyright 2012 by Wolfram Sang
 * same license as the driver
 */

#ifndef _LINUX_AT24_H
#define _LINUX_AT24_H

#include <linux/types.h>
#include <linux/nvmem-consumer.h>
#include <linux/bitops.h>

/**
 * struct at24_platform_data - data to set up at24 (generic eeprom) driver
 * @byte_len: size of eeprom in byte
 * @page_size: number of byte which can be written in one go
 * @flags: tunable options, check AT24_FLAG_* defines
 * @setup: an optional callback invoked after eeprom is probed; enables kernel
	code to access eeprom via nvmem, see example
 * @context: optional parameter passed to setup()
 *
 * If you set up a custom eeprom type, please double-check the parameters.
 * Especially page_size needs extra care, as you risk data loss if your value
 * is bigger than what the chip actually supports!
 *
 * An example in pseudo code for a setup() callback:
 *
 * void get_mac_addr(struct nvmem_device *nvmem, void *context)
 * {
 *	u8 *mac_addr = ethernet_pdata->mac_addr;
 *	off_t offset = context;
 *
 *	// Read MAC addr from EEPROM
 *	if (nvmem_device_read(nvmem, offset, ETH_ALEN, mac_addr) == ETH_ALEN)
 *		pr_info("Read MAC addr from EEPROM: %pM\n", mac_addr);
 * }
 *
 * This function pointer and context can now be set up in at24_platform_data.
 */

#define DISP_START_REG_ADDR	0x50	/* scala china display timing start reg addr*/
#define DELAY_START_REG_ADDR	0x84	/* scala china display delay start reg addr*/
struct at24_platform_data {
	u32		byte_len;		/* size (sum of all addr) */
	u16		page_size;		/* for writes */
	u8		flags;
#define AT24_FLAG_ADDR16	BIT(7)	/* address pointer is 16 bit */
#define AT24_FLAG_READONLY	BIT(6)	/* sysfs-entry will be read-only */
#define AT24_FLAG_IRUGO		BIT(5)	/* sysfs-entry will be world-readable */
#define AT24_FLAG_TAKE8ADDR	BIT(4)	/* take always 8 addresses (24c00) */
#define AT24_FLAG_SERIAL	BIT(3)	/* factory-programmed serial number */
#define AT24_FLAG_MAC		BIT(2)	/* factory-programmed mac address */
#define AT24_FLAG_NO_RDROL	BIT(1)	/* does not auto-rollover reads to */
					/* the next slave address */

	void		(*setup)(struct nvmem_device *nvmem, void *context);
	void		*context;
};

struct disp_timing
{
	unsigned int clock_freq;
	unsigned short hactive;
	unsigned short vactive;
	unsigned short hfront_porch;
	unsigned short hsync_len;
	unsigned short hback_porch;
	unsigned short vfront_porch;
	unsigned short vsync_len;
	unsigned short vback_porch;
	unsigned short hsync_active;
	unsigned short vsync_active;
	unsigned short de_active;
	unsigned short pixelclk_active;
};

struct timing_id
{
	unsigned int checksum;
	unsigned int length;
	struct disp_timing scala_timing;
};

struct panel_delay
{
	unsigned short prepare_delay_ms;
	unsigned short enable_delay_ms;
	unsigned char reserved[8];
};

enum eeprom_page {
       EEPROM_BANK0 = 0,
       EEPROM_BANK1,
       EEPROM_BANK2,
       EEPROM_BANK3,
       EEPROM_BANK4,
       EEPROM_BANK5,
       EEPROM_BANK6,
       EEPROM_BANK7,
};
void get_scala_timing(struct disp_timing *timing_a);
void get_scala_lcddelay(struct panel_delay *lcddelay);
#endif /* _LINUX_AT24_H */
