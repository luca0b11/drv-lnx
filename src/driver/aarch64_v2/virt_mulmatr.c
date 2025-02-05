#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

// Define memory addresses for matrices
#define MATR_A_START        0x000
#define MATR_A_END	        0x190 	// 100 elements (0x000 + 100 * 4 bytes)
#define MATR_B_START        0x200
#define MATR_B_END	        0x228 	// 10 elements (0x210 + 10 * 4 bytes)
#define	MATR_C_START        0x300
#define	MATR_C_END	        0x328	// 10 elements (0x300 + 10 * 4 bytes)

// Define control register flags
#define CONTROL_REG         0x400   // Address of the control register
#define BIT_C_ENABLE        BIT(0)  // Enable the device
#define BIT_C_END_OP_IRQ_EN BIT(1)  // Enable IRQ at end of operation
#define BIT_C_START_OP      BIT(2)  // Start the operation
#define BIT_C_RESET_STAT    BIT(3)  // Reset the status (includes IRQ reset)
#define DEFAULT_CTRL_REG    0x01    // Default control register value

// Define matrix size register
#define SIZE_REG            0x410   // Address of the matrix size register
#define DEFAULT_SIZE_REG    0x03    // Default matrix size

// Status register
#define STATUS_REG	        0x420   // Address of the status register
#define BIT_S_OP_STARTED    BIT(0)  // Flag indicating operation has started
#define BIT_S_OP_ENDED      BIT(1)  // Flag indicating operation has ended

// Device ID register
#define ID_REG              0x430   // Address of the ID register

#define MAX_SIZE            10      // Maximum size for the flat matrices (10 x 1 or 1 x 10)
#define MAX_SIZE_QUAD       100     // Max size for the square matrix (10 x 10)


#define DEVICE_NAME "mulmatr_core" /* Dev name as it appears in /proc/devices   */

// IOCTL command definitions for interacting with the device
#define RD_ID               _IOR('a','b',int32_t*)      // Read device ID
#define RD_STATUS           _IOW('a','c',int32_t*)      // Read device status

// Control IOCTL commands for device control and interrupt handling
#define CTRL_ENABLE_DEV     _IOR('a','d',int32_t*)      // Enable the device
#define CTRL_DISABLE_DEV    _IOR('a','e',int32_t*)      // Disable the device
#define CTRL_ENABLE_IRQ     _IOR('a','f',int32_t*)      // Enable interrupt
#define CTRL_DISABLE_IRQ    _IOR('a','g',int32_t*)      // Disable interrupt
#define CTRL_START_OP       _IOR('a','h',int32_t*)      // Start a matrix operation
#define CTRL_RESET_STAT     _IOR('a','i',int32_t*)      // Reset device status

// Matrix IOCTL commands for reading and writing matrix data and size
#define RD_SIZE             _IOR('a','j',int32_t*)      // Read the size of matrices
#define WR_SIZE             _IOR('a','k',int32_t*)      // Write the size of matrices
#define RD_MATRA            _IOR('a','l',int32_t*)      // Read data from matrix A
#define WR_MATRA            _IOR('a','m',int32_t*)      // Write data to matrix A
#define RD_MATRB            _IOR('a','n',int32_t*)      // Read data from matrix B
#define WR_MATRB            _IOR('a','o',int32_t*)      // Write data to matrix B
#define RD_MATRC            _IOR('a','p',int32_t*)      // Read data from matrix C

static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

struct virt_mulmatr {
    struct device *dev;     
    void __iomem *base;
};

static int major;
static long base_address;

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);  // Track if device is currently open
static struct class *cls;                              

// File operations structure, specifying functions for file operations
static struct file_operations dev_fops = {
    .open =  device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

// Function to handle opening the device file
static int device_open(struct inode *inode, struct file *file)
{
    // Ensure device is not already open; returns -EBUSY if in use
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    try_module_get(THIS_MODULE);
    printk(KERN_DEBUG "KERNEL mmc: Open executed\n");
    pr_info("KERNEL mmc: Open executed\n");
    return 0;
}

// Function to handle closing the device file
static int device_release(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, CDEV_NOT_USED); // Reset device open state
    
    module_put(THIS_MODULE); 
    printk(KERN_DEBUG "KERNEL mmc: Release executed\n\n");
    pr_info("KERNEL mmc: Release executed\n");
    return 0;
}

// IOCTL handler function to process custom commands sent to device
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    u32 val;
    u32 *p_vals;
    u32 size;
    int i;
    long reg_base;
    
    switch(cmd) {

            case RD_ID:
                // Read device ID from ID_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_ID data\n");
                pr_info("KERNEL mmc: ioctl RD_ID data\n");
                val = (u32)readl_relaxed(base_address + ID_REG);        // Read the device ID
                // Copy the device ID value to user space
                if( copy_to_user((int32_t*) arg, &val, sizeof(val)) )
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_to_user ERR!\n");
                    pr_err("KERNEL mmc: copy_to_user ERR!\n");
                }
                break;

            case RD_STATUS:
                // Read current device status from STATUS_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_SIZE data\n");
                pr_info("KERNEL mmc: ioctl RD_SIZE data\n");
                val = (u32)readl_relaxed(base_address + STATUS_REG);    // Read the status
                // Copy the status value to user space
                if( copy_to_user((int32_t*) arg, &val, sizeof(val)) )
                {   
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_to_user ERR!\n");
                    pr_err("KERNEL mmc: copy_to_user ERR!\n");
                }
                break;

            case CTRL_ENABLE_DEV:
                // Enable device by setting BIT_C_ENABLE in CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_ENABLE_DEV data\n");
                pr_info("KERNEL mmc: ioctl CTRL_ENABLE_DEV data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val | BIT_C_ENABLE;                               // Set enable bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case CTRL_DISABLE_DEV:
                // Disable device by clearing BIT_C_ENABLE in CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_DISABLE_DEV data\n");
                pr_info("KERNEL mmc: ioctl CTRL_DISABLE_DEV data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val & (~BIT_C_ENABLE);                            // Clear enable bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case CTRL_ENABLE_IRQ:
                // Enable interrupts by setting the BIT_C_END_OP_IRQ_EN in the CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_ENABLE_IRQ data\n");
                pr_info("KERNEL mmc: ioctl CTRL_ENABLE_IRQ data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val | BIT_C_END_OP_IRQ_EN;                        // Set interrupt enable bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case CTRL_DISABLE_IRQ:
                // Disable interrupts by clearing the BIT_C_END_OP_IRQ_EN in the CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_DISABLE_IRQ data\n");
                pr_info("KERNEL mmc: ioctl CTRL_DISABLE_IRQ data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val & (~BIT_C_END_OP_IRQ_EN);                     // Clear interrupt enable bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case CTRL_START_OP:
                // Start the operation by setting the BIT_C_START_OP in the CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_START_OP data\n");
                pr_info("KERNEL mmc: ioctl CTRL_START_OP data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val | BIT_C_START_OP;                             // Set the start operation bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case CTRL_RESET_STAT:
                // Reset the status by setting the BIT_C_RESET_STAT in the CONTROL_REG
                printk(KERN_DEBUG "KERNEL mmc: ioctl CTRL_RESET_STAT data\n");
                pr_info("KERNEL mmc: ioctl CTRL_RESET_STAT data\n");
                val = (u32)readl_relaxed(base_address + CONTROL_REG);   // Read control register
                val = val | BIT_C_RESET_STAT;                           // Set the reset status bit
                writel_relaxed(val, base_address + CONTROL_REG);        // Write updated value to control register
                break;

            case RD_SIZE:
                // Read the size of the matrix from the SIZE_REG register
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_SIZE data\n");
                pr_info("KERNEL mmc: ioctl RD_SIZE data\n");
                val = (u32)readl_relaxed(base_address + SIZE_REG);      // Read size value
                // Copy the size value to user space
                if( copy_to_user((int32_t*) arg, &val, sizeof(val)) )
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_to_user ERR!\n");
                    pr_err("KERNEL mmc: copy_to_user ERR!\n");
                }
                break;

            case WR_SIZE:
                // Write a new size value to the SIZE_REG register
                printk(KERN_DEBUG "KERNEL mmc: ioctl WR_SIZE data\n");
                pr_info("KERNEL mmc: ioctl RD_SIZE data\n");
                // Copy the new size value from user space
                if(copy_from_user(&val ,(int32_t*) arg, sizeof(val)) )
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_from_user ERR!\n");
                    pr_err("KERNEL mmc: copy_from_user ERR!\n");
                }
                else
                {
                    // Check if the new size is within limits
                    if(val > MAX_SIZE)
                    {
                        // Log error if too large size
                        printk(KERN_DEBUG "KERNEL mmc: tryng to write %d on size, too big, ERR!\n", val);
                        pr_info("KERNEL mmc: tryng to write %d on size, too big, ERR!\n", val);
                    }
                    else
                        writel_relaxed(val, base_address + SIZE_REG);   // Write the new size to SIZE_REG
                }
                break;

            case RD_MATRA:
                // Read data from matrix A
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_MATRA data\n");
                pr_info("KERNEL mmc: ioctl RD_MATRA data\n");
                size = (u32)readl_relaxed(base_address + SIZE_REG);     // Read the size
                size = size*size;   // Calculate total number of elements in the square matrix A

                // Memory allocation for the matrix A
                p_vals = kmalloc(sizeof(u32)*size, GFP_KERNEL);
                if (!p_vals) {
                    // Log error if allocation fails
                    printk(KERN_ERR "KERNEL mmc: Error allocating dynamic mem\n");
                    pr_info("KERNEL mmc: Error allocating dynamic mem\n");
                    return -EFAULT;
                }

                reg_base = base_address + MATR_A_START;     // Calculate base address for matrix A
                // Read each element of matrix A
                for (i = 0; i < size; i++) 
                    p_vals[i] = readl_relaxed(reg_base + (i * 4));  
                
                 // Copy the matrix data to user space
                if(copy_to_user((int32_t*) arg, p_vals, sizeof(u32) * size))
                {   
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_to_user ERR!\n");
                    pr_err("KERNEL mmc: copy_to_user ERR!\n");
                }
                kfree(p_vals);      // Free the allocated memory
                break;

            case WR_MATRA:
                // Write data to matrix A
                printk(KERN_DEBUG "KERNEL mmc: ioctl WR_MATRA data\n");
                pr_info("KERNEL mmc: ioctl WR_MATRA data\n");
                
                size = (int)readl_relaxed(base_address + SIZE_REG);     // Read the size
                size = size*size;       // Calculate total number of elements in the square matrix A
                
                // Memory allocation for the matrix A
                p_vals = kmalloc(sizeof(u32)*size, GFP_KERNEL);
                if (!p_vals) {
                    // Log error if allocation fails
                    printk(KERN_ERR "KERNEL mmc: Error allocating dynamic mem\n");
                    pr_info("KERNEL mmc: Error allocating dynamic mem\n");
                    return -EFAULT;
                }

                // Copy the new matrix from user space
                if(copy_from_user(p_vals, (int32_t*) arg, sizeof(u32) * size))
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_from_user ERR!\n");
                    pr_err("KERNEL mmc: copy_from_user ERR!\n");
                }
                else
                {
                    reg_base = base_address + MATR_A_START;     // Calculate base address for matrix A
                    // Write each element of matrix A
                    for (i = 0; i < size; i++) 
                        writel_relaxed(p_vals[i], reg_base + (i * 4));
                }

                kfree(p_vals);      // Free the allocated memory
                break;

            case RD_MATRB:
                // Read data from matrix B
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_MATRB data\n");
                pr_info("KERNEL mmc: ioctl RD_MATRB data\n");
                size = (u32)readl_relaxed(base_address + SIZE_REG);     // Read the size
                size = size;  // Flat matrix

                // Memory allocation for the matrix B
                p_vals = kmalloc(sizeof(u32)*size, GFP_KERNEL);
                if (!p_vals) {
                    // Log error if allocation fails
                    printk(KERN_ERR "KERNEL mmc: Error allocating dynamic mem\n");
                    pr_info("KERNEL mmc: Error allocating dynamic mem\n");
                    return -EFAULT;
                }

                reg_base = base_address + MATR_B_START;     // Calculate base address for matrix B
                // Read each element of matrix B
                for (i = 0; i < size; i++) 
                    p_vals[i] = readl_relaxed(reg_base + (i * 4));

                // Copy the matrix data to user space
                if(copy_to_user((int32_t*) arg, p_vals, sizeof(u32) * size))
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_from_user ERR!\n");
                    pr_err("KERNEL mmc: copy_from_user ERR!\n");
                }

                kfree(p_vals);      // Free the allocated memory
                break;
                
            case WR_MATRB:
                // Write data to matrix B
                printk(KERN_DEBUG "KERNEL mmc: ioctl WR_MATRB data\n");
                pr_info("KERNEL mmc: ioctl WR_MATRB data\n");
                size = (u32)readl_relaxed(base_address + SIZE_REG);     // Read the size
                size = size;    // Flat matrix

                // Memory allocation for the matrix B
                p_vals = kmalloc(sizeof(u32)*size, GFP_KERNEL);
                if (!p_vals) {
                    // Log error if allocation fails
                    printk(KERN_ERR "KERNEL mmc: Error allocating dynamic mem\n");
                    pr_info("KERNEL mmc: Error allocating dynamic mem\n");
                    return -EFAULT;
                }

                // Copy the new matrix from user space
                if(copy_from_user(p_vals, (int32_t*) arg, sizeof(u32) * size))
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_from_user ERR!\n");
                    pr_err("KERNEL mmc: copy_from_user ERR!\n");
                }
                else
                {
                    reg_base = base_address + MATR_B_START;     // Calculate base address for matrix B
                    // Write each element of matrix B
                    for (i = 0; i < size; i++) 
                        writel_relaxed(p_vals[i], reg_base + (i * 4));
                }
                kfree(p_vals);      // Free the allocated memory
                break;

            case RD_MATRC:
                // Read data from matrix C
                printk(KERN_DEBUG "KERNEL mmc: ioctl RD_MATRC data\n");
                pr_info("KERNEL mmc: ioctl RD_MATRC data\n");
                size = (u32)readl_relaxed(base_address + SIZE_REG);     // Read the size
                size = size;

                // Memory allocation for the matrix C
                p_vals = kmalloc(sizeof(u32)*size, GFP_KERNEL);
                if (!p_vals) {
                    // Log error if allocation fails
                    printk(KERN_ERR "KERNEL mmc: Error allocating dynamic mem\n");
                    pr_info("KERNEL mmc: Error allocating dynamic mem\n");
                    return -EFAULT;
                }

                reg_base = base_address + MATR_C_START;     // Calculate base address for matrix C
                // Read each element of matrix C
                for (i = 0; i < size; i++) 
                    p_vals[i] = readl_relaxed(reg_base + (i * 4));

                // Copy the matrix data to user space
                if(copy_to_user((int32_t*) arg, p_vals, sizeof(u32) * size))
                {
                    // Log error if copy fails
                    printk(KERN_ERR "KERNEL mmc: copy_to_user ERR!\n");
                    pr_err("KERNEL mmc: copy_to_user ERR!\n");
                }
                kfree(p_vals);      // Free the allocated memory
                break;                       

            default:
            // Invalid IOCTL command
                    printk(KERN_DEBUG "KERNEL mmc: Error calling IOCTL cmd function\n");
                    pr_info("KERNEL mmc: Error calling IOCTL cmd function\n");
                    break;
    }
    return 0;

}

// Initialize the device
static void vm_init(struct virt_mulmatr *vm)
{
    // Register the character device with the system, requesting a dynamic major number
    major = register_chrdev(0, DEVICE_NAME, &dev_fops);

    printk(KERN_DEBUG "KERNEL mmc: vm_init\n");
    pr_info("KERNEL mmc: vm_init\n");

    // Check if the device was registered successfully
    if (major < 0) {
        // Log error if registration fail
        printk(KERN_DEBUG "KERNEL mmc: Registering mulmatr device failed\n");
        pr_alert("KERNEL mmc: Registering mulmatr device failed with %d\n", major);
        return;
    }

    // Log the assigned major number for the device
    printk(KERN_DEBUG "KERNEL mmc: I was assigned major number %d.\n", major);
    pr_info("KERNEL mmc: I was assigned major number %d.\n", major);

    //Linux is 6.6.32
    cls = class_create(DEVICE_NAME);

    // Create the device in the /dev directory, using the major number assigned
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    // Log the creation of the device
    printk(KERN_DEBUG "KERNEL mmc: Device created on /dev/%s\n", DEVICE_NAME);
    pr_info("KERNEL mmc: Device created on /dev/%s\n", DEVICE_NAME);

    // Store the base address for the hardware registers
    base_address = vm->base;

    // Log the base address used for accessing device registers
    printk(KERN_DEBUG "KERNEL mmc: Base Address: %x\n", base_address);
    pr_info("KERNEL mmc: Base Address: %x\n", base_address);

    // Write the default control register value to the device's control register
    writel_relaxed(DEFAULT_CTRL_REG, vm->base + CONTROL_REG);

    return;
}

// Interrupt handler for the device
static irqreturn_t vm_irq_handler(int irq, void *data)
{
    struct virt_mulmatr *vm = (struct virt_mulmatr *)data;
    u32 status;

    // Log information indicating that the IRQ handler has been raised
    printk(KERN_DEBUG "KERNEL mmc: Raised IRQ handler\n");
    pr_info("KERNEL mmc: Raised IRQ handler\n");

    status = readl_relaxed(vm->base + STATUS_REG);  // Read the current device status from the status register

    // Check if the operation has ended
    if (status & BIT_S_OP_ENDED)
    {   
        // Log information indicating that the operation has been ended
        printk(KERN_DEBUG "KERNEL mmc: IRQ Operation Terminated\n");
        pr_info("KERNEL mmc: IRQ Operation Terminated\n");
    }

    return IRQ_HANDLED;
}

// Probe function called when the device is detected
static int vm_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    struct virt_mulmatr *vm;
    int ret;

    // Log information indicating the probe function has been called
    printk(KERN_DEBUG "KERNEL mmc: vm_probe\n");
    pr_info("KERNEL mmc: vm_probe\n");

    // Get the memory resource from the device tree
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENOMEM;

    // Allocate memory for the virt_mulmatr structure
    vm = devm_kzalloc(dev, sizeof(*vm), GFP_KERNEL);
    if (!vm)
        return -ENOMEM;

    // Set the device pointer and map the device memory into the kernel space
    vm->dev = dev;
    vm->base = devm_ioremap(dev, res->start, resource_size(res));
    if (!vm->base)
        return -EINVAL;

    // Get the IRQ resource from the device tree
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res) {
        // Request the IRQ and associate it with the IRQ handler
        ret = devm_request_irq(dev, res->start, vm_irq_handler,
                       IRQF_TRIGGER_HIGH, "vm_irq", vm);
        if (ret)
            return ret;
    }

    // Store the device data in the platform device structure
    platform_set_drvdata(pdev, vm);

    // Call the initialization function for the device
    vm_init(vm);

    return 0;
}

// Remove function called when the device is removed
static int vm_remove(struct platform_device *pdev)
{
    // Log information indicating the device driver is being detached
    printk(KERN_DEBUG "KERNEL mmc: detaching device driver\n");
    pr_info("KERNEL mmc: detaching device driver\n");

    return 0;
}

static const struct of_device_id vm_of_match[] = {
    { .compatible = "virt-mulmatr", },
    { }
};
MODULE_DEVICE_TABLE(of, vm_of_match);

static struct platform_driver vm_driver = {
    .probe = vm_probe,
    .remove = vm_remove,
    .driver = {
        .name = "virt_mulmatr",
        .of_match_table = vm_of_match,
    },
};
module_platform_driver(vm_driver);  // Register the platform driver with the kernel

// Module information for kernel module
MODULE_DESCRIPTION("MUL MATR Kernel Module");
MODULE_AUTHOR("OS - Group8");
MODULE_LICENSE("GPL");