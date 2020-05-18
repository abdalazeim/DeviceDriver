#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>//file operations strucute allows us to open/close read/write to device
#include <linux/cdev.h>//char driver makes cdev available
#include <linux/semaphore.h>//access semaphores synchorinzation behaviors
#include <asm/uaccess.h> //copy to user; from user

//1)create structue for user fake device
struct fake_device{
       char data[100];
       struct semaphore sem;
} virtual_device;
//2) for later register our device we need cdev object and some variables 
struct cdev *mcdev ;//m stands my
int major_number;//store  our major number extracted from dev_t using macro - mknod /director/file c major_minor
//will be used to hold return values of functions this is  because the kernel stack is very small so declaring variables all over the passin 
//our module functions eats up the stack very fast 	
int ret;
dev_t dev_num;//will hold major number that kernel gives us
 

#define DEVICE_NAME "soliddevice"

//7) called on device open, inode reference to the file on disk and contains information about that file , struct file is represnts and abstract open file 
int device_open(struct inode *inode,struct file *filep)
{
 //only allow one proccess to open this device by using a semaphore as mutual exclusive lock-mutex
if(down_interruptible(&virtual_device.sem)!=0)
{
 printk(KERN_ALERT "soliddevice could not lock device during open");
return -1;
}
 printk(KERN_INFO " device opened ");
return 0;
}//end of device open 

//8) called when user wants to get information from the device 
ssize_t device_read(struct file* filp,char* bufStoreData,ssize_t bufCount,loff_t* curOffset)
{
 //take data from kernrel space (device) to user space(proccess)
//copy_to_user (destination ,source.sizetotransfer)
printk(KERN_INFO "solidcode : reading from device");
ret= copy_to_user(bufStoreData,virtual_device.data,bufCount);
return ret;
}//end of read

//9) called when user wants to send information to the device
ssize_t device_write(struct file* filp,const char* bufSourceData,size_t bufCount,loff_t* curOffset)
{
 //send data from user to kernel
 //copy_from_user (dest,source,count)
printk(KERN_INFO "solidcode : writing to device");
ret = copy_from_user(virtual_device.data,bufSourceData,bufCount);
return ret;
 
}//end of device write 

//10) called upon user close
int device_close(struct inode* inode,struct file *filp)
{
//by calling up whic is opposite of down semaphore we relase the mutex that we obtained at device open this has the effect of allowing 
//other proccess to use the device now 
up(&virtual_device.sem);
printk(KERN_INFO "solidcode :device closed");
return 0;
}//end of close


//6)tell the kernel which functions to call when user operates on our device
struct file_operations fops = {
 .owner = THIS_MODULE,//prevent unloaded module when operation are in use
 .open  = device_open,//point to the method to call when opening the device file
 .release = device_close,
 .write  = device_write,
 .read   = device_read
};

static int driver_entry(void)
{ //strart driver entry 

//3) register our device with the system : a two step  proccess
//step 1) use dynamic allocation to assign our device
// major number -- alloc_chrdev_region(dev_t*,uint,fminor,unit count,char* name)
ret = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
if(ret < 0)
{
// at time kernel functions return negativ there is error
printk(KERN_ALERT "soliddevice: failed to allocate major");
return ret;//propgate error
}

major_number = MAJOR(dev_num);//extract the major number and stor in our variable (macro)
printk(KERN_INFO "soliddev : major number is %d",major_number);
printk(KERN_INFO "\t use \"mknod /dev/%s c %d 0\" for device file ",DEVICE_NAME,major_number); //demsg
//step 2)
mcdev = cdev_alloc();//create our cdev structure initialize our dev
mcdev->ops = &fops;//struct the file operations
mcdev->owner = THIS_MODULE;
//now that we created cdev we have to add it to the kernel 
//int cdev_add(struct cdev* dev,dev,dev_t num,unsigned int count)
ret = cdev_add(mcdev,dev_num,1);
if(ret < 0)
{
printk(KERN_ALERT "soliddevcode = unable to add dev to kernel ");
return ret;
}
	
//4) initialize our semaphore 
sema_init(&virtual_device.sem,1);//intial value of one 

return 0;
}
static void driver_exit(void)
{
 //5) unregister everything in reverse 
 //a)
cdev_del(mcdev);
//b)
unregister_chrdev_region(dev_num,1);
printk(KERN_ALERT "soliddevice : unloaded module");
}//end of exit function 

//inform the kernel where to start and stop with module/driver
module_init(driver_entry);
module_exit(driver_exit);
