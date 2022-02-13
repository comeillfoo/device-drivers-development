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

#define COMP_NAME "var2"
#define DEV_NAME  "/dev/"  COMP_NAME
#define PROC_NAME "/proc/" COMP_NAME

static dev_t comp_first_dev_id;
static u32   comp_dev_count = 1;
static int   comp_major = 410, comp_minor = 0;
static struct cdev* comp_cdev = NULL;

static bool init_chr_dev( dev_t* first_dev_id, int major, int minor, u32 count, struct cdev* cdev, const char* name, const struct file_operations* fops );
static bool cleanup_chr_dev( struct cdev* cdev, dev_t* first_dev_id, u32 count );

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

static const struct file_operations comp_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_var2_open,
  .read    = proc_var2_read,
  .write   = proc_var2_write,
  .release = proc_var2_release
};


static int __init init_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module inited" );

  proc_var2_entry = proc_create( COMP_NAME, 0444, NULL, &proc_var2_ops ); // 0444 -> r--r--r--

  init_chr_dev( &comp_first_dev_id, comp_major, comp_minor, comp_dev_count, comp_cdev, COMP_NAME, &comp_fops );

  return 0;
}

static void __exit cleanup_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module cleaned up" );

  proc_remove( proc_var2_entry );

  cleanup_chr_dev( comp_cdev, &comp_first_dev_id, comp_dev_count );
}

module_init( init_chr_comp );
module_exit( cleanup_chr_comp );


static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file " PROC_NAME " opened\n" );
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
  printk( KERN_INFO MOD_NAME ": file " PROC_NAME " closed\n" );
  return 0;
}

static bool init_chr_dev( dev_t* first_dev_id, int major, int minor, u32 count, struct cdev* cdev, const char* name, const struct file_operations* fops ) {
  *first_dev_id = MKDEV( major, minor );
  register_chrdev_region( *first_dev_id, count, name );

  cdev = cdev_alloc();
  
  cdev_init( cdev, fops );
  cdev_add( cdev, *first_dev_id, count );

  return true;
}

static bool cleanup_chr_dev( struct cdev* cdev, dev_t* first_dev_id, u32 count ) {
  if ( cdev )
    cdev_del( cdev );

  unregister_chrdev_region( *first_dev_id, count );
  return true;
}