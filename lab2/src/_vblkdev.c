#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/blk-mq.h>
#include <linux/string.h>
#include <linux/lockdep.h>
#include <linux/lockdep_types.h>
#include <linux/proc_fs.h> /* essential for procfs */

int partcfg = 1;
module_param(partcfg, int, 0660);

#define DISK_NAME "ramvdisk"

#define MEMSIZE 0x19000 // Size of Ram disk in sectors

int major = 0; //Variable for Major Number

#define MDISK_SECTOR_SIZE 512
#define MBR_SIZE MDISK_SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16
#define PARTITION_TABLE_SIZE 64
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE MDISK_SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55

typedef struct {
    unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
    unsigned char start_head;
    unsigned char start_sec: 6;
    unsigned char start_cyl_hi: 2;
    unsigned char start_cyl;
    unsigned char part_type;
    unsigned char end_head;
    unsigned char end_sec: 6;
    unsigned char end_cyl_hi: 2;
    unsigned char end_cyl;
    unsigned int abs_start_sec;
    unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

#define SEC_PER_HEAD 63
#define HEAD_PER_CYL 255
#define HEAD_SIZE (SEC_PER_HEAD * MDISK_SECTOR_SIZE)
#define CYL_SIZE (SEC_PER_HEAD * HEAD_PER_CYL * MDISK_SECTOR_SIZE)

#define sec4size(s) ((((s) % CYL_SIZE) % HEAD_SIZE) / MDISK_SECTOR_SIZE)
#define head4size(s) (((s) % CYL_SIZE) / HEAD_SIZE)
#define cyl4size(s) ((s) / CYL_SIZE)

// 2 primary partitions, 1 extended partition
static PartTable def_part_table = {
    {
        .boot_type =  0x00,
        .start_sec =  0x2,
        .start_head =  0x0,
        .start_cyl =  0x0,
        .part_type =  0x83,
        .end_head =  0x3,
        .end_sec =  0x20,
        .end_cyl =  0x9F,
        .abs_start_sec =  0x1,
        .sec_in_part =  0x4FFF // 10Mbyte
    },
    {
        .boot_type =  0x00,
        .start_sec =  0x2,
        .start_head =  0x4,
        .start_cyl =  0x0,
        .part_type =  0x83,
        .end_head =  0xB,
        .end_sec =  0x20,
        .end_cyl =  0x9F,
        .abs_start_sec =  0x5000,
        .sec_in_part =  0x9FFF // 20Mbyte
    },
    {
        .boot_type =  0x00,
        .start_sec =  0x1,
        .start_head =  0xC,
        .start_cyl =  0x0,
        .part_type =  0x05, // extended partition type
        .end_sec =  0x20,
        .end_head =  0x13,
        .end_cyl =  0x9F,
        .abs_start_sec =  0xF000,
        .sec_in_part =  0xA000 //20Mbyte
    }
};

static unsigned int def_log_part_br_abs_start_sector_1[] = {0xF000};
static const PartTable def_log_part_table_1[] = {
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x9FFF
        }
    }
};

static unsigned int def_log_part_br_abs_start_sector_2[] = {0xF000, 0x14000};
static const PartTable def_log_part_table_2[] = {
    {
        {
            .boot_type =  0x00,
            .start_head =  0xC,
            .start_sec =  0x2,
            .start_cyl =  0x0,
            .part_type =  0x83,
            .end_head =  0xF,
            .end_sec =  0x20,
            .end_cyl =  0x9F,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x4FFF
        },
        {
            .boot_type =  0x00,
            .start_head =  0x10,
            .start_sec =  0x01,
            .start_cyl =  0x00,
            .part_type =  0x05,
            .end_head =  0x13,
            .end_sec =  0x20,
            .end_cyl =  0x9F,
            .abs_start_sec =  0x5000,
            .sec_in_part =  0x5000
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0x10,
            .start_sec =  0x02,
            .start_cyl =  0x00,
            .part_type =  0x83,
            .end_head =  0x13,
            .end_sec =  0x20,
            .end_cyl =  0x9F,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x4FFF
        }
    }
};

static unsigned int def_log_part_br_abs_start_sector_3[] = {0xF000, 0x12555, 0x15aaa};
static const PartTable def_log_part_table_3[] = {
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x3554
        }, // link to the next
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x05,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x3555,
            .sec_in_part =  0x3555
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x3554
        }, // link to the next
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x05,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x6aaa,
            .sec_in_part =  0x3556
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x3555
        }
    }
};

static unsigned int def_log_part_br_abs_start_sector_4[] = {0xF000, 0x11800, 0x14000, 0x16800};
static const PartTable def_log_part_table_4[] = {
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x27ff
        }, // link to the next
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x05,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x2800,
            .sec_in_part =  0x2800
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x27ff
        }, // link to the next
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x05,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x5000,
            .sec_in_part =  0x2800
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x27ff
        }, // link to the next
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x05,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x7800,
            .sec_in_part =  0x2800
        }
    },
    {
        {
            .boot_type =  0x00,
            .start_head =  0xfe,
            .start_sec =  0xff,
            .start_cyl =  0xff,
            .part_type =  0x83,
            .end_head =  0xfe,
            .end_sec =  0xff,
            .end_cyl =  0xff,
            .abs_start_sec =  0x1,
            .sec_in_part =  0x27ff
        }
    }
};

static int     proc_disk_open( struct inode* ptr_inode, struct file* ptr_file );
static ssize_t proc_disk_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset );
static ssize_t proc_disk_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset );
static int     proc_disk_release( struct inode* ptr_inode, struct file* ptr_file );

static struct proc_dir_entry* proc_disk_entry = NULL;
static const struct proc_ops proc_disk_ops = {
  .proc_open    = proc_disk_open,
  .proc_read    = proc_disk_read,
  .proc_write   = proc_disk_write,
  .proc_release = proc_disk_release
};

static void copy_mbr( u8* disk ) {
    memset( disk, 0x0, MBR_SIZE );
    *(unsigned long*) ( disk + MBR_DISK_SIGNATURE_OFFSET ) = 0x36E5756D;
    memcpy( disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE );
    *(unsigned short*) ( disk + MBR_SIGNATURE_OFFSET ) = MBR_SIGNATURE;
}

static void copy_br( u8* disk, int abs_start_sector, const PartTable* part_table ) {
    disk += ( abs_start_sector * MDISK_SECTOR_SIZE );
    memset( disk, 0x0, BR_SIZE );
    memcpy( disk + PARTITION_TABLE_OFFSET, part_table, PARTITION_TABLE_SIZE );
    *(unsigned short*) ( disk + BR_SIGNATURE_OFFSET ) = BR_SIGNATURE;
}

void copy_mbr_n_br( u8* disk, int config ) {
    int i;
    int size = 1;
    switch ( config ) {
        case 2:
            size = ARRAY_SIZE( def_log_part_table_2 ); break;
        case 3:
            size = ARRAY_SIZE( def_log_part_table_3 ); break;
        case 4:
            size = ARRAY_SIZE( def_log_part_table_4 ); break;
        case 1:
        default:
            size = ARRAY_SIZE( def_log_part_table_1 );
    }

    copy_mbr( disk );
    for ( i = 0; i < size; ++i )
        switch ( config ) {
            case 2:
                copy_br( disk, def_log_part_br_abs_start_sector_2[ i ], &def_log_part_table_2[ i ] ); break;
            case 3:
                copy_br( disk, def_log_part_br_abs_start_sector_3[ i ], &def_log_part_table_3[ i ] ); break;
            case 4:
                copy_br( disk, def_log_part_br_abs_start_sector_4[ i ], &def_log_part_table_4[ i ] ); break;
            case 1:
            default:
                copy_br( disk, def_log_part_br_abs_start_sector_1[ i ], &def_log_part_table_1[ i ] ); break;
        }
}

/* Structure associated with Block device*/
struct ramv_disk_device {
    size_t size;
    u8* data;
    atomic_t nr_users;
    // spinlock_t lock;
    struct blk_mq_tag_set tag_set;
    struct request_queue* queue;
    struct gendisk* gd;
} device;

// struct ramv_disk_device *x;

static int ramvdisk_open( struct block_device* x, fmode_t mode ) {
    struct ramv_disk_device* dev = x->bd_disk->private_data;
    if ( dev == NULL ) {
        printk( KERN_WARNING DISK_NAME "drive: invalid disk private_data\n" );
        return -ENXIO;
    }

    atomic_inc(&dev->nr_users);
    printk( KERN_INFO DISK_NAME "drive: open \n" );

    return 0;

}

static void ramvdisk_release(struct gendisk *disk, fmode_t mode) {
    struct ramv_disk_device* dev = disk->private_data;
    if ( dev ) {
        atomic_dec( &dev->nr_users );
        printk( KERN_INFO DISK_NAME "drive: closed \n" );
    } else printk( KERN_WARNING DISK_NAME "drive: invalid disk private_data\n" );
}

static struct block_device_operations fops = {
    .owner = THIS_MODULE,
    .open = ramvdisk_open,
    .release = ramvdisk_release,
};

int ramvdisk_init( int nr_config ) {
    device.size = MEMSIZE;
    ( device.data ) = vmalloc( MEMSIZE * MDISK_SECTOR_SIZE );
    if ( device.data == NULL ) {
        printk( KERN_WARNING DISK_NAME "drive: vmalloc failure.\n" );
        return -ENOMEM;
    }

    /* Setup its partition table */
    copy_mbr_n_br(device.data, nr_config);

    return 0;
}

static int rb_transfer( struct request *req, unsigned int* nr_bytes ) {
    int dir = rq_data_dir( req );
    // int ret = 0;
    /* starting sector where to do operation */
    sector_t start_sector = blk_rq_pos( req );
    unsigned int sector_cnt = blk_rq_sectors( req ); /* no of sector on which opn to be done*/

    struct bio_vec bv;
#define BV_PAGE( bv ) ( (bv).bv_page )
#define BV_OFFSET( bv ) ( (bv).bv_offset )
#define BV_LEN( bv ) ( (bv).bv_len )

    struct req_iterator iter;
    sector_t sector_offset = 0;
    unsigned int sectors;
    u8* buffer;

    rq_for_each_segment( bv, req, iter ) {
        buffer = page_address( BV_PAGE( bv ) ) + BV_OFFSET( bv );

        if ( BV_LEN( bv ) % ( MDISK_SECTOR_SIZE ) != 0 ) {
            printk( KERN_ERR DISK_NAME ": bio size is not a multiple ofsector size\n" );
            return -EIO;
        }

        sectors = BV_LEN( bv ) / MDISK_SECTOR_SIZE;
        printk( KERN_DEBUG DISK_NAME ": Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Length: %u sectors\n",\
            (unsigned long long) ( start_sector ), ( unsigned long long ) \
            ( sector_offset ), buffer, sectors );

        if ( dir == WRITE ) /* Write to the device */ 
            memcpy( (device.data) + ( ( start_sector + sector_offset ) * MDISK_SECTOR_SIZE ), buffer, sectors * MDISK_SECTOR_SIZE );
        else /* Read from the device */
            memcpy(buffer, (device.data) + ( ( start_sector + sector_offset ) * MDISK_SECTOR_SIZE ), sectors * MDISK_SECTOR_SIZE);

        sector_offset += sectors;
        *nr_bytes += sectors * MDISK_SECTOR_SIZE;
    }

    if ( sector_offset != sector_cnt ) {
        printk( KERN_ERR DISK_NAME ": bio info doesn't match with the request info\n" );
        return -EIO;
    }

    return 0;
#undef BV_PAGE
#undef BV_OFFSET
#undef BV_LEN
}

/* request handling function */
/**
 * Changing according to blk multi-queue feature
 */
static blk_status_t do_request( struct blk_mq_hw_ctx* hctx, const struct blk_mq_queue_data* bd ) {
    blk_status_t status = BLK_STS_OK;
    unsigned int nr_bytes = 0;

    struct request* req = bd->rq;
    switch ( rb_transfer( req, &nr_bytes ) ) {
        case -EIO: {
            status = BLK_STS_IOERR;
        }
            break;
        case 0:
        default: {
            status = BLK_STS_OK;
        }
            break;
    };

#if 0 //simply and can be called from proprietary module
    blk_mq_end_request( req, status );
#else //can set real processed bytes count
    if ( blk_update_request( req, status, nr_bytes ) ) //GPL-only symbol
        BUG();
    __blk_mq_end_request( req, status );
#endif

    return status;
}

static struct blk_mq_ops ramv_queue_ops = {
    .queue_rq = do_request,
};

static int device_gendisk_setup( int nr_config ) {
    if ( ramvdisk_init( nr_config ) )
        return -ENOMEM;

    /* Allocate queue */
    // device.queue = blk_( &(device.tag_set) ); // blk_init_queue( do_request, &device.lock);
    // if ( IS_ERR( device.queue) ) {
    //     blk_mq_free_tag_set( &(device.tag_set) );
    //     return -ENOMEM;
    // }

    // // blk_queue_logical_block_size( device.queue, MDISK_SECTOR_SIZE );
    // device.queue->queuedata = &device;

    /* Initialize gendisk */
    // can't understand why we have here 8 minors
    device.gd = blk_mq_alloc_disk( &( device.tag_set ), &device ); // gendisk allocation
    if( !device.gd ) {
        printk(KERN_INFO DISK_NAME ": alloc_disk failure\n");
        return -EBUSY;
    }

    device.queue = device.gd->queue;

    (device.gd)->major = major; // major no to gendisk
    device.gd->first_minor = 0; // first minor of gendisk
    device.gd->minors = 8; // set strange 8 minors

    device.gd->fops = &fops;
    device.gd->private_data = &device;
    device.gd->queue = device.queue;
    printk( KERN_INFO "THIS IS DEVICE SIZE %zu\n", device.size );

    /* Use buffer-safe functions */
    snprintf( ((device.gd)->disk_name), DISK_NAME_LEN, DISK_NAME );

    set_capacity( device.gd, device.size );

    /* Achtung: after add_disk invokation it's possible to send request to disk */
    add_disk( device.gd );
    return 0;
}

int device_setup( void ) {

    /* Register block device */
    major = register_blkdev( major, DISK_NAME );// major no. allocation

    if( major <= 0 ) {
        printk( KERN_WARNING DISK_NAME ": unable to get major number\n" );
        return -EBUSY;
    }

    printk( KERN_ALERT "Major Number is : %d\n", major );

    // spin_lock_init( &device.lock ); // lock for queue

    /* Initialize tag set */
    device.tag_set.ops = &ramv_queue_ops;
    device.tag_set.nr_hw_queues = 1;
    device.tag_set.queue_depth = 128;
    device.tag_set.numa_node = NUMA_NO_NODE;
    device.tag_set.cmd_size = sizeof( struct ramv_disk_device );
    device.tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
    device.tag_set.driver_data = &device;
    if ( blk_mq_alloc_tag_set( &(device.tag_set) ) )
        return -ENOMEM;

    return device_gendisk_setup( partcfg );
}

void ramvdisk_cleanup( void ) {
    if ( device.gd ) {
        del_gendisk( device.gd );
    }
    // blk_mq_free_tag_set( &(device.tag_set) );
    if ( device.data )
        vfree( device.data );
}

static int proc_disk_open( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO DISK_NAME ": file /proc/%s opened\n", ptr_file->f_path.dentry->d_iname );
  return 0;
}

static ssize_t proc_disk_read( struct file* ptr_file, char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  if ( *ptr_offset > 0 ) return 0;
  *ptr_offset += length;
  return length;
}

static ssize_t proc_disk_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_offset ) {
  int nr_config;
  size_t count = sscanf( usr_buf, "%d", &nr_config );

  printk( KERN_INFO DISK_NAME ": proc_disk_write: count = %zu, nr_config, length = %zu\n", count, nr_config, length );
  
  if ( nr_config <= 0 || nr_config > 4 )
    return -EINVAL;

  if ( count == 1 ) {
    // do disk reconfiguration
    ramvdisk_cleanup();
    printk( KERN_INFO DISK_NAME ": proc_disk_write: disk cleaned up\n" );
    device_gendisk_setup( nr_config );
    printk( KERN_INFO DISK_NAME ": proc_disk_write: disk reconfigured\n" );
    return length;
  }
  return -EINVAL;
}

static int proc_disk_release( struct inode* ptr_inode, struct file* ptr_file ) {
  printk( KERN_INFO DISK_NAME ": file /proc/%s closed\n", ptr_file->f_path.dentry->d_iname );
  return 0;
}

static int __init ramvdisk_drive_init(void) {
    int ret = 0;
    proc_disk_entry = proc_create( DISK_NAME, 0666, NULL, &proc_disk_ops );
    if ( proc_disk_entry == NULL )
        return -EINVAL;
    ret = device_setup( );
    return ret;
}

void __exit ramvdisk_drive_exit(void) {
    proc_remove( proc_disk_entry );
    // cleanup device buffer in ram
    ramvdisk_cleanup( );
    if ( device.gd )
        blk_cleanup_disk( device.gd );
    
    unregister_blkdev( major, DISK_NAME );
    printk( KERN_INFO ": module successfully unloaded\n" );
}

module_init( ramvdisk_drive_init );
module_exit( ramvdisk_drive_exit );
MODULE_LICENSE( "Dual MIT/GPL" );
MODULE_AUTHOR( "Lenar" );
MODULE_AUTHOR( "Michael" );
MODULE_DESCRIPTION( "BLOCK DRIVER" );
