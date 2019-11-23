#define pr_fmt(fmt)	KBUILD_MODNAME ":%s " fmt, __func__	/* override print format */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <soc/bcm2835/raspberrypi-firmware.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#define HDMI_GPIO_MAGIC 46

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A virtual echoing device");

static int state = 1;

static struct rpi_firmware * rpi_fw = NULL;
static struct timer_list tlist;

struct workqueue_struct *wq;
struct work_struct *ws;

typedef unsigned int uint32;
typedef unsigned char uint8;

static void hdmi_release(struct device *dev);
							       
char *envp[] = {"SUBSYSTEM=hdmi", NULL};

struct edid_response {
	uint32 block_number;
	uint32 status;
	uint8 block[128];
} er;

struct hdmi_dev_data {
	int hdmi_status;
};

#define to_hdmi_dev_data(d) ((struct hdmi_dev *)(to_platform_device(d)))

static ssize_t state_show (struct device * dev, struct device_attribute * attr, char * resp) {

	return snprintf(resp, 8, "%d\n", state);
}

static DEVICE_ATTR_RO (state);

static struct attribute *hdmi_attrs[] = {
	&dev_attr_state.attr,
	NULL
};

static struct attribute_group hdmi_group = {
	.attrs = hdmi_attrs,
};

static const struct attribute_group *hdmi_all_attr_groups[] = {
	&hdmi_group,
	NULL
};

static void hdmi_release(struct device *dev) {
	struct platform_device *pdev = to_platform_device(dev);

	dev_alert(dev, "releasing hdmi device %s\n", pdev->name);
}

static int hdmi_register_device(struct platform_device *pdev) {

	// struct digi_dev_data *digi = (typeof(digi))pdev->dev.platform_data;
	int err;

	err = platform_device_register(pdev);
	if (err)
		pr_err("digiout: device %s failed to register, error %d\n", pdev->name, err);
	return err;
}

static struct platform_device hrpi = {
	.name = "hdmi",
	.id = 1,
	.dev = {
		.release = hdmi_release,
		.groups = hdmi_all_attr_groups,
	},
};

static void hdmi_unregister_device(struct platform_device *pdev)
{
	dev_alert(&pdev->dev, "digiout: unregistering gpio device %s\n", pdev->name);
	platform_device_unregister(pdev);
}


void raspi_callback(struct work_struct *a) {

	int gpio_val = gpio_get_value(HDMI_GPIO_MAGIC);

	if(gpio_val == 0) { //HDMI plugged
		if(state == 0)
			state = 1;
	} else { // HDMI Unplugged
		if(state != 0) {
			kobject_uevent_env(&(hrpi.dev.kobj), KOBJ_CHANGE, envp);
			printk ("HDMI Unplugged\n");
			state = 0;
		}
	}
}

void timer_callback (struct timer_list * tl) {
	printk ("In %s\n", __func__);

	//Setting up work_struct structure at runtime
	INIT_WORK(ws, raspi_callback);
	queue_work(wq, ws);
    
	
	tl->expires = (unsigned long) (jiffies + 1*HZ);
	add_timer (tl);
}

static void timer_init_func (void) {
	timer_setup (&tlist, timer_callback, 0);
	tlist.function = timer_callback;
	tlist.expires = (unsigned long) (jiffies + 1*HZ);
	add_timer (&tlist);
}

static int __init hdmi_rpi_init(void)
{
	int ret = 0;
	pr_info ("loading ... \n");
	rpi_fw = rpi_firmware_get (NULL);


	// Check whether the given GPIO is valid
	if(!gpio_is_valid(HDMI_GPIO_MAGIC)) {
		printk("GPIO %s is not valid\n", HDMI_GPIO_MAGIC);
		return -1;
	}
	
	//Things related to workqueue:

	//Need to allocate here, not in timer because, this may not
	//be atomic. Refer LDD
	ws = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
	wq = create_workqueue("rpi_wq");
  
	timer_init_func ();	
	return hdmi_register_device(&hrpi);
}

static void timer_exit_func (void) {
	pr_info ("timer unloading ... \n");
	del_timer_sync (&tlist);
}

static void __exit hdmi_rpi_exit(void)
{
	kfree(ws);
	timer_exit_func ();  
	flush_workqueue(wq);
	destroy_workqueue(wq);

	pr_info("unloading ... \n");
	hdmi_unregister_device(&hrpi);
}

module_init(hdmi_rpi_init);
module_exit(hdmi_rpi_exit);

