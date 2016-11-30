#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/usb/input.h>

// Treiber Daten
#define USB_VENDOR_ID			0x045e
#define USB_PRODUCT_ID			0x028e

#define DRIVER_NAME				"XCon-Treiber"
#define DRIVER_DESCRIPTION		"Xbox Pad Treiber"
#define DRIVER_AUTHOR			"KP 16/17 - BTDDTC"
#define DRIVER_LICENSE			"GPL"

#define DRIVER_FILE_NAME		"xcon-%d"

#define XCON_PKT_LEN			64

#define XCON_OUT_CMD_IDX		0
#define XCON_OUT_FF_IDX			1
#define XCON_OUT_LED_IDX		(1 + IS_ENABLED(CONFIG_JOYSTICK_XPAD_FF))
#define XCON_NUM_OUT_PACKETS	(1 + IS_ENABLED(CONFIG_JOYSTICK_XCON_FF) + IS_ENABLED(CONFIG_JOYSTICK_XCON_LEDS))


// ------------------------------------------------------
// Module Parameter
// ------------------------------------------------------
int param_var[3] = {0,0,0};
module_param_array(param_var, int, NULL, S_IRUSR | S_IWUSR);

// ------------------------------------------------------
// Grundlegende Structs
// ------------------------------------------------------

// STRUCT XCON_OUTPUT_PACKET
struct xcon_output_packet {
	u8 data[XCON_PKT_LEN];
	u8 len;
	bool pending;
};

// STRUCT USB_XCON
struct usb_xcon {

	struct usb_device *udev;			/* usb device */
	struct usb_interface *intf;			/* usb interface */

	bool pad_present;
	char phys[64];			/* physical device path */
	int pad_nr;				/* the order x360 pads were attached */
	const char *name;		/* name of the device */

};

// STRUCT USB_DEVICE
struct usb_xcon* dev;

// STRUCT USB_DEVICE_ID - Geraete
static struct usb_device_id dev_table [] = {
	{ USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
	{  }
};

// ------------------------------------------------------
// Prototypen
// ------------------------------------------------------
void display(void);


static int xcon_init(void);
static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id);

static int xcon_open(struct inode *devicefile, struct file* instance);
static ssize_t xcon_read(struct file* instanz, char* buffer, size_t count, loff_t* ofs);
//static void xcon_write(void);

static void xcon_disconnect(struct usb_interface *interface);
static void xcon_exit(void);

static DEFINE_MUTEX( ulock );


// ------------------------------------------------------
// Methoden abhängige Structs
// ------------------------------------------------------

// STRUCT USB_DRIVER
static struct usb_driver driver_desc = {
	.name = DRIVER_NAME,
	.probe = xcon_probe,
	.disconnect = xcon_disconnect,
	.id_table = dev_table,
};

// STRUCT FILE_OPERATIONS
static struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = xcon_open,
	.read = xcon_read,
	//.write = xcon_write,
};

// STRUCT USB_CLASS_DRIVER
static struct usb_class_driver device_file = {
	.name = DRIVER_FILE_NAME,
	.fops = &usb_fops,
	.minor_base = 16,
};


// ------------------------------------------------------
// 	METHODEN
// ------------------------------------------------------

void display(void){

	printk("XCon:    PARAM_VAR 1: %d\n", param_var[0]);
	printk("XCon:    PARAM_VAR 2: %d\n", param_var[1]);
	printk("XCon:    PARAM_VAR 3: %d\n", param_var[2]);

}

// INIT
static int __init xcon_init(void){
	
	
	printk("XCon: -- INIT - Enter\n");
	display();
	
	if(usb_register(&driver_desc)){
		printk("XCon:    REG - Failed\n");
		printk("XCon: -- INIT - Exit\n");
			
		return -EIO;
	}

	printk("XCon:    REG - Success\n");
	printk("XCon: -- INIT - Exit\n");
	
	return 0;
}


// PROBE
static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id){
	
	printk("XCon: -- PROBE - Enter\n");

	dev->udev = interface_to_usbdev(interface);

	printk("XCon: 0x%4.4x|0x%4.4x, if=%p\n", (dev->udev)->descriptor.idVendor, (dev->udev)->descriptor.idProduct, interface);

	if((dev->udev)->descriptor.idVendor == USB_VENDOR_ID && (dev->udev)->descriptor.idProduct == USB_PRODUCT_ID){
		

		if(usb_register_dev(interface, &device_file)){
			printk("XCon:    REGDEV - Failed\n");
			printk("XCon: -- PROBE - Exit\n");
			return -EIO;
		}
	
		printk("XCon:    MINOR = %d\n", interface->minor);

		printk("XCon:    REGDEV - Success\n");
		printk("XCon: -- PROBE - Exit\n");
	
		return 0;

	}

	return -ENODEV;
}


// OPEN
static int xcon_open(struct inode *devicefile, struct file* instance){
	
	printk("XCon: -- OPEN - Enter\n");

	printk("XCon: -- OPEN - Exit\n");

	return 0;
}


// READ
static ssize_t xcon_read(struct file* instanz, char* buffer, size_t count, loff_t* ofs){
	
	printk("XCon: -- READ - Enter\n");

	char pbuf[20];
	__u16 *status = kmalloc( sizeof(__u16), GFP_KERNEL );

	mutex_lock( &ulock ); // Jetzt nicht disconnecten ...

	if( usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), USB_REQ_GET_STATUS, USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE, 0, 0, status, sizeof(*status), 5*HZ) < 0){

		count = -EIO;
		goto read_out;

	}

	snprintf( pbuf, sizeof(pbuf), "status=%d\n", *status );

	if(strlen(pbuf) < count)
		count = strlen(pbuf);

	count -= copy_to_user(buffer, pbuf, count);
	*ofs += count;

read_out:
	mutex_unlock( &ulock );
	kfree( status );

	printk("XCon: -- READ - Exit\n");

	return count;
}


// WRITE
/*static void xcon_write(void){
	
	printk("XCon: -- WRITE - Enter\n");

	printk("XCon: -- WRITE - Exit\n");
}*/


// DISCONNECT
static void xcon_disconnect(struct usb_interface *interface){
	
	printk("XCon: -- DISCONNECT - Enter\n");

	
	mutex_lock( &ulock );
	
	usb_deregister_dev(interface, &device_file);

	mutex_unlock( &ulock );


	printk("XCon: -- DISCONNECT - Exit\n");
}


// EXIT
static void __exit xcon_exit(void){

	printk("XCon: -- EXIT - Enter\n");

	usb_deregister(&driver_desc);
	printk("XCon:    DEREG\n");

	printk("XCon: -- EXIT - Exit\n");
}


// ------------------------------------------
// Modul Initialisierungen
// ------------------------------------------
MODULE_DEVICE_TABLE(usb, dev_table);
module_init(xcon_init);
module_exit(xcon_exit);

//-------------------------------------------
// Modul-Meta-Daten
// ------------------------------------------
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_LICENSE(DRIVER_LICENSE);
