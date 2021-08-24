#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h> // udelay()
#include <linux/device.h> // device_create()
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/version.h>      /* constant of kernel version */
#include <linux/uaccess.h> // get_user()


#include <linux/string.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/fb.h>
#include <linux/slab.h>
#define SCALADEBUG

#define APDS9300_DRV_NAME "apds9300"
#define APDS9300_IRQ_NAME "apds9300_event"
#define LIGHT 1
/* Command register bits */
#define APDS9300_CMD	BIT(7) /* Select command register. Must write as 1 */
#define APDS9300_WORD	BIT(5) /* I2C write/read: if 1 word, if 0 byte */
#define APDS9300_CLEAR	BIT(6) /* Interrupt clear. Clears pending interrupt */

/* Register set */
#define APDS9300_CONTROL	0x00 /* Control of basic functions */
#define APDS9300_THRESHLOWLOW	0x02 /* Low byte of low interrupt threshold */
#define APDS9300_THRESHHIGHLOW	0x04 /* Low byte of high interrupt threshold */
#define APDS9300_INTERRUPT	0x06 /* Interrupt control */
#define APDS9300_DATA0LOW	0x0c /* Low byte of ADC channel 0 */
#define APDS9300_DATA1LOW	0x0e /* Low byte of ADC channel 1 */

/* Power on/off value for APDS9300_CONTROL register */
#define APDS9300_POWER_ON	0x03
#define APDS9300_POWER_OFF	0x00

/* Interrupts */
#define APDS9300_INTR_ENABLE	0x10
/* Interrupt Persist Function: Any value outside of threshold range */
#define APDS9300_THRESH_INTR	0x01

#define APDS9300_THRESH_MAX	0xffff /* Max threshold value */

/***************
scalalight@49 {
	compatible = "scalalight,scalalight-apds9300";
	reg = <0x49>;	
}

***********/

struct i2c_client *mClient = 0;
int power_state;




#define SCALALIGHT_NAME "scalalight"

#define I2CADR client->addr

#define FM_PROC_FILE "scalalight"



/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/


/* Lux calculation */

/* Calculated values 1000 * (CH11.4 for CH1/CH0 from 0 to 0.52 */
static const u16 apds9300_lux_ratio[] = {
	0, 2, 4, 7, 11, 15, 19, 24, 29, 34, 40, 45, 51, 57, 64, 70, 77, 84, 91,
	98, 105, 112, 120, 128, 136, 144, 152, 160, 168, 177, 185, 194, 203,
	212, 221, 230, 239, 249, 258, 268, 277, 287, 297, 307, 317, 327, 337,
	347, 358, 368, 379, 390, 400,
};

static unsigned long apds9300_calculate_lux(u16 ch0, u16 ch1)
{
	unsigned long lux, tmp;

	/* avoid division by zero */
	if (ch0 == 0)
		return 0;

	tmp = DIV_ROUND_UP(ch1 * 100, ch0);
	if (tmp <= 52) {
		lux = 3150 * ch0 - (unsigned long)DIV_ROUND_UP_ULL(ch0
				* apds9300_lux_ratio[tmp] * 5930ull, 1000);
	} else if (tmp <= 65) {
		lux = 2290 * ch0 - 2910 * ch1;
	} else if (tmp <= 80) {
		lux = 1570 * ch0 - 1800 * ch1;
	} else if (tmp <= 130) {
		lux = 338 * ch0 - 260 * ch1;
	} else {
		lux = 0;
	}

	return lux / 10000;
}


static int apds9300_get_adc_val(struct i2c_client *client, int adc_number)
{
	int ret;
	u8 flags = APDS9300_CMD | APDS9300_WORD;

	if (!power_state)
		return -EBUSY;

	/* Select ADC0 or ADC1 data register */
	flags |= adc_number ? APDS9300_DATA1LOW : APDS9300_DATA0LOW;

	ret = i2c_smbus_read_word_data(client, flags);
	if (ret < 0)
		printk("failed to read ADC%d value\n", adc_number);

	return ret;
}

static int apds9300_set_power_state(struct i2c_client *client, int state)
{
	int ret;
	u8 cmd;

	cmd = state ? APDS9300_POWER_ON : APDS9300_POWER_OFF;
	ret = i2c_smbus_write_byte_data(client,APDS9300_CONTROL | APDS9300_CMD, cmd);
	if (ret) {
		printk("failed to set power state %d ret = %d\n", state,ret);

	}

		
	power_state = state;

	return 0;
}



static int apds9300_chip_init(struct i2c_client *client)
{
	int ret;

	/* Need to set power off to ensure that the chip is off */
	ret = apds9300_set_power_state(client, 0);
	if (ret < 0)
		goto err;
	/*
	 * Probe the chip. To do so we try to power up the device and then to
	 * read back the 0x03 code
	 */
	ret = apds9300_set_power_state(client, 1);
	if (ret < 0)
		goto err;
	ret = i2c_smbus_read_byte_data(client,APDS9300_CONTROL | APDS9300_CMD);
			
	if (ret != APDS9300_POWER_ON) {
		ret = -ENODEV;
		goto err;
	}

	return 0;

err:
	return ret;
}

static int apds9300_read_raw(struct i2c_client *client, int val)
{
	int ch0, ch1, ret = -EINVAL;

//#ifdef LIGHT
		ch0 = apds9300_get_adc_val(client, 0);
		if (ch0 < 0) {
			ret = ch0;
		}
		ch1 = apds9300_get_adc_val(client, 1);
		if (ch1 < 0) {
			ret = ch1;
		}
		val = apds9300_calculate_lux(ch0, ch1);
		printk("|||||apds9300_calculate_lux|||   val = %d  ch0  = %d  ch1 = %d\n",val,ch0,ch1);
		ret = val;
		
//#else

		/*ret = apds9300_get_adc_val(client, 0);
		if (ret < 0)
			val = ret;
		ret = 1;*/
//#endif

	return ret;
}





int scalalight_open(struct inode *inode,struct file *filp)
{
//	DPRINTK("Device Opened Success!\n");		
	return nonseekable_open(inode,filp);
}

int scalalight_release(struct inode *inode,struct file *filp)
{
//	DPRINTK("Device Closed Success!\n");
	return 0;
}



ssize_t scalalight_read(struct file *file, char __user *buf,size_t count, loff_t *offset)
{
  int ret = -1;
  int val = 0;
  
  printk("scala scalalight_read\n");

  ret = apds9300_read_raw(mClient,val);
	if(ret < 0)
		printk("ret = %d",ret);

  printk("\r\n");
  //read data to user
  if(!copy_to_user((char *)buf, &ret, sizeof(val))){
		 printk("\r\n");
         return val;
  }else{
     return -1;
  }
}


ssize_t scalalight_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
  char alpha[4];
  int cnt = 0;
  memset(alpha, 0, 4);
  printk("\r\n");
  if(count > 99)
  {
         cnt = 99;
  }
  else
  {
       cnt = count;
  }
	//send data
  if(!copy_from_user((char *)alpha, buf, cnt))
  {  
		 printk("write:");
         printk("\r\n");
         return cnt;
  }  
  else
  {
         return -1;
  }
}


static struct file_operations scalalight_ops = {
	.write   = scalalight_write,
	.read    = scalalight_read,
	.owner 	= THIS_MODULE,
	.open 	= scalalight_open,
	.release= scalalight_release,
};

static struct miscdevice scalalight_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.fops	= &scalalight_ops,
	.name	= "scalalight",
};



static struct of_device_id msm_match_table[] = {
	{.compatible = "scalalight,scalalight-apds9300"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_match_table);



static ssize_t level_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)
{
	return count;
}

static ssize_t level_show(struct class *cls,struct class_attribute *attr,char *_buf)
{
	int ret;
	int val = 0;
	

	ret = apds9300_read_raw(mClient,val);
	return sprintf(_buf, "%u\n", ret);
}

static CLASS_ATTR_RW(level);

static int scalalight_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	
	static struct class *lsensor = NULL; 

	int ret = 0;
	mClient = client;
	ret = misc_register(&scalalight_dev);
	
	if(ret < 0)
		goto err;
	
	printk("scalalight_i2c_probe\n");
	
	lsensor = class_create(THIS_MODULE, "lsensor");
	if (IS_ERR(lsensor))
	{
		printk("sensordata fail.\n");
		ret = -ENOMEM;
	}
	
	ret = class_create_file(lsensor,&class_attr_level);
		
	ret = apds9300_chip_init(mClient);
	if(ret < 0)
		goto err;

	
	
	return 0;
	
	
	
err:
	return ret;
}

static int scalalight_remove(struct i2c_client *client)
{
    int err = 0;
    
    printk("scalalight_remove\n");

    
    return err;

}

static const struct i2c_device_id scalalight_id[] = {
	{"scalalight-i2c", 0},
	{}
};

static struct i2c_driver scalalight_driver = {
	.id_table = scalalight_id,
	.probe = scalalight_probe,
	.remove = scalalight_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "scalalight-i2c",
		.of_match_table = msm_match_table,
	},
};

/*
 * module load/unload record keeping
 */

static int __init scalalight_dev_init(void)
{
	printk("scalalight_dev_init\n");
	return i2c_add_driver(&scalalight_driver);
}
module_init(scalalight_dev_init);

static void __exit scalalight_dev_exit(void)
{
	i2c_del_driver(&scalalight_driver);
}
module_exit(scalalight_dev_exit);

MODULE_AUTHOR("SCALA");
MODULE_DESCRIPTION("scalalight transceiver driver");
MODULE_LICENSE("GPL");


