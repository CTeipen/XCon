#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define USB_VENDOR_ID		0x045e
#define USB_PRODUCT_ID		0x028e

#define DRIVER_NAME		"XCon-Treiber"
#define DRIVER_FILE_NAME	"xcon-%d"


static int xcon_init(void);
static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id);

static int xcon_open(struct inode *devicefile, struct file* instance);
static ssize_t xcon_read(struct file* instanz, char* buffer, size_t count, loff_t* ofs);
//static void xcon_write(void);

static void xcon_disconnect(struct usb_interface *interface);
static void xcon_exit(void);


struct usb_device* dev;
static DEFINE_MUTEX( ulock );

static struct usb_device_id dev_table [] = {
	{ USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
	{  }
};

static struct usb_driver driver_desc = {
	.name = DRIVER_NAME,
	.id_table = dev_table,
	.probe = xcon_probe,
	.disconnect = xcon_disconnect,
};

static struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = xcon_open,
	.read = xcon_read,
	//.write = xcon_write,
};


static struct usb_class_driver device_file = {
	.name = DRIVER_FILE_NAME,
	.fops = &usb_fops,
	.minor_base = 16,
};


static int __init xcon_init(void){
	
	printk("XCon: INIT - Enter\n");

	if(usb_register(&driver_desc)){
		printk("XCon: REG - Failed\n");
		printk("XCon: INIT - Exit\n");
			
		return -EIO;
	}

	printk("XCon: REG - Success\n");
	printk("XCon: INIT - Exit\n");
	
	return 0;
}

static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id){
	
	printk("XCon: PROBE - Enter\n");

	dev = interface_to_usbdev(interface);

	printk("XCon: 0x%4.4x|0x%4.4x, if=%p\n", dev->descriptor.idVendor, dev->descriptor.idProduct, interface);

	if(dev->descriptor.idVendor == USB_VENDOR_ID && dev->descriptor.idProduct == USB_PRODUCT_ID){
		

		if(usb_register_dev(interface, &device_file)){
			printk("XCon: REGDEV - Failed\n");
			printk("XCon: PROBE - Exit\n");
			return -EIO;
		}
	
		printk("XCon: MINOR = %d\n", interface->minor);

		printk("XCon: REGDEV - Success\n");
		printk("XCon: PROBE - Exit\n");
	
		return 0;

	}

	return -ENODEV;
}


static int xcon_open(struct inode *devicefile, struct file* instance){
	
	printk("XCon: OPEN - Enter\n");

	printk("XCon: OPEN - Exit\n");

	return 0;
}

static ssize_t xcon_read(struct file* instanz, char* buffer, size_t count, loff_t* ofs){
	
	printk("XCon: READ - Enter\n");

	char pbuf[20];
	__u16 *status = kmalloc( sizeof(__u16), GFP_KERNEL );

	mutex_lock( &ulock ); // Jetzt nicht disconnecten ...

	if( usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_GET_STATUS, USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE, 0, 0, status, sizeof(*status), 5*HZ) < 0){

		count = -EIO;
		goto read_out;

	}

	sprintf( pbuf, sizeof(pbuf), "status=%d\n", *status );

	if(strlen(pbuf) < count)
		count = strlen(pbuf);

	count -= copy_to_user(buffer, pbuf, count);
	*ofs += count;

read_out:
	mutex_unlock( &ulock );
	kfree( status );

	printk("XCon: READ - Exit\n");

	return count;
}

/*static void xcon_write(void){
	
	printk("XCon: WRITE - Enter\n");

	printk("XCon: WRITE - Exit\n");
}*/


static void xcon_disconnect(struct usb_interface *interface){
	
	printk("XCon: DISCONNECT - Enter\n");

	
	mutex_lock( &ulock );
	
	usb_deregister_dev(interface, &device_file);

	mutex_unlock( &ulock );


	printk("XCon: DISCONNECT - Exit\n");
}

static void __exit xcon_exit(void){

	printk("XCon: EXIT - Enter\n");

	usb_deregister(&driver_desc);
	printk("XCon: DEREG\n");

	printk("XCon: EXIT - Exit\n");
}

MODULE_DEVICE_TABLE(usb, dev_table);
module_init(xcon_init);
module_exit(xcon_exit);
//-------------------------------------------
// Modul-Meta-Daten
// ------------------------------------------
MODULE_AUTHOR("KP 2016/17 - BTDDTC");
MODULE_DESCRIPTION("XCon - Treiber");
MODULE_LICENSE("GPL");
