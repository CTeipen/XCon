#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/usb/input.h>
#include <linux/types.h>

// Treiber Daten
#define USB_VENDOR_ID			0x045e
#define USB_PRODUCT_ID			0x028e

#define DRIVER_NAME			"XCon-Treiber"
#define DRIVER_DESCRIPTION		"Xbox Pad Treiber"
#define DRIVER_AUTHOR			"KP 16/17 - BTDDTC"
#define DRIVER_LICENSE			"GPL"

#define DRIVER_FILE_NAME		"xcon-%d"

#define MESSAGE_SIZE			64


// ------------------------------------------------------
// Module Parameter
// ------------------------------------------------------
int param_var[3] = {0,0,0};
module_param_array(param_var, int, NULL, S_IRUSR | S_IWUSR);

// ------------------------------------------------------
// Grundlegende Structs
// ------------------------------------------------------

// STRUCT USB_XCON
struct usb_xcon {

	struct usb_device *udev;			/* usb Device */
	struct usb_interface *intf;			/* usb interface */
	__u8 int_in_endpointAddr;			/* Addresse vom Input Endpoint */
	__u8 int_out_endpointAddr;			/* Addresse vom Output Endpoint */
	__u8 bInterval_output;

	int hasEndpoint;				/* Hat das Gerät einen Endpoint? */
	int isFirstDevice;				/* Ist es das erste Gerät des Controllers ?*/
	int number;					/* Die Nummer des Controllers */

	unsigned char* bulk_in_buffer;			/* Der Input Buffer */

	bool pad_present;				/* Datenfelder ohne die das Gerät nicht erkannt wird */
	char phys[64];					/* Physische addresse */
	const char *name;				/* Name des Geräts */

};

// STRUCT USB_DEVICE
struct usb_xcon* dev;

//Vergebene IDs
static int ids[4];

//Anzahl Geräte
static int counter;


// STRUCT USB_DEVICE_ID - Geraete
static struct usb_device_id dev_table [] = {
	{ USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
	{  }
};

// ------------------------------------------------------
// Prototypen
// ------------------------------------------------------

static int xcon_init(void);
static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id);

static int xcon_open(struct inode *devicefile, struct file* instance);
static ssize_t xcon_read(struct file* instanz, char* buffer, size_t count, loff_t* ofs);
static ssize_t xcon_write(struct file* file, const char* user_buffer, size_t count, loff_t* ppos);
static int xcon_release(struct inode* inode, struct file* file);

static void xcon_write_int_callback(struct urb *urb);


static int getNextNumber(void);
static void setNumber(struct usb_xcon * dev);

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

// INIT
static int __init xcon_init(void){
	
	
	counter = 0;
	ids[0] = 0;
	ids[1] = 0;
	ids[2] = 0;
	ids[3] = 0;

	if(usb_register(&driver_desc)){
		return -EIO;
	}

	return 0;
}

static int getNextNumber(void) {
	for(int i = 0; i < 4; i++) {
		if(ids[i] == 0) {
			ids[i] = 1;
			return i;
		}
	}

	return -1;
}


// PROBE
static int xcon_probe(struct usb_interface *interface, const struct usb_device_id *id){

	struct usb_host_interface *interface_descriptor;
	struct usb_endpoint_descriptor *endpoint;

	struct usb_xcon *dev;

	dev = kmalloc(sizeof(struct usb_xcon), GFP_KERNEL);

	dev->udev = interface_to_usbdev(interface);

	if((dev->udev)->descriptor.idVendor == USB_VENDOR_ID && (dev->udev)->descriptor.idProduct == USB_PRODUCT_ID){

		interface_descriptor = interface->cur_altsetting;
		
		//Suche nach Endpunkten, nehme die ersten verfuegbaren
		if(interface_descriptor->endpoint != NULL) { 
			dev->hasEndpoint = 1;

			for(int i = 0; i < interface_descriptor->desc.bNumEndpoints; i++) {
				endpoint = &interface_descriptor->endpoint[i].desc;

				//Ist Input Endpoint
				if(dev->int_in_endpointAddr == 0 && 
					(endpoint->bEndpointAddress & USB_DIR_IN)) {
			
					dev->int_in_endpointAddr = endpoint->bEndpointAddress;
					dev->bulk_in_buffer = kmalloc(MESSAGE_SIZE, GFP_KERNEL);
				}

				//Ist Output Endpoint
				if(dev->int_out_endpointAddr == 0 && 
					!(endpoint->bEndpointAddress & USB_DIR_IN)) {
				
					dev->bInterval_output = endpoint->bInterval;
					dev->int_out_endpointAddr = endpoint->bEndpointAddress;
				}

			}
			
		} else {
			dev->hasEndpoint = 0;
		}

		/* Speichern der Struct im Interface selbst */
		usb_set_intfdata(interface, dev);

		/* Auslesen des Pfades. Muss gemacht werden */
		char str[32];	
 		int ret;
 		ret = usb_make_path(dev->udev, str, 32 * sizeof(char));
 

		//Jedes 4te Device ist ein neuer Controller
		if(counter % 4 == 0) {
			dev->number = getNextNumber();
			dev->isFirstDevice = 1;
		}else {
			dev->isFirstDevice = 0;
		}

		if(usb_register_dev(interface, &device_file)){
			return -EIO;
		}

		if(dev->isFirstDevice == 1) {
			setNumber(dev);
		}

		counter++;
		return 0;
	}

	return -ENODEV;
}

static void setNumber(struct usb_xcon * dev) {

	mutex_lock( &ulock ); // Jetzt nicht disconnecten ...

	int number = 2 + dev->number;

	char *buf = NULL;
	struct urb *urb = NULL;
	
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (urb == NULL) {
		return;
	}

	//buf = usb_alloc_coherent(dev->udev, sizeof(char) * 3, GFP_KERNEL, &urb->transfer_dma);
	buf = kmalloc(sizeof(char) * 3, GFP_KERNEL);

	if (buf == NULL) {
		goto error_alloc;
	}

	buf[0] = 0x01;
	buf[1] = 0x03;
	buf[2] = number;

	usb_fill_int_urb(urb, 
		     dev->udev,
		     usb_sndintpipe(dev->udev, dev->int_out_endpointAddr),
		     buf, 
		     sizeof(char) * 3, 
		     xcon_write_int_callback, 
		     dev, dev->bInterval_output);
	//urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	int retval = usb_submit_urb(urb, GFP_KERNEL);

	if (retval) {
		goto error_send;
	}

error_alloc:
	usb_free_urb(urb);
	mutex_unlock( &ulock );
	
	return;
error_send:
	kfree(buf);
	goto error_alloc;

}


// OPEN
static int xcon_open(struct inode *devicefile, struct file* file){
	
	struct usb_xcon *dev;
	struct usb_interface *interface;
	int subminor;
	
	subminor = iminor(devicefile);
	
	interface = usb_find_interface(&driver_desc, subminor);
	if (!interface) {
		return -ENODEV;
	}

	dev = usb_get_intfdata(interface);

	if(!dev) {
		return -ENODEV;
	}

	file->private_data = dev;

	return 0;
}


// READ
static ssize_t xcon_read(struct file* file, char* buffer, size_t count, loff_t* ofs){
	
	dev = (struct usb_xcon *)file->private_data;

	if(!dev->hasEndpoint) {
		return 0;
	}
		

	mutex_lock( &ulock ); // Jetzt nicht disconnecten ...

	int actual_length;

	int retval = usb_bulk_msg (dev->udev,
                       usb_rcvbulkpipe (dev->udev,
                       dev->int_in_endpointAddr),
                       dev->bulk_in_buffer,
                       min(MESSAGE_SIZE, (int) count),
                       &actual_length, 0);
	
	/* Beim erfolgreichen lesen kann der Buffer zum Anwender kopiert werden*/
	if (!retval) {
		if (copy_to_user(buffer, dev->bulk_in_buffer, actual_length)) {
		        retval = -EFAULT;
		}
		else {
		        retval = count;
		}
	}
	
	mutex_unlock( &ulock );

	return retval;
}

static void xcon_write_int_callback(struct urb *urb) {

	struct usb_xcon *dev;

	dev = (struct usb_xcon*) urb->context;	

	if (urb->status && 
		!(urb->status == -ENOENT || 
		urb->status == -ECONNRESET ||
		urb->status == -ESHUTDOWN)) {
	}

	/* Freigeben der Buffer je nach Art */
	if(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) {
		usb_free_coherent(urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
	} else {
		kfree(urb->transfer_buffer);
	}

}

static ssize_t xcon_write(struct file *file, const char* user_buffer, size_t count, loff_t *ppos) {
	   
	struct usb_xcon *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;

	dev = (struct usb_xcon *)file->private_data;

	/* Sicherstellen, dass wir wirklich Daten bekommen haben */
   	if (count == 0) {
		goto exit;
	}
	
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (urb == NULL) {
		retval = -ENOMEM;
		return retval;
	}

	buf = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	
	if (buf == NULL) {
		retval = -ENOMEM;
		goto error_alloc_coherent;
	}

	if (copy_from_user(buf, user_buffer, count)) {
		retval = -EFAULT;
		goto error;
	}


	usb_fill_int_urb(urb, 
		     dev->udev,
		     usb_sndintpipe(dev->udev, dev->int_out_endpointAddr),
		     buf, 
		     count, 
		     xcon_write_int_callback, 
		     dev, dev->bInterval_output);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
   

	/* Senden der Daten */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval) {
		goto error;
	}

	usb_free_urb(urb);
exit:
	return count;
error:
	usb_free_coherent(dev->udev, count, buf, urb->transfer_dma);

error_alloc_coherent:
	usb_free_urb(urb);
	return retval;
}

static int xcon_release(struct inode* inode, struct file* file){

	struct usb_xcon* dev;

	dev = (struct usb_xcon*)file->private_data;
	if(dev == NULL)
		return -ENODEV;

	return 0;

}

// DISCONNECT
static void xcon_disconnect(struct usb_interface *interface){

	struct usb_xcon *dev;

	dev = kmalloc(sizeof(struct usb_xcon), GFP_KERNEL);

	dev = usb_get_intfdata(interface);

	if(dev->isFirstDevice == 1) {
		ids[dev->number] = 0;
	}
	
	mutex_lock( &ulock );
	
	usb_deregister_dev(interface, &device_file);

	mutex_unlock( &ulock );

	counter--;
}


// EXIT
static void __exit xcon_exit(void){
	usb_deregister(&driver_desc);
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
