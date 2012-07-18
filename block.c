#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
 
#include "parapin.h"
#include "sd.h"
 
MODULE_LICENSE("Dual BSD/GPL");
 
 
static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors;
static int hardsect_size = 512;
module_param(nsectors, int, 0);
 
unsigned char PPORT_ACTIVATED = 0;
/*
* We can tweak our hardware sector size, but the kernel talks to us
* in terms of small sectors, always.
*/
#define KERNEL_SECTOR_SIZE 512
 
/*
* Our request queue.
*/
static struct request_queue *Queue;
 
/*
* The internal representation of our device.
*/
static struct sbd_device {
    unsigned long size;
    spinlock_t lock;
    u8 *data;
    struct gendisk *gd;
} Device;
 
/*
* Handle an I/O request.
*/
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
unsigned long nsect, char *buffer, int write)
{
    unsigned long offset = sector*hardsect_size;
    unsigned long nbytes = nsect*hardsect_size;
     
    if ((offset + nbytes) > dev->size) {
        printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }
    if (write)
    mmc_write_multiple_sector(sector, buffer, nsect);
    else
    mmc_read_multiple_sector(sector, buffer, nsect);
     
}
 
static void sbd_request(struct request_queue *q) {
    struct request *req;
     
    req = blk_fetch_request(q);
    while (req != NULL) {
        // blk_fs_request() was removed in 2.6.36 - many thanks to
        // Christian Paro for the heads up and fix...
        //if (!blk_fs_request(req)) {
            if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
                printk (KERN_NOTICE "Skip non-CMD request\n");
                __blk_end_request_all(req, -EIO);
                continue;
            }
            sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
            req->buffer, rq_data_dir(req));
            if ( ! __blk_end_request_cur(req, 0) ) {
                req = blk_fetch_request(q);
            }
        }
    }
     
    /*
    * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
    * calls this. We need to implement getgeo, since we can't
    * use tools such as fdisk to partition the drive otherwise.
    */
    int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
        long size;
         
        /* We have no real geometry, of course, so make something up. */
        size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
        geo->cylinders = (size & ~0x3f) >> 6;
        geo->heads = 4;
        geo->sectors = 16;
        geo->start = 4;
        return 0;
    }
     
    /*
    * The device operations structure.
    */
    static struct block_device_operations sbd_ops = {
        .owner = THIS_MODULE,
        .getgeo = sbd_getgeo
    };
     
    static int __init sbd_init(void){
         
        if (pin_init_kernel(0,NULL) < 0) {
            printk("LPT1 not available\n");
            return -ENOMEM;
            } else {
            printk("SUCCESS! LPT1 IS AVAILABLE!!!!\n");
            PPORT_ACTIVATED = 1;
            pin_output_mode(LP_DATA_PINS | LP_SWITCHABLE_PINS);
            if(mmc_init() != 0)
            if(mmc_init() != 0)
            if(mmc_init() != 0)
            goto out_unreg_pport;
             
        }
        /*
        * Set up our internal device.
        */
        Device.size = read_card_size()-(512);
        printk("\nMEMORY SIZE = %lu\n",Device.size);
         
        nsectors = Device.size / logical_block_size;
         
        spin_lock_init(&Device.lock);
        Queue = blk_init_queue(sbd_request, &Device.lock);
        if (Queue == NULL)
        goto out;
        blk_queue_logical_block_size(Queue, logical_block_size);
        /*
        * Get registered.
        */
        major_num = register_blkdev(major_num, "sbd");
        if (major_num <= 0) {
            printk(KERN_WARNING "sbd: unable to get major number\n");
            goto out;
        }
        /*
        * And the gendisk structure.
        */
        Device.gd = alloc_disk(16);
        if (!Device.gd)
        goto out_unregister;
        Device.gd->major = major_num;
        Device.gd->first_minor = 0;
        Device.gd->fops = &sbd_ops;
        Device.gd->private_data = &Device;
        strcpy(Device.gd->disk_name, "sbd");
        set_capacity(Device.gd, nsectors);
        Device.gd->queue = Queue;
        add_disk(Device.gd);
         
        return 0;
         
        out_unregister:
        unregister_blkdev(major_num, "sbd");
        out:
        vfree(Device.data);
        return -ENOMEM;
        out_unreg_pport:
        pin_release();
        return -ENOMEM;
    }
     
    static void __exit sbd_exit(void)
    {
        if(PPORT_ACTIVATED)
        pin_release();
         
        del_gendisk(Device.gd);
        put_disk(Device.gd);
        unregister_blkdev(major_num, "sbd");
        blk_cleanup_queue(Queue);
        vfree(Device.data);
    }
     
    module_init(sbd_init);
    module_exit(sbd_exit);
