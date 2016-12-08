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
#define ENDPOINT_IN				81

#define DRIVER_NAME				"XCon-Treiber"
#define DRIVER_DESCRIPTION		"Xbox Pad Treiber"
#define DRIVER_AUTHOR			"KP 16/17 - BTDDTC"
#define DRIVER_LICENSE			"GPL"

#define DRIVER_FILE_NAME		"xcon-%d"

#define MESSAGE_SIZE			64

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
	u8 data[MESSAGE_SIZE];
	u8 len;
	bool pending;
};

// STRUCT USB_XCON
struct usb_xcon {

	struct usb_device *udev;			/* usb device */
	struct usb_interface *intf;			/* usb interface */
	__u8 bulk_in_endpointAddr;			/* the address of the bulk in endpoint */
	__u8 bulk_out_endpointAddr;			/* the address of the bulk out endpoint */

	int hasEndpoint;				/*Has the device an endpoint? */
		
	unsigned char* bulk_in_buffer;		/* the buffer to receive data */

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
static ssize_t xcon_write(struct file* file, const char* user_buffer, size_t count, loff_t* ppos);
static int xcon_release(struct inode* inode, struct file* file);

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
	.write = xcon_write,
	.release = xcon_release,
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

	struct usb_host_interface *interface_descriptor;
	struct usb_endpoint_descriptor *endpoint;

	struct usb_xcon *dev;

	dev = kmalloc(sizeof(struct usb_xcon), GFP_KERNEL);

	dev->udev = interface_to_usbdev(interface);
	
	printk("XCon: -- PROBE - Enter\n");

	printk("XCon: 0x%4.4x|0x%4.4x, if=%p\n", (dev->udev)->descriptor.idVendor, (dev->udev)->descriptor.idProduct, interface);

	if((dev->udev)->descriptor.idVendor == USB_VENDOR_ID && (dev->udev)->descriptor.idProduct == USB_PRODUCT_ID){

		
		printk("XCon: -- PROBE - Correct Device\n");

		interface_descriptor = interface->cur_altsetting;

		
		printk("XCon: interface_descriptor endpoint: %p\n", interface_descriptor->endpoint);
		
		//Interface 4 hat bei mir warum auch immer keinen Endpoint..
		if(interface_descriptor->endpoint != NULL) { 
			dev->hasEndpoint = 1;
			endpoint = &interface_descriptor->endpoint[0].desc;

			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(MESSAGE_SIZE, GFP_KERNEL);

			printk("XCon: found endpoint: %p\n", interface_descriptor->endpoint);
			
		} else {
			printk("XCon: no endpoint: %p\n", interface_descriptor->endpoint);
			dev->hasEndpoint = 0;
		}

		/* save xcon struct in interface */
		usb_set_intfdata(interface, dev);

		char str[32];	
 		int ret;
 		ret = usb_make_path(dev->udev, str, 32 * sizeof(char));
 
		printk("XCon:    USB-PATH: %s\n", str);
		

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
static int xcon_open(struct inode *devicefile, struct file* file){
	
	printk("XCon: -- OPEN - Enter\n");

	struct usb_xcon *dev;
	struct usb_interface *interface;
	int subminor;
	
	subminor = iminor(devicefile);
	
	interface = usb_find_interface(&driver_desc, subminor);
	if (!interface) {
		printk("XCon: -- OPEN - Error - Kein interface gefunden");
		return -ENODEV;
	}

	dev = usb_get_intfdata(interface);

	file->private_data = dev;

	printk("XCon: -- OPEN - Exit\n");

	return 0;
}


// READ
static ssize_t xcon_read(struct file* file, char* buffer, size_t count, loff_t* ofs){
	
	printk("XCon: -- READ - Enter\n");
	
	dev = (struct usb_xcon *)file->private_data;

	printk("XCon: -- READ - device:%p\n", dev);

	if(!dev->hasEndpoint) {
		printk("XCon: -- READ - Exit - No Endpoint!\n");
		return 0;
	} else {
		printk("XCon: -- READ -has Endpoint!\n");
	}
		

	mutex_lock( &ulock ); // Jetzt nicht disconnecten ...

	int actual_length;

	int retval = usb_bulk_msg (dev->udev,
                       usb_rcvbulkpipe (dev->udev,
                       dev->bulk_in_endpointAddr),
                       dev->bulk_in_buffer,
                       min(MESSAGE_SIZE, (int) count),
                       &actual_length, 0);
	
	printk("XCon: -- READ - count: %d\n", (int)count);
	printk("XCon: -- READ - Length: %d\n", actual_length);

	/* if the read was successful, copy the data
	   to user space */
	if (!retval) {
		if (copy_to_user(buffer, dev->bulk_in_buffer, actual_length)) {
		        retval = -EFAULT;
			printk("XCon: -- READ - Exit  - Error copy-to-user\n");
		}
		else {
			printk("XCon: -- READ - Exit -- Success\n");
		        retval = count;
		}
	}

	
	mutex_unlock( &ulock );

	return retval;
}

static void xcon_write_bulk_callback(struct urb* urb){

	struct usb_xcon* dev;

	dev = (struct usb_xcon*)urb->context;
	
	if(urb->status &&
		!(urb->status == -ENOENT ||
		  urb->status == -ECONNRESET ||
		  urb->status == -ESHUTDOWN)){

		printk("%s - nonzero write bulk status received: %d", __FUNCTION__, urb->status);		

	}

	usb_free_coherent(urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);

}

// WRITE
static ssize_t xcon_write(struct file* file, const char* user_buffer, size_t count, loff_t* ppos){
	
	printk("XCon: -- WRITE - Enter\n");
	printk("   WRITE-MSG: %s\n", user_buffer);

	struct usb_xcon* dev;
	int retval = 0;
	struct urb* urb = NULL;
	char* buffer = NULL;

	dev = (struct usb_xcon*)file->private_data;
	if(count == 0){
		goto exit;
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!urb){
		retval = -ENOMEM;
		goto error;
	}

	buffer = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if(!buffer){
		retval = -ENOMEM;
		goto error;
	}

	if(copy_from_user(buffer, user_buffer, count)){
		retval = -EFAULT;
		goto error;
	}

	usb_fill_bulk_urb(urb, dev->udev, usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
						buffer, count, xcon_write_bulk_callback, dev);

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	retval = usb_submit_urb(urb, GFP_KERNEL);
	if(retval){
	
		printk("%s - failed submitting write urb, error %d\n", __FUNCTION__, retval);
		goto error;

	}
	
	usb_free_urb(urb);

	printk("XCon: -- WRITE - Exit\n");

exit:
	printk("XCon: -- WRITE - Exit - Exit\n");
	return count;

error:
	printk("XCon: -- WRITE - Exit - Error\n");
	return retval;
	
}


static int xcon_release(struct inode* inode, struct file* file){

	printk("XCon: -- RELEASE - Enter\n");	

	struct usb_xcon* dev;

	dev = (struct usb_xcon*)file->private_data;
	if(dev == NULL)
		return -ENODEV;


	printk("XCon: -- RELEASE - Exit\n");

	return 0;

}

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
