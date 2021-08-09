#include <dt-bindings/gpio/gpio.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/slab.h>
#include <linux/delay.h>
//#include "enable.h"
extern int eeprom_pcbversion_id;
static struct of_device_id remote_led_of_match[] = {
	{ .compatible = "remote_led" },
	{ }
};

MODULE_DEVICE_TABLE(of, remote_led_of_match);


int flag_input=0;
int flag_tp=0;
static struct class *remote_gpio = NULL;
int remote_test=1;
unsigned int  tp_flag=0x00;
unsigned int  input_flag=0x00;
unsigned int  _otgnoovo=0x00;
unsigned int  _lednoovo=0x00;
unsigned int  _level=0x01;
struct device_node *remote_node;

static unsigned int gpio_4g;
static int gpio_4g_value;

static ssize_t net_lte_show(struct class *cls,struct class_attribute *attr, char *_buf)
{
	return sprintf(_buf, "%d\n", gpio_4g_value);
}
static ssize_t tp_enable_show(struct class *cls,struct class_attribute *attr, char *_buf)
{
	return sprintf(_buf, "%u\n", tp_flag);
}

static ssize_t input_enable_show(struct class *cls,struct class_attribute *attr, char *_buf)
{
	return sprintf(_buf, "%u\n", input_flag);
}

static ssize_t led_noovo_show(struct class *cls,struct class_attribute *attr, char *_buf)
{
	return sprintf(_buf, "%u\n", _lednoovo);
}

static ssize_t otg_noovo_show(struct class *cls,struct class_attribute *attr, char *_buf)
{
	return sprintf(_buf, "%u\n", _otgnoovo);
}

static ssize_t led_noovo_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)
{
	int value = simple_strtoul(buf, NULL, 0);
	_lednoovo=value;
	// set value
#ifdef CONFIG_PRODUCT_BOX
	if(_lednoovo==0){
// wake
		gpio_direction_output(71,0);
		if(eeprom_pcbversion_id == 0){
			gpio_direction_output(146,0);
		}else{
			gpio_direction_output(146,1);
		}
    }else if(_lednoovo==1){
//sleep
		gpio_direction_output(71,1);
		if(eeprom_pcbversion_id ==0){
			gpio_direction_output(146,1);
		}else{
			gpio_direction_output(146,0);}
		}
	}
	else if(_lednoovo==2){
		printk("led_noovo_store 2\n");
		gpio_direction_output(71,0);
		if(eeprom_pcbversion_id ==0){
			printk("led_noovo_store set 1\n");
			gpio_direction_output(146,1);
		}else{
			printk("led_noovo_store set 0\n");
			gpio_direction_output(146,0);
		}
	}
#endif
        return count;
}





static ssize_t otg_noovo_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)
{
	int value = simple_strtoul(buf, NULL, 0);
	_otgnoovo=value;
	// set value
	if(_otgnoovo==0){
		//otg
		gpio_direction_input(156);
		msleep(1000);
		gpio_direction_output(153,0);
		msleep(1000);
		gpio_direction_output(124,0);
	}else if(_otgnoovo==1){
		//host
		gpio_direction_output(156,0);
		msleep(1000);
		gpio_direction_output(153,1);
		msleep(1000);
		gpio_direction_output(124,1);
	}
	return count;
}



static ssize_t tp_enable_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)

{
	int value = simple_strtoul(buf, NULL, 0);
	tp_flag=value;
	if(tp_flag == 0){
		flag_tp=0;
	}else if(tp_flag == 1){
		flag_tp=1;

	}
	return count;
}
static ssize_t input_enable_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)

{
	int value = simple_strtoul(buf, NULL, 0);
	input_flag =value;
	if(input_flag == 0){
		flag_input=0;
	}else if(input_flag == 4){
		flag_input=4;

	}else if(input_flag == 2){
		flag_input =2;
	}else if(input_flag == 6){
		flag_input =6;
	}else if(input_flag == 7){
		flag_input =7;
	}
	return count;
}

static ssize_t net_lte_store(struct class *cls,struct class_attribute *attr, const char __user *buf,size_t count)
{
        int value = simple_strtoul(buf, NULL, 0);
        // set value
        if(value==0){
		gpio_direction_output(gpio_4g,0);
                gpio_4g_value = 0;
        }else if(value==1){
                gpio_direction_output(gpio_4g,1);
                gpio_4g_value = 1;
        }else{
		printk("invalid value to 4g power gpio");
        }

        return count;
}


static CLASS_ATTR_RW(net_lte);
static CLASS_ATTR_RW(tp_enable);
static CLASS_ATTR_RW(input_enable);
static CLASS_ATTR_RW(otg_noovo);
static CLASS_ATTR_RW(led_noovo);

static int remote_led_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
    enum of_gpio_flags flags;
	int ret;
	int error;
	if (!node)
		return -ENODEV;

    gpio_4g = of_get_named_gpio_flags(node, "power4g-gpios", 0, &flags);
    if (!gpio_is_valid(gpio_4g)) {
        printk("invalid 4G power gpio specified\n");
	}else{
	      error = gpio_direction_output(gpio_4g , 1);
              if(error < 0){
                   printk("invalid 4G power gpio control error\n");
              }
              gpio_4g_value = 1;
        }

	remote_node = node;

	remote_gpio = class_create(THIS_MODULE, "remote_gpio");
	if (IS_ERR(remote_gpio))
	{
		printk("remote gpio fail.\n");
		ret = -ENOMEM;
	}
	ret = class_create_file(remote_gpio,&class_attr_tp_enable);
	ret = class_create_file(remote_gpio,&class_attr_input_enable);
	ret = class_create_file(remote_gpio,&class_attr_otg_noovo);
	ret = class_create_file(remote_gpio,&class_attr_led_noovo);
	ret = class_create_file(remote_gpio,&class_attr_net_lte);

	return 0;
}

static int remote_led_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver remote_led_driver = {
	.driver		= {
		.name		= "remote_led_driver",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(remote_led_of_match),
	},
	.probe		= remote_led_probe,
	.remove		= remote_led_remove,
};

module_platform_driver(remote_led_driver);
MODULE_DESCRIPTION("remote led power of Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:remote_led_power");
