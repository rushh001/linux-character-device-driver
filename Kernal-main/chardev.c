/*
 * Character Device Driver with Read/Write/IOCTL Interface
 * Implements synchronization using mutex
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#define DEVICE_NAME "chardev"
#define CLASS_NAME  "chardev_class"
#define BUFFER_SIZE 1024
/* IOCTL commands */
#define IOCTL_RESET     _IO('c', 1)
#define IOCTL_GET_SIZE  _IOR('c', 2, int)
#define IOCTL_SET_FLAG  _IOW('c', 3, int)
#define IOCTL_GET_FLAG  _IOR('c', 4, int)
/* Device data structure */
struct chardev_data {
    struct cdev cdev;
    char buffer[BUFFER_SIZE];
    size_t buffer_size;
    int flag;
    struct mutex lock;
};

static dev_t dev_number;
static struct class *chardev_class = NULL;
static struct chardev_data *device_data = NULL;

/*
 * Device open function
 */
static int chardev_open(struct inode *inode, struct file *file)
{
    struct chardev_data *data = container_of(inode->i_cdev, struct chardev_data, cdev);
    file->private_data = data;
    
    pr_info("chardev: Device opened\n");
    return 0;
}

/*
 * Device close function
 */
static int chardev_release(struct inode *inode, struct file *file)
{
    pr_info("chardev: Device closed\n");
    return 0;
}

/*
 * Device read function
 */
static ssize_t chardev_read(struct file *file, char __user *user_buffer, 
                           size_t count, loff_t *offset)
{
    struct chardev_data *data = file->private_data;
    size_t to_read;
    ssize_t ret;

    if (mutex_lock_interruptible(&data->lock))
        return -ERESTARTSYS;

    /* Check if offset is beyond buffer */
    if (*offset >= data->buffer_size) {
        ret = 0;
        goto out;
    }

    /* Calculate bytes to read */
    to_read = min(count, data->buffer_size - (size_t)*offset);

    /* Copy data to user space */
    if (copy_to_user(user_buffer, data->buffer + *offset, to_read)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += to_read;
    ret = to_read;

    pr_info("chardev: Read %zu bytes from device\n", to_read);

out:
    mutex_unlock(&data->lock);
    return ret;
}

/*
 * Device write function
 */
static ssize_t chardev_write(struct file *file, const char __user *user_buffer,
                            size_t count, loff_t *offset)
{
    struct chardev_data *data = file->private_data;
    size_t to_write;
    ssize_t ret;

    if (mutex_lock_interruptible(&data->lock))
        return -ERESTARTSYS;

    /* Check if offset is beyond buffer */
    if (*offset >= BUFFER_SIZE) {
        ret = -ENOSPC;
        goto out;
    }

    /* Calculate bytes to write */
    to_write = min(count, BUFFER_SIZE - (size_t)*offset);

    /* Copy data from user space */
    if (copy_from_user(data->buffer + *offset, user_buffer, to_write)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += to_write;
    
    /* Update buffer size if we wrote beyond current size */
    if (*offset > data->buffer_size)
        data->buffer_size = *offset;

    ret = to_write;

    pr_info("chardev: Wrote %zu bytes to device\n", to_write);

out:
    mutex_unlock(&data->lock);
    return ret;
}

/*
 * Device ioctl function
 */
static long chardev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct chardev_data *data = file->private_data;
    int ret = 0;
    int value;

    if (mutex_lock_interruptible(&data->lock))
        return -ERESTARTSYS;

    switch (cmd) {
        case IOCTL_RESET:
            /* Reset buffer */
            memset(data->buffer, 0, BUFFER_SIZE);
            data->buffer_size = 0;
            data->flag = 0;
            pr_info("chardev: IOCTL - Buffer reset\n");
            break;

        case IOCTL_GET_SIZE:
            /* Get current buffer size */
            value = data->buffer_size;
            if (copy_to_user((int __user *)arg, &value, sizeof(int))) {
                ret = -EFAULT;
            } else {
                pr_info("chardev: IOCTL - Get size: %d\n", value);
            }
            break;

        case IOCTL_SET_FLAG:
            /* Set flag value */
            if (copy_from_user(&value, (int __user *)arg, sizeof(int))) {
                ret = -EFAULT;
            } else {
                data->flag = value;
                pr_info("chardev: IOCTL - Set flag: %d\n", value);
            }
            break;

        case IOCTL_GET_FLAG:
            /* Get flag value */
            value = data->flag;
            if (copy_to_user((int __user *)arg, &value, sizeof(int))) {
                ret = -EFAULT;
            } else {
                pr_info("chardev: IOCTL - Get flag: %d\n", value);
            }
            break;

        default:
            pr_err("chardev: Invalid IOCTL command\n");
            ret = -EINVAL;
            break;
    }

    mutex_unlock(&data->lock);
    return ret;
}

/*
 * File operations structure
 */
static struct file_operations chardev_fops = {
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .read = chardev_read,
    .write = chardev_write,
    .unlocked_ioctl = chardev_ioctl,
};

/*
 * Module initialization function
 */
static int __init chardev_init(void)
{
    int ret;
    struct device *device;

    pr_info("chardev: Initializing character device driver\n");

    /* Allocate device data */
    device_data = kzalloc(sizeof(struct chardev_data), GFP_KERNEL);
    if (!device_data) {
        pr_err("chardev: Failed to allocate memory\n");
        return -ENOMEM;
    }

    /* Initialize mutex */
    mutex_init(&device_data->lock);

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("chardev: Failed to allocate device number\n");
        goto fail_alloc;
    }

    pr_info("chardev: Allocated device number - Major: %d, Minor: %d\n",
            MAJOR(dev_number), MINOR(dev_number));

    /* Create device class */
    chardev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(chardev_class)) {
        pr_err("chardev: Failed to create device class\n");
        ret = PTR_ERR(chardev_class);
        goto fail_class;
    }

    /* Initialize and add character device */
    cdev_init(&device_data->cdev, &chardev_fops);
    device_data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&device_data->cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("chardev: Failed to add character device\n");
        goto fail_cdev;
    }

    /* Create device file */
    device = device_create(chardev_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        pr_err("chardev: Failed to create device file\n");
        ret = PTR_ERR(device);
        goto fail_device;
    }

    pr_info("chardev: Character device driver loaded successfully\n");
    pr_info("chardev: Device node created at /dev/%s\n", DEVICE_NAME);

    return 0;

fail_device:
    cdev_del(&device_data->cdev);
fail_cdev:
    class_destroy(chardev_class);
fail_class:
    unregister_chrdev_region(dev_number, 1);
fail_alloc:
    kfree(device_data);
    return ret;
}

/*
 * Module cleanup function
 */
static void __exit chardev_exit(void)
{
    pr_info("chardev: Unloading character device driver\n");

    /* Destroy device */
    device_destroy(chardev_class, dev_number);
    
    /* Delete character device */
    cdev_del(&device_data->cdev);
    
    /* Destroy class */
    class_destroy(chardev_class);
    
    /* Unregister device number */
    unregister_chrdev_region(dev_number, 1);
    
    /* Free device data */
    kfree(device_data);

    pr_info("chardev: Character device driver unloaded successfully\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Character Device Driver with Read/Write/IOCTL Interface and Mutex Synchronization");
MODULE_VERSION("1.0");
