#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x67fc6d5b, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xfe5ef8fc, __VMLINUX_SYMBOL_STR(usb_deregister) },
	{ 0x8eec4d01, __VMLINUX_SYMBOL_STR(usb_register_driver) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xc671e369, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x88db9f48, __VMLINUX_SYMBOL_STR(__check_object_size) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x1ca73692, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0xd7544073, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xac66cb06, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x246827f5, __VMLINUX_SYMBOL_STR(usb_register_dev) },
	{ 0xc1df961c, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xcbc6530b, __VMLINUX_SYMBOL_STR(usb_deregister_dev) },
	{ 0x789e2616, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=usbcore";

MODULE_ALIAS("usb:v045Ep028Ed*dc*dsc*dp*ic*isc*ip*in*");
