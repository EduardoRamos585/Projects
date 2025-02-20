// QE IP Example
// Serial Driver (serial_driver.c)
// Eduardo Ramos

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: Xilinx XUP Blackboard

// Hardware configuration:
//
// AXI4-Lite interface
//   Mapped to offset of 0x10000
//
//  Serial Driver

// Load kernel module with insmod qe_driver.ko [param=___]

//-----------------------------------------------------------------------------

#include <linux/kernel.h>     // kstrtouint
#include <linux/module.h>     // MODULE_ macros
#include <linux/init.h>       // __init
#include <linux/kobject.h>    // kobject, kobject_atribute,
                              // kobject_create_and_add, kobject_put
#include <asm/io.h>           // iowrite, ioread, ioremap_nocache (platform specific)
#include "../address_map.h"   // overall memory map
#include "serial_regs.h"          // register offsets in QE IP

//-----------------------------------------------------------------------------
// Kernel module information
//-----------------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo Ramos");
MODULE_DESCRIPTION("Serial IP Driver");

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

static unsigned int *base = NULL;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void enableSerial(void)
{
    unsigned int value = ioread(base + OFS_CONTROL);
    iowrite32(value | ((1 << 3) | (1 << 4)) , base + OFS_CONTROL);
}

void disableSerial(void)
{
    unsigned int value = ioread(base + OFS_CONTROL);
    iowrite32(value & ~((1 << 3) | (1 << 4)), base + OFS_CONTROL);
}

bool isSerialEnabled(void)
{
    bool ok;
    unsigned int value = ioread(base + OFS_CONTROL);
    ok = (value & 0x30);
    return ok;
}

void setBaudRate(int BRD)
{
    int32_t IBRD;
    int32_t FBRD;
    int32_t Data;


    int32_t divisorTimes128 = (int32_t) (((100 * 1e6) * 16) / BRD);   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    divisorTimes128 += 1;                               // add 1/128 to allow rounding
    IBRD = divisorTimes128 >> 8;                // set integer value to floor(r)
    FBRD = ((divisorTimes128)) & 0xFF;       // set fractional value to round(fract(r)*64)



    Data = (IBRD << 8) | (FBRD);
    unsigned int = ioread(base + OFS_BRD);
    iowrite32(value | Data, base + OFS_BRD);

}

int32_t getBaudRate(void)
{
    return ioread(base + OFS_BRD);
}


void setWordSize(uint8_t bits)
{
    unsigned int value = ioread(base + OFS_CONTROL);
    iowrite(value & ~(0x3), base + OFS_CONTROL ); // Clear any previous size bits
    iowrite(value | bits, base + OFS_CONTROL);
}

uint8_t getWordSize(void)
{
    return (ioread(base + OFS_CONTROL) & 0x3);
}

void setParityBits(uint8_t bits)
{
    unsigned int value = ioread(base + OFS_CONTROL);
    iowrite(value & ~(0x3 << 2), base + OFS_CONTROL); // Clear any previous parity bits
    iowrite(value | (bits << 2), base + OFS_CONTROL);
}

uint8_t getParityBits(void)
{
    return (ioread(base + OFS_CONTROL) & 0xC);
}

void writeTxData(int8_t data)
{
    iowrite(data,base + OFS_DATA);
}

int32_t getRxData(void)
{
    return (ioread(base + OFS_DATA));
}


//-----------------------------------------------------------------------------
// Kernel Objects
//-----------------------------------------------------------------------------





// Enable
static bool enable = 0;
module_param(enable0, bool, S_IRUGO);
MODULE_PARM_DESC(enable0, " Enable Serial");

static ssize_t enableStore(struct kobject *kobj, struct kobj_attribute *attr, const char *buffer, size_t count)
{
    if (strncmp(buffer, "true", strlen("true")) == 0)
    {
        enableSerial();
        enable0 = true;
    }
    else
        if (strncmp(buffer, "false", strlen("false")) == 0)
        {
            disableSerial();
            enable0 = false;
        }
    return count;
}

static ssize_t enableShow(struct kobject *kobj, struct kobj_attribute *attr, char *buffer)
{

    enable = isSerialEnabled();
    if (enable)
        strcpy(buffer, "true");
    else
        strcpy(buffer, "false");

    return strlen(buffer);
}

static struct kobj_attribute enableAttr = __ATTR(enable, S_IRUGO | S_IWUSR, enableShow, enableStore);

//baud_rate
static int baud_rate = 0;
module_param(baud_rate, int, S_IRUGO);
MODULE_PARM_DESC(baud_rate, " BaudRate of TX");
static ssize_t baud_rateStore(struct kobject *kobj, struct kobj_attribute *attr, const char *buffer, size_t count)
{
    int result = kstrtouint(buffer, 0, &baud_rate);
    if (result == 0)
        setBaudRate(baud_rate);
    return count;
}
static ssize_t baud_rateShow(struct kobject *kobj, struct kobj_attribute *attr, char *buffer)
{
    baud_rate = getBaudRate();
    return sprintf(buffer, "%d\n", baud_rate);
}
SPAN_IN_BYTES
static struct kobj_attribute baud_rateATTR = __ATTR(baud_rate, 0664, baud_rateShow , baud_rateStore);

//Size bits
static uint8_t word_size = 0;
module_param(word_size, uint8_t. S_IRUGO);
MODULE_PARM_DESC(word_size, "Size of data");
static ssize_t word_sizeStore(struct kobject *kobj, struct kobj_attribute *attr, const char *buffer, size_t count)
{
    int result = kstrtouint(buffer, 0, &word_size);
    if (result == 0)
        setWordSize(word_size);
    return count;
}
static ssize_t word_sizeShow(struct kobject *kobj, struct kobj_attribute *attr, char *buffer)
{
    word_size = getWordSize();
    return sprintf(buffer, "%d\n", word_size);
}
static struct kobj_attribute word_sizeATTR = __ATTR(word_size, 0664, word_sizeShow , word_sizeStore);

//parity_mode
static uint8_t parity_mode = 0;
module_param(parity_mode, uint8_t, S_IRUGO);
MODULE_PARM_DESC(parity_mode, "Parity of data");
static ssize_t parity_modeStore(struct kobject *kobj, struct kobj_attribute *attr, const char *buffer, size_t count)
{
    int result = kstrtouint(buffer, 0, &parity_mode);
    if (result == 0)
        setParityBits(parity_mode);
    return count;
}
static ssize_t parity_modeShow(struct kobject *kobj, struct kobj_attribute *attr, char *buffer)
{
    parity_mode = getParityBits();
    return sprintf(buffer, "%d\n", parity_mode);
}
static struct kobj_attribute parity_modeATTR = __ATTR(parity_mode, 0664, parity_modeShow , parity_modeStore);

//tx_data
static int8_t tx_data = 0;
module_param(tx_data, int8_t, S_IRUGO);
MODULE_PARM_DESC(tx_data, "Data to Transmit");
static ssize_t tx_dataStore(struct kobject *kobj, struct kobj_attribute *attr, const char *buffer, size_t count)
{
    int result = kstrtouint(buffer, 0, &tx_data);
    if (result == 0)
        writeTxData(tx_data);
    return count;
}
static struct kobj_attribute tx_dataATTR = __ATTR(tx_data, 0222, NULL , parity_modeStore);

//rx_data
static int32_t rx_data = 0;
module_param(rx_data, int32_t, S_IRUGO);
MODULE_PARM_DESC(rx_data, "Data Recieved");
static ssize_t parity_modeShow(struct kobject *kobj, struct kobj_attribute *attr, char *buffer)
{
    rx_data = getRxData();
    if(rx_data != 0)
    {
      return sprintf(buffer, "%d\n", parity_mode);
    }
    else
    {
      return sprintf(buffer, "%d\n", -1);
    }

}

static struct kobj_attribute rx_dataATTR = __ATTR(rx_data, 0444, parity_modeShow , NULL);

//Attributes

static struct attribute *attrs[] = {&enableAttr ,&baud_rateATTR.attr, &word_sizeATTR.attr, &parity_modeATTR.attr, &tx_dataATTR.attr, &rx_dataATTR.attr, NULL};

static struct attribute_group group0 =
{
    .name = "serial",
    .attrs = attrs
};

static struct kobject *kobj;

//-----------------------------------------------------------------------------
// Initialization and Exit
//-----------------------------------------------------------------------------


static int __init initialize_module(void)
{
    int result;

    printk(KERN_INFO "Serial driver: starting\n");

    // Create qe directory under /sys/kernel
    kobj = kobject_create_and_add("serial", NULL); //kernel_kobj);
    if (!kobj)
    {
        printk(KERN_ALERT "Serial driver: failed to create and add kobj\n");
        return -ENOENT;
    }

    // Create qe0 and qe1 groups
    result = sysfs_create_group(kobj, &group0);
    if (result !=0)
        return result;

    // Physical to virtual memory map to access gpio registers
    base = (unsigned int*)ioremap(AXI4_LITE_BASE + SERIAL_BASE_OFFSET,
                                          SPAN_IN_BYTES);
    if (base == NULL)
        return -ENODEV;

    printk(KERN_INFO "Serial driver: initialized\n");

    return 0;
}

static void __exit exit_module(void)
{
    kobject_put(kobj);
    printk(KERN_INFO "Serial driver: exit\n");
}

module_init(initialize_module);
module_exit(exit_module);












