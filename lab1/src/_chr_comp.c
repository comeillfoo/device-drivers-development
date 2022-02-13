#include <linux/module.h>  /* essential for modules' macros */
#include <linux/kernel.h>  /* essential for KERNEL_INFO */

#include <linux/proc_fs.h> /* essential for procfs */
#include <linux/slab.h>    /* essential for kmalloc, kfree */
#include <linux/cdev.h>    /* essential for dev_t */


MODULE_LICENSE( "Dual MIT/GPL" );
MODULE_AUTHOR( "Lenar" );
MODULE_AUTHOR( "Michael" );
MODULE_DESCRIPTION( "Kernel module consisting of /dev/var2 which do some simple calculations and store interim results and /proc/var2 which only stores interim results from /dev/var2" );

#define MOD_NAME "chr_comp"

#define COMM_NAME "var2"
#define DEV_NAME  COMM_NAME
#define PROC_NAME COMM_NAME

static dev_t first_dev_id;

static struct proc_dir_entry* proc_var2_entry = NULL;

static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t proc_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t proc_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int proc_var2_release( struct inode* ptr_inode, struct file* ptr_file );

static const struct proc_ops proc_var2_ops = {
  .proc_open    = proc_var2_open,
  .proc_read    = proc_var2_read,
  .proc_write   = proc_var2_write,
  .proc_release = proc_var2_release
};


static int __init init_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module inited" );

  proc_var2_entry = proc_create( PROC_NAME, 0444, NULL, &proc_var2_ops ); // 0444 -> r--r--r--

  return 0;
}

static void __exit cleanup_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module cleaned up" );

  proc_remove( proc_var2_entry );
}

module_init( init_chr_comp );
module_exit( cleanup_chr_comp );


static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  return 0;
}

static ssize_t proc_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  *ptr_offset += length;
  return length;
}

static ssize_t proc_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  *ptr_offset += length;
  return length;
}

static int proc_var2_release( struct inode* ptr_inode, struct file* ptr_file ) {
  return 0;
}