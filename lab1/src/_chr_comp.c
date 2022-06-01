#include <linux/module.h>  /* essential for modules' macros */
#include <linux/kernel.h>  /* essential for KERNEL_INFO */

#include <linux/proc_fs.h> /* essential for procfs */
#include <linux/slab.h>    /* essential for kmalloc, kfree */
#include <linux/cdev.h>    /* essential for dev_t */


MODULE_LICENSE( "Dual MIT/GPL" );
MODULE_AUTHOR( "Lenar" );
MODULE_AUTHOR( "Michael" );
MODULE_DESCRIPTION( "Kernel module consisting of /dev/var2 which do some simple calculations and store interim results and /proc/var2 which only stores interim results from /dev/var2" );

// number of character devices
static int nr_devices = 1;
module_param(nr_devices, int, 0660);

// name of module
#define MOD_NAME "chr_comp"
// name of device class
#define CLASS_NAME "chr_comp_class"

// just base name of devices
#define DEV_NAME "var2"

#define OUTCOMES_LENGTH (5)

// device pool
static struct class* device_class = NULL;
static struct cdev*  var2_devices  = NULL;

// common device data
static int    outcomes[ OUTCOMES_LENGTH ];
static size_t dev_idx = 0;
static size_t proc_idx = 0;

static bool    dev_create( int count, struct file_operations* fops );
static bool    dev_remove( int count );
static int     dev_var2_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t dev_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t dev_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int     dev_var2_release( struct inode* ptr_inode, struct file* ptr_file );


static dev_t dev_var2_first_device_id;

static struct file_operations dev_var2_fops = {
  .owner   = THIS_MODULE,
  .open    = dev_var2_open,
  .read    = dev_var2_read,
  .write   = dev_var2_write,
  .release = dev_var2_release
};


static int     proc_var2_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t proc_var2_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t proc_var2_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int     proc_var2_release( struct inode* ptr_inode, struct file* ptr_file );

static struct proc_dir_entry* proc_var2_entry = NULL;
static const struct proc_ops proc_var2_ops = {
  .proc_open    = proc_var2_open,
  .proc_read    = proc_var2_read,
  .proc_write   = proc_var2_write,
  .proc_release = proc_var2_release
};


static int __init init_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module inited with number of devices: %d\n", nr_devices );

  if ( dev_create( nr_devices, &dev_var2_fops ) == false ) 
    return -EINVAL;
  printk( KERN_INFO MOD_NAME ": /dev/var2_[0-%d] created\n", nr_devices - 1 );

  proc_var2_entry = proc_create( DEV_NAME, 0666, NULL, &proc_var2_ops ); // 0444 -> r--r--r-- 0666 -> rw-rw-rw-
  if ( proc_var2_entry == NULL )
    return -EINVAL;
  printk( KERN_INFO MOD_NAME ": proc entry successfully created\n" );
  return 0;
}

static void __exit cleanup_chr_comp( void ) {
  printk( KERN_INFO MOD_NAME ": module cleaned up\n" );

  proc_remove( proc_var2_entry );

  dev_remove( nr_devices );
}

module_init( init_chr_comp );
module_exit( cleanup_chr_comp );


static int proc_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file /proc/%s opened\n", ptr_file->f_path.dentry->d_iname );
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
  printk( KERN_INFO MOD_NAME ": file /proc/%s closed\n", ptr_file->f_path.dentry->d_iname );
  return 0;
}


static bool dev_create( int count, struct file_operations* fops ) {
  int i;
  int major;
  dev_t dev_inst;
  if ( alloc_chrdev_region( &dev_var2_first_device_id, 0, count, MOD_NAME "_devices" ) != 0 ) {
    // unregister_chrdev_region( *first_dev_id, count ); // void can't check for the errors
    printk( KERN_INFO MOD_NAME ": cant allocate character devices region\n" );
    return false;
  }
  printk( KERN_INFO MOD_NAME ": region successfully allocated\n" );

  major = MAJOR( dev_var2_first_device_id );
  var2_devices = kmalloc( sizeof(struct cdev), GFP_KERNEL );
  if ( var2_devices == NULL ) {
    printk( KERN_INFO MOD_NAME ": cant allocate memory for array of struct cdev\n" );
    unregister_chrdev_region( dev_var2_first_device_id, count ); // void can't check for the errors
    return false;
  }
  printk( KERN_INFO MOD_NAME ": cdev allocated successfully\n" );

  if ( ( device_class = class_create( THIS_MODULE, CLASS_NAME ) ) == NULL ) {
    printk( KERN_INFO MOD_NAME ": cant create class for the device\n" );
    kfree( var2_devices );
    unregister_chrdev_region( dev_var2_first_device_id, count ); // void can't check for the errors
    return false;
  }
  printk( KERN_INFO MOD_NAME ": device class created\n" );

  for ( i = 0; i < count; ++i ) {

    dev_inst = MKDEV( major, i );
    cdev_init( &( var2_devices[ i ] ), fops );

    if ( cdev_add( &( var2_devices[ i ] ), dev_inst, 1 ) == 0 ) {
      printk( KERN_INFO MOD_NAME ": cdev %d added to subsystem\n", i );
      if ( device_create( device_class, NULL, dev_inst, NULL, DEV_NAME "_%d", i ) == NULL ) {
        int j = i;
        for ( ; j >= 0; --j ) {
          printk( KERN_INFO MOD_NAME ": cannot create device %d\n", j );
          cdev_del( &( var2_devices[ i ] ) );
          class_destroy( device_class );
          kfree( var2_devices );
          unregister_chrdev_region( dev_var2_first_device_id, count ); // void can't check for the errors
        }
        return false;
      }
    } else {
      int j = i;
      for ( ; j >= 0; --j ) {
        printk( KERN_INFO MOD_NAME ": failed adding cdev %d to subsystem\n", j );
        class_destroy( device_class );
        kfree( var2_devices );
        unregister_chrdev_region( dev_var2_first_device_id, count ); // void can't check for the errors
      }
      return false;
    }
    printk( KERN_INFO MOD_NAME ": var2_%d successfully added\n", i );
  }
  
  printk( KERN_INFO MOD_NAME ": all devices added\n" );
  return true;
}

static bool dev_remove( int count ) {
  const int major = MAJOR( dev_var2_first_device_id );
  int i;
  printk( KERN_INFO MOD_NAME ": removing all devices\n" );
  for ( i = 0; i < count; ++i ) {
    dev_t dev_inst = MKDEV( major, i );
    device_destroy( device_class, dev_inst );
    cdev_del( &( var2_devices[ i ] ) );
    printk( KERN_INFO MOD_NAME ": var2_%d deleted\n", i );
  }

  class_destroy( device_class );
  kfree( var2_devices );
  unregister_chrdev_region( dev_var2_first_device_id, count ); // void can't check for the errors
  printk( KERN_INFO MOD_NAME ": all resources freed\n" );
  return true;
}

static int dev_var2_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO MOD_NAME ": file /dev/%s opened\n", ptr_file->f_path.dentry->d_iname );
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
  printk( KERN_INFO MOD_NAME ": file /dev/%s closed\n", ptr_file->f_path.dentry->d_iname );
  return 0;
}