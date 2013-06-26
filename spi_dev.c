#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <asm/arch/at91_spi.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#define SPIDEV_DEBUG
#undef SPIDEV_DEBUG

static int spidev_open(struct inode* inode, struct file *file) {
#ifdef SPIDEV_DEBUG
	printk("[KERNEL INFO] : SPI OPEN.\n");
#endif /* SPIDEV_DEBUG */
	unsigned int spi_device = MINOR(inode->i_rdev);

	if (spi_device >= NR_SPI_DEVICES)
		return -ENODEV;

	/*
	 * 'private_data' is actually a pointer, but we overload it with the
	 * value we want to store.
	 */
	file->private_data = (void *) spi_device;

	return 0;
}

static int spidev_close(struct inode * inode, struct file * file) {
#ifdef SPIDEV_DEBUG
	printk("[KERNEL INFO] : SPI CLOSE.\n");
#endif /* SPIDEV_DEBUG */
	return 0;
}

static int send_data(char* data, int len) {

	char * plocal_data = (char *) kmalloc(sizeof(char)*len, GFP_KERNEL);
	memcpy(plocal_data, data, len);
	struct spi_transfer_list list = {
			tx : { plocal_data },
			txlen : { len },
			rx : { data },
			rxlen : { len },
			nr_transfers : 1, };
	spi_transfer(&list);

#ifdef SPIDEV_DEBUG
	int i;
	for (i = 0; i < len; i++)
		printk("[DEUBG INFO] : sending %x result %x\n", *(plocal_data + i), *(data + i));
#endif /* SPIDEV_DEBUG */

	kfree(plocal_data);
	return  0;
}

static ssize_t spidev_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char buffer[100];

#ifdef SPIDEV_DEBUG
	printk("[KERNEL INFO] : Enter Write Info.\n");
#endif /* SPIDEV_DEBUG */

	if (copy_from_user(buffer, userbuf, count))
	return -EFAULT;
	/*
	 printk("[KERNLE INFO] : len = %d.\n", len);
	 for(i=0;i<len;i++){
	 printk("[KERNEL INFO] : buff[%d] = 0x%x.\n",i,buffer[i]);
	 }
	 */
	spi_access_bus(0);
	send_data(buffer,count);
	spi_release_bus(0);

	return 0;
}

/******************************************************************************/
static ssize_t spidev_read(struct file * file,  char __user * userbuf, size_t count, loff_t * off)
{

	char buffer[100];
	memset(buffer,0, 100);

	spi_access_bus(0);
	send_data(buffer, count);
	spi_release_bus(0);
#ifdef SPIDEV_DEBUG
	int i;
	for (i = 0; i < len; i++)
		printk("[KERNEL INFO] : Enter Read Info.%x\n", buffer[i]);
#endif /* SPIDEV_DEBUG */


	if (copy_to_user(userbuf, buffer, count))
		return -EFAULT;


#ifdef SPIDEV_DEBUG
	printk("[KERNEL INFO] : copy_to_user finished.\n");
#endif /* SPIDEV_DEBUG */
	return 0;
}

/******************************************************************************/

static struct file_operations spidev_fops = {
	.owner = THIS_MODULE,
	.read  =  spidev_read,
	.write = spidev_write, 
	.open = spidev_open, 
	.release = spidev_close,
};

static int __init spi_dev_init(void)
{
#ifdef CONFIG_DEVFS_FS
	int i;
#endif	

	if (register_chrdev(SPI_MAJOR, "spi", &spidev_fops)) {
		printk(KERN_ERR "at91_spidev: Unable to get major %d for SPI bus\n", SPI_MAJOR);
		return -EIO;
	}

#ifdef CONFIG_DEVFS_FS
	devfs_mk_dir("spi");
	for (i = 0; i < NR_SPI_DEVICES; i++) {
		devfs_mk_cdev(MKDEV(SPI_MAJOR, i), S_IFCHR | S_IRUSR | S_IWUSR, "spi/%d",i);
	}
#endif	
	printk(KERN_INFO "AT91 SPI driver loaded\n");

	return 0;
}

static void __exit spi_dev_exit(void)
{
#ifdef CONFIG_DEVFS_FS
	int i;
	for (i = 0; i < NR_SPI_DEVICES; i++) {
		devfs_remove("spi/%d", i);
	}

	devfs_remove("spi");
#endif	

	if (unregister_chrdev(SPI_MAJOR, "spi")) {
		printk(KERN_ERR "at91_spidev: Unable to release major %d for SPI bus\n", SPI_MAJOR);
		return;
	}
}

module_init( spi_dev_init);
module_exit( spi_dev_exit);
MODULE_LICENSE("GPL");
