#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/device.h>


#define DEVICE_NAME "adxl345"
#define CLASS_NAME  "adxl345_class"
#define ADXL345_ADDR 0x53


static int majorNumber;
static struct class* adxl345Class = NULL;
static struct device* adxl345Device = NULL;
static struct i2c_client *adxl345_client = NULL;
static struct cdev adxl345_cdev;


static int adxl345_read_reg(uint8_t reg, uint8_t *data) {
    struct i2c_msg msgs[2];
    msgs[0].addr  = adxl345_client->addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;


    msgs[1].addr  = adxl345_client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = 1;
    msgs[1].buf   = data;


    return i2c_transfer(adxl345_client->adapter, msgs, 2);
}


static int adxl345_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return i2c_master_send(adxl345_client, buf, 2);
}


static int adxl345_init_device(void) {
    // 設置 ADXL345 進入測量模式
    adxl345_write_reg(0x2D, 0x08);
    // 設置數據格式為完全分辨率，範圍為±16g
    adxl345_write_reg(0x31, 0x0B);
    return 0;
}


static int adxl345_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "ADXL345: Device has been opened\n");
    return 0;
}


static ssize_t adxl345_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int16_t data[3];
    uint8_t raw_data[6];


    adxl345_read_reg(0x32, &raw_data[0]);
    adxl345_read_reg(0x33, &raw_data[1]);
    adxl345_read_reg(0x34, &raw_data[2]);
    adxl345_read_reg(0x35, &raw_data[3]);
    adxl345_read_reg(0x36, &raw_data[4]);
    adxl345_read_reg(0x37, &raw_data[5]);


    data[0] = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    data[1] = (int16_t)((raw_data[3] << 8) | raw_data[2]);
    data[2] = (int16_t)((raw_data[5] << 8) | raw_data[4]);


    if (copy_to_user(buffer, data, sizeof(data))) {
        return -EFAULT;
    }


    return sizeof(data);
}


static int adxl345_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "ADXL345: Device successfully closed\n");
    return 0;
}


static struct file_operations fops = {
    .open = adxl345_open,
    .read = adxl345_read,
    .release = adxl345_release,
};






static int __init adxl345_init(void) {
    printk(KERN_INFO "ADXL345: Initializing the ADXL345 LKM\n");


    // 註冊字符設備
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "ADXL345 failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "ADXL345: registered correctly with major number %d\n", majorNumber);


    // 創建 class
    adxl345Class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(adxl345Class)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(adxl345Class);
    }
    printk(KERN_INFO "ADXL345: device class registered correctly\n");


    // 創建設備
    adxl345Device = device_create(adxl345Class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(adxl345Device)) {
        class_destroy(adxl345Class);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(adxl345Device);
    }


    // 初始化 I2C 連接
    struct i2c_adapter *adapter = i2c_get_adapter(1);
    if (!adapter) {
        printk(KERN_ALERT "Failed to get I2C adapter\n");
        device_destroy(adxl345Class, MKDEV(majorNumber, 0));
        class_destroy(adxl345Class);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return -ENODEV;
    }


    struct i2c_board_info board_info = {
        I2C_BOARD_INFO("adxl345", ADXL345_ADDR)
    };
    adxl345_client = i2c_new_client_device(adapter, &board_info);
    if (!adxl345_client) {
        printk(KERN_ALERT "Failed to create i2c client\n");
        i2c_put_adapter(adapter);
        device_destroy(adxl345Class, MKDEV(majorNumber, 0));
        class_destroy(adxl345Class);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return -ENODEV;
    }


    adxl345_init_device(); 


    return 0;
}


static void __exit adxl345_exit(void) {
    i2c_unregister_device(adxl345_client);
    device_destroy(adxl345Class, MKDEV(majorNumber, 0));
    class_unregister(adxl345Class);
    class_destroy(adxl345Class);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "ADXL345: Goodbye from the LKM!\n");
}


module_init(adxl345_init);
module_exit(adxl345_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ting");
MODULE_DESCRIPTION("ADXL345 accelerometer");
MODULE_VERSION("0.1");
