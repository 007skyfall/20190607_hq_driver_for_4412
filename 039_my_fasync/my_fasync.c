#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define MAX 64
#define NAME "demo"
int flag = 0;
int count = 1;
char ker_buf[MAX] = {0};
int major = 0 , minor = 0;
struct fasync_struct *fasync = NULL;
/*
实现系统调用的read阻塞:read无数据  则等待 直到有数据  
原理:使用等待队列相关的机制

步骤:
	1.定义  初始化对应的等待队列头
	2.判断条件
		2.1  如果说条件不满足   阻塞等待--->wake_up_interrutible
		2.2  非阻塞   判断file->f_flags的O_NONBLOCK是否被设置?
*/

int demo_open(struct inode *inode, struct file *file)
{
	printk("%s,%d\n",__func__,__LINE__);
	return 0;
}
int demo_close(struct inode *inode, struct file *file)
{
	printk("%s,%d\n",__func__,__LINE__);
	return 0;
}
ssize_t demo_read(struct file *file, char __user *user, size_t size, loff_t *loff)//务必返回size，否则会出现问题
{
	if(size > MAX) size = MAX;

	if(copy_to_user(user,ker_buf,size)){
		printk("copy_to_user fail...\n");
		return -EINVAL;
	}

	printk("%s,%d\n",__func__,__LINE__);
	return size;
}
ssize_t demo_write(struct file *file, const char __user *user, size_t size, loff_t *loff)//务必返回size，否则会出现问题
{
	if(size > MAX) size = MAX;

	if(copy_from_user(ker_buf,user,size)){
		printk("copy_from_user fail...\n");
		return -EINVAL;
	}

	kill_fasync(&fasync,SIGIO,POLL_IN);
	printk("%s,%d\n",__func__,__LINE__);
	return size;
}

int demo_fasync (int fd , struct file *file, int on)
{
	return fasync_helper(fd,file,on,&fasync);
}

struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = demo_open,
	.release = demo_close,
	.read = demo_read,
	.write = demo_write,
	.fasync  = demo_fasync,
};


struct class *cls = NULL;
struct device *dev = NULL;

int __init demo_init(void)
{
	int ret ,i ;
	major = register_chrdev(major,NAME,&f_ops);
	if(major < 0){
		printk("register_chrdev fail ...%s,%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	cls = class_create(THIS_MODULE,"demo_class");
	if(IS_ERR(cls)){
		printk("class_create ...\n");
		ret = PTR_ERR(cls);
		goto ERR_STEP1;
	}
	for(i = minor ; i < minor + count ; i++){
		dev = device_create(cls,NULL,MKDEV(major,i),NULL,"%s%d","demo",i);
		if(IS_ERR(dev)){
			printk("class_create ...\n");
			ret = PTR_ERR(dev);
			goto ERR_STEP2;
		}
	}
	printk("major :%d \t %s,%d\n",major ,__func__,__LINE__);
	return 0;
ERR_STEP2:
	for(i-- ; i >= minor ; i--)
		device_destroy(cls,MKDEV(major,i));
	class_destroy(cls);
ERR_STEP1:
	unregister_chrdev(major,NAME);
	return ret;
}

void __exit demo_exit(void)
{
	int i ;
	for(i = minor ; i < minor + count ; i++)
		device_destroy(cls,MKDEV(major,i));
	class_destroy(cls);
	unregister_chrdev(major,NAME);
	printk("%s,%d\n",__func__,__LINE__);
}

module_init(demo_init);
module_exit(demo_exit);
MODULE_LICENSE("GPL");










