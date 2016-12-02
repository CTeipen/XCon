#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/rcupdate.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/usb/input.h>
#include <linux/usb/quirks.h>

// Treiber Daten
#define USB_VENDOR_ID			0x045e
#define USB_PRODUCT_ID			0x028e

#define DEV_NAME			"Microsoft X-Box 360 pad"

#define DRIVER_NAME			"XCon-Treiber"
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
//int param_var[3] = {0,0,0};
//module_param_array(param_var, int, NULL, S_IRUSR | S_IWUSR);

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

	struct input_dev* idev;
	
	struct usb_device *udev;			/* usb device */
	struct usb_interface *intf;			/* usb interface */

	struct usb_interface* interface;

	unsigned char* iBuffer;				/* Input Buffer */
	//unsigned char* oBuffer;				/* Output Buffer */
	dma_addr_t iBuffer_dma;

	struct urb* irq_in;

	//bool con_present;

	struct work_struct work;

	//bool pad_present;
	char phys[64];			/* physical device path */
	//int pad_nr;			/* the order x360 pads were attached */
	const char *name;		/* name of the device */

};

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

static void xcon_presence_work(struct work_struct *work);


static int xcon_init_input(struct usb_xcon *xcon);
static void xcon_deinit_input(struct usb_xcon *xcon);

static int xcon_init_output(struct usb_interface* interface, struct usb_xcon *xcon);


static void xcon_irq_in(struct urb *urb);

static void xcon_process_packet(struct usb_xcon* xcon, u16 cmd, unsigned char* data);

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
	
	
	printk("XCon: -- INIT - Enter\n");
	
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

	struct usb_device* dev;
	struct usb_xcon* xcon;
	struct usb_endpoint_descriptor* ep_desc_in;
	int error;

	dev = interface_to_usbdev(interface);



	if(interface->cur_altsetting->desc.bNumEndpoints != 2){
		return -ENODEV;
	}

	xcon = (struct usb_xcon*)kzalloc(sizeof(struct usb_xcon), GFP_KERNEL);
	if(xcon == NULL){
		return -ENOMEM;
	}
	
	xcon->iBuffer = usb_alloc_coherent(dev, XCON_PKT_LEN, GFP_KERNEL, &(xcon->iBuffer_dma));

	if(xcon->iBuffer == NULL){
		error = -ENOMEM;
		goto ERROR_FREE_MEM;
	}

	xcon->irq_in = usb_alloc_urb(0, GFP_KERNEL);
	if(xcon->irq_in == NULL){
		error = -ENOMEM;
		goto ERROR_FREE_IBUFFER;
	}

	xcon->udev = dev;
	xcon->interface = interface;
	xcon->name = DEV_NAME;

	usb_make_path(xcon->udev, xcon->phys, 64 * sizeof(char));
	strlcat(xcon->phys, "/input0", sizeof(xcon->phys));
	printk("XCon:    USB-PATH: %s\n", xcon->phys);


	INIT_WORK(&xcon->work, xcon_presence_work);

	error = xcon_init_output(interface, xcon);
	if(error){
		goto ERROR_FREE_IN_URB;	
	}

	ep_desc_in = &interface->cur_altsetting->endpoint[0].desc;

	usb_fill_int_urb(xcon->irq_in, dev,
			usb_rcvintpipe(dev, ep_desc_in->bEndpointAddress), xcon->iBuffer, XCON_PKT_LEN, xcon_irq_in, xcon, ep_desc_in->bInterval);


	xcon->irq_in->transfer_dma = xcon->iBuffer_dma;
	xcon->irq_in->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usb_set_intfdata(interface, xcon);

	printk("XCon:    0x%4.4x|0x%4.4x, if=%p\n", (xcon->udev)->descriptor.idVendor, (xcon->udev)->descriptor.idProduct, interface);	

	error = xcon_init_input(xcon);
	if(error){
		goto ERROR_DEINIT_INPUT;
	}


	printk("XCon: -- PROBE - Exit\n");
	return 0;

ERROR_DEINIT_INPUT:
	xcon_deinit_input(xcon);	


ERROR_FREE_IN_URB:
	printk("XCon: -- PROBE - Exit with Error: FREE_IN_URB\n");	
	usb_free_urb(xcon->irq_in);

ERROR_FREE_IBUFFER:
	printk("XCon: -- PROBE - Exit with Error: FREE_IBUFFER\n");	
	usb_free_coherent(dev, XCON_PKT_LEN, xcon->iBuffer, xcon->iBuffer_dma);

ERROR_FREE_MEM:
	kfree(xcon);
	printk("XCon: -- PROBE - Exit with Error: FREE_MEM\n");
	return error;
}

static void xcon_presence_work(struct work_struct *work){

	printk("XCon:    PRESENCE WORK - Enter\n");

	/*struct usb_xcon *xcon = container_of(work, struct usb_xcon, work);
	int error;

	if (xcon->con_present) {
		error = xcon_init_input(xcon);
		if (error) {
			/complain only, not much else we can do here/
			dev_err(&xcon->idev->dev,
				"unable to init device: %d\n", error);
		}
	}*/

	printk("XCon:    PRESENCE WORK - Exit\n");
}

static int xcon_init_input(struct usb_xcon *xcon){

	printk("XCon:    INIT INPUT - Enter\n");


	printk("XCon:    INIT INPUT - Exit\n");

	return 0;
}

static void xcon_deinit_input(struct usb_xcon *xcon){

	printk("XCon:    DEINIT INPUT - Enter\n");


	printk("XCon:    DEINIT INPUT - Exit\n");

}

static int xcon_init_output(struct usb_interface* interface, struct usb_xcon *xcon){

	printk("XCon:    INIT OUTPUT - Enter\n");


	printk("XCon:    INIT OUTPUT - Exit\n");

	return 0;
}

static void xcon_irq_in(struct urb *urb)
{
	/*struct usb_xcon *xcon = urb->context;
	struct device *dev = &xcon->interface->dev;
	int retval, status;

	status = urb->status;

	switch (status) {
	case 0:
		// success
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		// this urb is terminated, clean up
		dev_dbg(dev, "%s - urb shutting down with status: %d\n",
			__func__, status);
		return;
	default:
		dev_dbg(dev, "%s - nonzero urb status received: %d\n",
			__func__, status);
		goto exit;
	}

	xcon_process_packet(xcon, 0, xcon->iBuffer);


exit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		dev_err(dev, "%s - usb_submit_urb failed with result %d\n",
			__func__, retval);
	*/
}

static void xcon_process_packet(struct usb_xcon* xcon, u16 cmd, unsigned char* data){
	printk("XCon:    Process-Packet: %s", data);
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
