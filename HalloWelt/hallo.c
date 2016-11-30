#include <linux/kernel.h>
#include <linux/module.h>

static int hello_init(void){
	printk(KERN_ALERT "TEST: Hello World");
	display();
	return 0;
}

static void hello_exit(void){
	printk(KERN_ALERT "TEST: Bye, bye!");
}

module_init(hello_init);
module_exit(hello_exit);

//-------------------------------------------
// Modul-Meta-Daten
// ------------------------------------------
MODULE_AUTHOR("KP 2016/17");
MODULE_DESCRIPTION("Hello World - Treiber");
MODULE_LICENSE("GPL");
