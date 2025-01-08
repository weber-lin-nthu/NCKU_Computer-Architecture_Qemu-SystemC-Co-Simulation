/*
 *  Copyright (c) 2018.  	Computer Architecture and System Laboratory, CASLab
 *							Dept. of Electrical Engineering & Inst. of Computer and Communication Engineering
 *							National Cheng Kung University, Tainan, Taiwan
 *  All Right Reserved
 *
 *	Written by  Dun-Jie Chen (cdj@caslab.ee.ncku.edu.tw)
 *   
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/page.h>

#include <linux/device.h>
#include <asm/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irqflags.h>

//Copy from Device spec.
#define CALC_REG_CONTROL	0x00
#define CALC_REG_OPERATOR	0x04
#define CALC_REG_OPERAND1	0x08
#define CALC_REG_OPERAND2	0x0C
#define CALC_REG_RESULT		0x10
#define CALC_REG_STATUS		0x14

#define CALC_STATUS_IDLE	0x00

#define CALC_CTRL_GO		0x01
#define CALC_CTRL_ABORT		0x02
#define CALC_CTRL_CLRI		0x04
#define CALC_CTRL_RESET		0x08

#define CALC_OP_ADD			0x01
#define CALC_OP_SUB			0x02
#define CALC_OP_MUL			0x03
#define CALC_OP_DIV			0x04
//

// Some CMD number is invaild(used by linux kernel)
// For detail read linux kernel document (Documentation/ioctl/ioctl-number.txt)
// and http://man7.org/linux/man-pages/man2/ioctl_list.2.html
#define CMD_STATUS			0x00
#define CMD_OP_SET			0x03
#define CMD_CTRL			0x04

#define BUFFER_SIZE			20

#define DRIVER_NAME			"caslab_calc_hw"
#define DEVICE_NAME			"caslab-calc"
#define DEVICE_COMP			"caslab,calc"
// Can be found within linux kernel Documentation/admin-guide/devices.txt
// 252 is LOCAL/EXPERIMENTAL USE
#define CASCALC_MAJOR		252
#define CASCALC_MINOR		0
#define CASCALC_BASE		0x80000000
#define CASCALC_MEMREGION	0x100
#define CASCALC_IRQ			30

MODULE_AUTHOR("DUN-JIE CHEN");
MODULE_LICENSE("GPL");

static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag = 0;

static unsigned int irqnum		= CASCALC_IRQ;

static int cascalc_major	= CASCALC_MAJOR;
static int cascalc_minor	= CASCALC_MINOR;

typedef struct {
	unsigned int buffer[BUFFER_SIZE];
} DevicePrivate;
static void __iomem *base;

static int get_dut_irq(const char* dev_compatible_name)
{
	struct device_node* dev_node;
	int irq = -1;
	dev_node = of_find_compatible_node(NULL, NULL, dev_compatible_name);
	if(!dev_node)
		return -1;
	irq = irq_of_parse_and_map(dev_node, 0);
	of_node_put(dev_node);
	return irq;
}

static DevicePrivate* allocate_memory_region(void)
{
	DevicePrivate* _prv;
	_prv = (DevicePrivate*)vmalloc(sizeof(DevicePrivate));
	return _prv;

}

static void free_allocated_memory(DevicePrivate *_prv)
{
	vfree(_prv);
	_prv = NULL;
}

static int calc_open(struct inode *inode, struct file *file)
{
	file->private_data = allocate_memory_region();
	if(file->private_data == NULL)
		return -ENOMEM;
	return 0;
}

static int calc_release(struct inode *inode, struct file *file)
{
	free_allocated_memory(file->private_data);
	return 0;
}

static ssize_t calc_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
	int i;
	DevicePrivate *prv_data = (DevicePrivate*)filp->private_data;

	/* Only result is readable memory address */
	if(len > 4)
		len = 4;

	for( i = 0; i < len/4; i++ )
		prv_data->buffer[i] = readl(base + CALC_REG_RESULT + i * 4);

	i = copy_to_user(buff, prv_data->buffer, len);

	return len;

}

static ssize_t calc_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	int i;
	DevicePrivate *prv_data = (DevicePrivate*)filp->private_data;

	/* Two operands are writeable memory address */
	if ( len > 8 )
		len = 8;
	
	i = copy_from_user(prv_data->buffer, buff, len);
	for(i = 0; i < len/4; i++)
		writel(prv_data->buffer[i], base + CALC_REG_OPERAND1 + i*4);

	return len;
}

static long calc_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned int tmp;
	int i;
	printk("IOCTL CMD = %u, value = %lu\n", cmd, arg);
	switch(cmd)
	{
		case CMD_STATUS:
			tmp = readl(base + CALC_REG_STATUS);
			i = copy_to_user((void*)arg, &tmp, sizeof(unsigned int));
		case CMD_OP_SET:
			if(arg >= CALC_OP_ADD && arg <= CALC_OP_DIV)
				writel(arg, base + CALC_REG_OPERATOR);
			else
				printk("Invaild OP %lu\n", arg);
			break;
		case CMD_CTRL:
			tmp = readl(base + CALC_REG_STATUS);
			if(tmp == CALC_STATUS_IDLE) {
				writel(arg, base + CALC_REG_CONTROL);
				wait_event_interruptible(wq, flag != 0);
				flag = 0;
			} else {
				if(arg == CALC_CTRL_ABORT || arg == CALC_CTRL_RESET) {
					writel(arg, base + CALC_REG_CONTROL);
					flag = 0;
				}
			}
			break;
		default:
			printk("Invaild CMD %u\n", cmd);
			ret = -ENOTTY;
			break;
	}
	return ret;
}

static struct cdev cascalc_cdev = {
	.kobj	= { .name = DRIVER_NAME },
	.owner	= THIS_MODULE
};

static struct file_operations cascalc_fops ={
	.owner				= THIS_MODULE,
	.read				= calc_read,
	.write				= calc_write,
	.unlocked_ioctl		= calc_unlocked_ioctl,
	.open				= calc_open,
	.release			= calc_release
};

static irqreturn_t cascalc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	if (irq == irqnum) {
		printk("received interrupt from %s\n", DRIVER_NAME);
		writel(CALC_CTRL_CLRI, base + CALC_REG_CONTROL);
		printk("Release interrupt \n");
		flag = 1;
		wake_up_interruptible(&wq);
	}
	return IRQ_HANDLED;
}

static int __init cascalc_init(void) {
	dev_t dev = 0;
	int result;
	if(CASCALC_MAJOR) {
		dev = MKDEV(cascalc_major, cascalc_minor);
		result = register_chrdev_region( dev, 1, DRIVER_NAME);
	} else {
		result = alloc_chrdev_region( &dev, cascalc_minor, 1, DRIVER_NAME);
		cascalc_major = MAJOR(dev);
	}
	if(result < 0) {
		printk(KERN_ERR DRIVER_NAME "Can't allocate major number %d for %s\n", cascalc_major, DEVICE_NAME);
		return -EAGAIN;
	}
	cdev_init( &cascalc_cdev, &cascalc_fops);
	result = cdev_add( &cascalc_cdev, dev, 1);
	if(result < 0) {
		printk(KERN_ERR DRIVER_NAME "Can't add device %d\n", dev);
		return -EAGAIN;
	}

	/* allocate memory */
	base = ioremap_nocache(CASCALC_BASE , CASCALC_MEMREGION);
	if(base == NULL)
		return -ENXIO;

	irqnum = get_dut_irq(DEVICE_COMP);
	if(irqnum == -1) {
		printk(KERN_ERR DRIVER_NAME " Can't request IRQ, Device not found\n");
		return -ENXIO;
	}
	
	result = request_irq(irqnum, (void *)cascalc_interrupt, 0, DEVICE_NAME, NULL);
	if(result) {
		printk(KERN_ERR DRIVER_NAME " Can't request IRQ %d (return %d)\n", irqnum, result);
		return -EFAULT;
	}

	return 0;
}

static void __exit cascalc_exit(void) {
	dev_t dev;
	dev = MKDEV(cascalc_major, cascalc_minor);
	unregister_chrdev_region( dev, 1);
	free_irq(irqnum, NULL);
	iounmap((unsigned long *) base);
	printk("CASLab_Calc driver exit.\n");
}

module_init(cascalc_init);
module_exit(cascalc_exit);
