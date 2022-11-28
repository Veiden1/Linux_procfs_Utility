#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>                 // kmalloc()
#include <linux/uaccess.h>              // copy_to/from_user()
#include <linux/proc_fs.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/path.h>
#include <linux/blk_types.h>
#include <linux/bio.h>
#include <linux/pci.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/blkdev.h>

#define BUF_SIZE 4096

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Stab linux module for operating system's lab");
MODULE_VERSION("1.0");

static int struct_id = 0;
static char * path_name = "";

static struct proc_dir_entry *parent;

/*
** Function Prototypes
*/
static int      __init lab_driver_init(void);
static void     __exit lab_driver_exit(void);


/***************** Procfs Functions *******************/
static int      open_proc(struct inode *inode, struct file *file);
static int      release_proc(struct inode *inode, struct file *file);
static ssize_t  read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t  write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);

/*
** procfs operation sturcture
*/
static struct proc_ops proc_fops = {
        .proc_open = open_proc, 
        .proc_read = read_proc,
        .proc_write = write_proc,
        .proc_release = release_proc
};


// pci_dev

static size_t write_pci_dev(char __user *ubuf){
    char buf[BUF_SIZE];
    size_t len = 0;
    
    static struct pci_dev *dev;
    
    len += sprintf(buf, "pci_dev is requested:\n");
    
    while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev))){
    	printk(KERN_INFO "PCI found: [%hu] -> %s\n", dev->device, pci_name(dev));
    	len += sprintf(buf+len, "PCI found: [%hu] -> %s\n", dev->device, pci_name(dev));
    	if (dev->slot != NULL){
    		len += sprintf(buf+len, "slot_name: %s\n\n", pci_slot_name(dev->slot));
    	}
    }
    
    if (copy_to_user(ubuf, buf, len)){
        return -EFAULT;
    }

    return len;
}

// block_device

static size_t write_block_device(char __user *ubuf){
    char buf[BUF_SIZE];
    size_t len = 0;
    
    struct path current_path;
    struct block_device *bd;
    int err;
    
    len += sprintf(buf, "block_device is requested:\n");
 	   
    if (path_name == NULL) return len;
    
    printk(KERN_INFO "path %s\n", path_name);
    
    err = kern_path(path_name, LOOKUP_FOLLOW, &current_path);
    if (err){
    	printk(KERN_INFO "Error to find path %s\n", path_name);
    	len += sprintf(buf+len, "Error to find path %s\n", path_name);
    	
    	if (copy_to_user(ubuf, buf, len)){
        	return -EFAULT;
    	}
    	
    	return len;
    }
    
    bd = current_path.dentry->d_inode->i_sb->s_bdev;
    
    if (bd == NULL){
    	printk(KERN_INFO "Block_device is NULL!!!\n");
    	len += sprintf(buf+len, "Block_device is NULL!!! for path = %s\n", path_name);
    	
    	if (copy_to_user(ubuf, buf, len)){
        	return -EFAULT;
    	}
    	
    	return len;
    }
    
    len += sprintf(buf+len, "Device name: %s\n", dev_name(&(bd->bd_device)));
    len += sprintf(buf+len, "Device bus name: %s\n", dev_bus_name(&(bd->bd_device)));
    len += sprintf(buf+len, "Holders = %d\n", bd->bd_holders);
    len += sprintf(buf+len, "Sector = %lld\n", bd->bd_start_sect);
    
    if (copy_to_user(ubuf, buf, len)){
        	return -EFAULT;
    	}
    
    return len;
}


/*
** Эта фануция будет вызвана, когда мы ОТКРОЕМ файл procfs
*/
static int open_proc(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "proc file opend.....\t");
    return 0;
}

/*
** Эта фануция будет вызвана, когда мы ЗАКРОЕМ файл procfs
*/
static int release_proc(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "proc file released.....\n");
    return 0;
}

/*
** Эта фануция будет вызвана, когда мы ПРОЧИТАЕМ файл procfs
*/
static ssize_t read_proc(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos) {
    int len = 0;
    
    printk(KERN_INFO "proc file read.....\n");
    if (*ppos > 0 || count < BUF_SIZE){
        return 0;
    }
    
    switch(struct_id){
    	case 0:
    		len = write_pci_dev(ubuf);
    		break;
    	case 1:
    		len = write_block_device(ubuf);
    		break;
    }
    
    printk(KERN_INFO "proc file read end.\n");
    
    *ppos = len;
    return len;
}

/*
** Эта фануция будет вызвана, когда мы ЗАПИШЕМ в файл procfs
*/
static ssize_t write_proc(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos) {

    int num_of_read_digits, c, a;
    char b[BUF_SIZE];
    char buf[BUF_SIZE];
    
    printk(KERN_INFO "proc file wrote.....\n");

    if (*ppos > 0 || count > BUF_SIZE){
        return -EFAULT;
    }

    if( copy_from_user(buf, ubuf, count) ) {
        return -EFAULT;
    }
    
    sscanf(buf, "%d %4096s", &a, b);
    
    /*
    num_of_read_digits = sscanf(buf, "%d %s", &a, &b);
    if (num_of_read_digits != 2){
        return -EFAULT;
    }
    */

    struct_id = a;
    path_name = b;

    c = strlen(buf);
    *ppos = c;
    printk(KERN_INFO "proc file wrote end.\n");
    return c;
}

/*
** Функция инициализации Модуля
*/
static int __init lab_driver_init(void) {

    /* Создание директории процесса. Она будет создана в файловой системе "/proc" */
    parent = proc_mkdir("lab",NULL);

    if( parent == NULL )
    {
        pr_info("Error creating proc entry");
        return -1;
    }

    /* Создание записи процесса в разделе "/proc/lab/" */
    proc_create("struct_info", 0666, parent, &proc_fops);

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;
}

/*
** Функция выхода из Модуля
*/
static void __exit lab_driver_exit(void)
{
    /* Удаляет 1 запись процесса */
    //remove_proc_entry("lab/struct_info", parent);

    /* Удяление полностью /proc/lab */
    proc_remove(parent);
    
    pr_info("Device Driver Remove...Done!!!\n");
}

module_init(lab_driver_init);
module_exit(lab_driver_exit);
