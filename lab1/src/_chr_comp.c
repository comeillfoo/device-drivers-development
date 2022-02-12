#include <linux/module.h> /* essential for modules */
#include <linux/kernel.h> /* essential for KERNEL_INFO */

#include <linux/proc_fs.h> /* essential for procfs */
#include <linux/slab.h> /* essential for kmalloc, kfree */


MODULE_LICENSE( "Dual MIT/GPL" );
MODULE_AUTHOR( "Lenar" );
MODULE_AUTHOR( "Michael" );
MODULE_DESCRIPTION( "Kernel module consisting of /dev/var2 which do some simple calculations and store interim results and /proc/var2 which only stores interim results from /dev/var2" );


#define MOD_NAME "chr_comp"

static int __init init_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module inited" );
  return 0;
}

static void __exit cleanup_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module cleaned up" );
}

module_init( init_chr_comp );
module_exit( cleanup_chr_comp );