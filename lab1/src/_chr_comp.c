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

#define OUTCOMES_LENGTH (5)

static int outcomes[ OUTCOMES_LENGTH ];
static size_t dev_idx = 0;
static size_t proc_idx = 0;

static bool dev_create( dev_t* first_dev_id, int major, int minor, u32 count, struct cdev* cdev, const char* name, const struct file_operations* fops );
static bool dev_remove( struct cdev* cdev, dev_t* first_dev_id, u32 count );
static int dev_var2_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t dev_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t dev_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int dev_var2_release( struct inode* ptr_inode, struct file* ptr_file );


static dev_t dev_var2_first_device_id;
static u32   dev_var2_count = 1;
static int   dev_var2_major = 410, dev_var2_minor = 0;
static struct cdev* dev_var2_cdev = NULL;
static const struct file_operations dev_var2_fops = {
  .owner   = THIS_MODULE,
  .open    = dev_var2_open,
  .read    = dev_var2_read,
  .write   = dev_var2_write,
  .release = dev_var2_release
};


static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t proc_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t proc_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int proc_var2_release( struct inode* ptr_inode, struct file* ptr_file );

static struct proc_dir_entry* proc_var2_entry = NULL;
static const struct proc_ops proc_var2_ops = {
  .proc_open    = proc_var2_open,
  .proc_read    = proc_var2_read,
  .proc_write   = proc_var2_write,
  .proc_release = proc_var2_release
};


static int __init init_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module inited" );

  proc_var2_entry = proc_create( COMP_NAME, 0444, NULL, &proc_var2_ops ); // 0444 -> r--r--r--
  if ( proc_var2_entry == NULL )
    return -EINVAL;

  if ( dev_create( &dev_var2_first_device_id, dev_var2_major, dev_var2_minor, dev_var2_count, dev_var2_cdev, COMP_NAME, &dev_var2_fops ) == false ) 
    return -EINVAL;

  return 0;
}

static void __exit cleanup_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module cleaned up" );

  proc_remove( proc_var2_entry );

  dev_remove( dev_var2_cdev, &dev_var2_first_device_id, dev_var2_count );
}

module_init( init_chr_comp );
module_exit( cleanup_chr_comp );


static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file " PROC_NAME " opened\n" );
  return 0;
}

static ssize_t proc_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  if ( *ptr_offset > 0 ) return 0;
  *ptr_offset += length;

  printk( KERN_INFO MOD_NAME ": proc_var2_read: %d\n", outcomes[ proc_idx ] );
  proc_idx = ( proc_idx + 1 ) % OUTCOMES_LENGTH;
  return length;
}

static ssize_t proc_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  return -EINVAL;
}

static int proc_var2_release( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file " PROC_NAME " closed\n" );
  return 0;
}


static bool dev_create( dev_t* first_dev_id, int major, int minor, u32 count, struct cdev* cdev, const char* name, const struct file_operations* fops ) {
  *first_dev_id = MKDEV( major, minor );
  if ( register_chrdev_region( *first_dev_id, count, name ) ) {
    // unregister_chrdev_region( *first_dev_id, count ); // void can't check for the errors
    return false;
  }

  cdev = cdev_alloc();
  if ( cdev == NULL ) {
    unregister_chrdev_region( *first_dev_id, count ); // void can't check for the errors
    return false;
  }
  
  cdev_init( cdev, fops );
  if ( cdev_add( cdev, *first_dev_id, count ) == -1 ) {
    unregister_chrdev_region( *first_dev_id, count ); // void can't check for the errors
    cdev_del( cdev );
    return false;
  }
  
  return true;
}

static bool dev_remove( struct cdev* cdev, dev_t* first_dev_id, u32 count ) {
  if ( cdev )
    cdev_del( cdev ); // void can't check for errors

  unregister_chrdev_region( *first_dev_id, count ); // void can't check for the errors
  return true;
}

static int dev_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file " DEV_NAME " opened\n" );
  return 0;
}

static ssize_t dev_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  // size_t count = 0;
  // if ( *ptr_offset > 0 ) return 0;

  // count = snprintf( usr_buf, length, "%d\n", outcomes[ dev_idx ] );
  // *ptr_offset += count;
  // dev_idx = ( dev_idx + 1 ) % OUTCOMES_LENGTH;
  // printk( KERN_INFO MOD_NAME ": dev_var2_read: dev_idx = %zu, count = %zu, length = %zu\n", dev_idx, count, length );
  // return count;
  if ( *ptr_offset > 0 ) return 0;
  *ptr_offset += length;

  printk( KERN_INFO MOD_NAME ": dev_var2_read: %d\n", outcomes[ ( dev_idx - 1 ) % OUTCOMES_LENGTH ] );
  dev_idx = ( dev_idx + 1 ) % OUTCOMES_LENGTH;
  return length;
}

#define defop( name, op ) static int name( int a, int b ) { return a op b; }

defop( sum, + )
defop( sub, - )
defop( mul, * )
defop( div, / )

typedef int (op)(int, int);
op* callbacks[] = {
  [ '+' ] = sum,
  [ '-' ] = sub,
  [ '/' ] = div,
  [ '*' ] = mul
};


static ssize_t dev_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  int a = 0, b = 0;
  char c = '+';
  size_t count = sscanf( usr_buf, "%d%c%d", &a, &c, &b );

  printk( KERN_INFO MOD_NAME ": dev_var2_write: count = %zu, a = %d, b = %d, c = %c, length = %zu\n", count, a, b, c, length );
  
  if ( b == 0 && c == '/' )
    return -EINVAL;

  if ( count == 3 ) {
    outcomes[ dev_idx ] = callbacks[ ( size_t ) c ]( a, b );
    dev_idx = ( dev_idx + 1 ) % OUTCOMES_LENGTH;
    return length;
  }
  return -EINVAL;
}

static int dev_var2_release( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file " DEV_NAME " closed\n" );
  return 0;
}