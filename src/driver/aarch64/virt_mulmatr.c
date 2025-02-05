#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#define MATR_A_START        0x000
#define MATR_A_END	        0x190 	//0x000 + 100 * 4
#define MATR_B_START        0x200
#define MATR_B_END	        0x228 	//0x210 + 10 * 4
#define	MATR_C_START        0x300
#define	MATR_C_END	        0x328	//0x300 + 10 * 4

#define CONTROL_REG         0x400
#define BIT_C_ENABLE        BIT(0)
#define BIT_C_END_OP_IRQ_EN BIT(1)
#define BIT_C_START_OP      BIT(2)
#define BIT_C_RESET_STAT    BIT(3)  //also reset irq
#define DEFAULT_CTRL_REG    0x01

#define SIZE_REG            0x410
#define DEFAULT_SIZE_REG    0x03

#define STATUS_REG	        0x420
#define BIT_S_OP_STARTED    BIT(0)
#define BIT_S_OP_ENDED      BIT(1)

#define ID_REG              0x430
#define CHIP_ID             0xc1a0

#define MAX_SIZE            10
#define MAX_SIZE_QUAD       100

struct virt_mulmatr {
    struct device *dev;
    void __iomem *base;
};

// Implementation of strtok (at kernel level it does not exist)
char *strtok_custom(char *str, char *delim) {
    static char *saved_str;
    char *token;
    
    // Se str è non NULL, iniziamo una nuova tokenizzazione
    if (str != NULL) {
        saved_str = str;
    }

    // Se la stringa salvata è NULL, non ci sono più token
    if (saved_str == NULL) {
        return NULL;
    }

    // Salta i delimitatori iniziali
    token = saved_str;
    saved_str = strpbrk(token, delim);  // Cerca il prossimo delimitatore

    if (saved_str != NULL) {
        *saved_str = '\0';  // Sostituisci il delimitatore con il terminatore NULL
        saved_str++;        // Avanza al carattere successivo
    }

    return token;
}

int parse_hex_string(char *str, int ssize, uint32_t *dest, int dmaxsize) {
    int count = 0; // Contatore degli elementi salvati
    char *endptr;
    char *token;
    int ret;
    //uint32_t value;
    unsigned long val;


    // Usiamo strtok per dividere la stringa per virgole
    token = strtok_custom(str, ",");
    while (token != NULL && count < dmaxsize) {
        ret = kstrtoul(token, 0, &val);  // Base 0 per gestire sia esadecimali che decimali

        // Controlliamo se la conversione è andata a buon fine
        if (ret != 0 || endptr == token) {
            return -EINVAL;    // Restituiamo errore per input non valido
        }

        dest[count++] = val;
        token = strtok_custom(NULL, ",");
    }

    return count;
}

static ssize_t vf_show_control(struct device *dev,
               struct device_attribute *attr, char *buf)
{
    printk(KERN_DEBUG "KERNEL: vf_show_control\n");
    
    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + CONTROL_REG);

    return scnprintf(buf, PAGE_SIZE, "0x%.x\n", val);
}

static ssize_t vf_store_control(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    printk(KERN_DEBUG "KERNEL: vf_store_control\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    writel_relaxed(val, vf->base + CONTROL_REG);

    return len;
}

static ssize_t vf_show_size(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
    printk(KERN_DEBUG "KERNEL: vf_show_size\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + SIZE_REG);

    return scnprintf(buf, PAGE_SIZE, "0x%.x\n", val);
}

static ssize_t vf_store_size(struct device *dev,
                            struct device_attribute *attr,
                            const char *buf, size_t len)
{
    printk(KERN_DEBUG "KERNEL: vf_store_size\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    val = (val <= MAX_SIZE) ? val : MAX_SIZE;

    writel_relaxed(val, vf->base + SIZE_REG);

    return len;
}

static ssize_t vf_show_status(struct device *dev,
              struct device_attribute *attr, char *buf)
{
    printk(KERN_DEBUG "KERNEL: vf_show_status\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + STATUS_REG);

    return scnprintf(buf, PAGE_SIZE, "0x%.x\n", val);
}

static ssize_t vf_show_id(struct device *dev,
              struct device_attribute *attr, char *buf)
{
    printk(KERN_DEBUG "KERNEL: vf_show_id\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + ID_REG);

    return scnprintf(buf, PAGE_SIZE, "0x%.x\n", val);
}

static ssize_t matrA_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
    //dmesg | grep "KERNEL:" To read kernel log messages
    printk(KERN_DEBUG "KERNEL: matrA_show\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    //u32 *data; 
    int i;
    void __iomem *reg_base;
    int written_chars = 0;

    int len_matr = (int)readl_relaxed(vf->base + SIZE_REG);
    int len_matr_quad = len_matr * len_matr;

    reg_base = vf->base + MATR_A_START;
    for (i = 0; i < len_matr_quad; i++) {
        if((i + 1) % len_matr == 0) // To print the \n only at end of line
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x\n", readl_relaxed(reg_base + (i * 4)));
        else
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x,", readl_relaxed(reg_base + (i * 4)));   
    }

    return written_chars; // Restituiamo la lunghezza dei dati letti
}


static ssize_t matrA_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    //dmesg | grep "KERNEL:" To read kernel log messages
    printk(KERN_DEBUG "KERNEL: matrA_store data\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 *data; 
    int i, data_to_store, max_len_matr, size_matr;
    void __iomem *reg_base;
    int readed_datas;
    char *newbuf;

    printk(KERN_DEBUG "KERNEL: %d data recived: -%.*s-\n", len, len, buf);

    size_matr = (int)readl_relaxed(vf->base + SIZE_REG);
    max_len_matr = size_matr * size_matr;
    
    data = kmalloc(sizeof(u32)*max_len_matr, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "KERNEL: matrA_store - Error allocating dynamic mem\n");
        return -EFAULT;
    }

    newbuf = kmalloc(sizeof(char)*len, GFP_KERNEL);
    if (!newbuf) {
        printk(KERN_ERR "KERNEL: matrA_store - Error allocating dynamic mem\n");
        return -EFAULT;
    }
    
    //Substituting /n with , to use the function parse_hex_string
    for (i = 0; i < len; i++) {
        newbuf[i] = (buf[i] == '\n') ? ',' : buf[i];
    }

    readed_datas = parse_hex_string(newbuf, len, data, max_len_matr);

    printk(KERN_DEBUG "KERNEL: %d data converted:", readed_datas);
    for(i = 0; i < readed_datas; i++)
        printk(KERN_DEBUG "\t\t%d ", data[i]);

    data_to_store = (max_len_matr > readed_datas) ? readed_datas : max_len_matr;
    reg_base = vf->base + MATR_A_START;

    for (i = 0; i < data_to_store; i++) {
        writel_relaxed(data[i], reg_base + (i * 4));
    }
    
    kfree(data);
    return len;
}


static ssize_t matrB_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
    //dmesg | grep "KERNEL:" To read kernel log messages
    printk(KERN_DEBUG "KERNEL: matrB_show\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    //u32 *data; 
    int i;
    void __iomem *reg_base;
    int written_chars = 0;

    int len_matr = (int)readl_relaxed(vf->base + SIZE_REG);

    reg_base = vf->base + MATR_B_START;
    for (i = 0; i < len_matr; i++) {
        if(i < len_matr - 1)
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x,", readl_relaxed(reg_base + (i * 4)));
        else
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x\n", readl_relaxed(reg_base + (i * 4)));
    }

    return written_chars; // Restituiamo la lunghezza dei dati letti
}

static ssize_t matrB_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    //dmesg | grep "KERNEL:" To read kernel log messages
    printk(KERN_DEBUG "KERNEL: matrB_store data\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    u32 *data; 
    int i, data_to_store, max_len_matr;
    void __iomem *reg_base;
    int readed_datas;

    printk(KERN_DEBUG "KERNEL: %d data recived: -%.*s-\n", len, len, buf);

    max_len_matr = (int)readl_relaxed(vf->base + SIZE_REG);
    
    data = kmalloc(sizeof(u32)*max_len_matr, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "KERNEL: matrB_store - Error allocating dynamic mem\n");
        return -EFAULT;
    }

    readed_datas = parse_hex_string(buf, len, data, max_len_matr);

    printk(KERN_DEBUG "KERNEL: %d data converted:", readed_datas);
    for(i = 0; i < readed_datas; i++)
        printk(KERN_DEBUG "\t\t%d ", data[i]);

    data_to_store = (max_len_matr > readed_datas) ? readed_datas : max_len_matr;
    reg_base = vf->base + MATR_B_START;

    for (i = 0; i < data_to_store; i++) {
        writel_relaxed(data[i], reg_base + (i * 4));
    }
    
    kfree(data);
    return len;
}

static ssize_t matrC_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
    //dmesg | grep "KERNEL:" To read kernel log messages
    printk(KERN_DEBUG "KERNEL: matrC_show\n");

    struct virt_mulmatr *vf = dev_get_drvdata(dev);
    //u32 *data; 
    int i;
    void __iomem *reg_base;
    int written_chars = 0;

    int len_matr = (int)readl_relaxed(vf->base + SIZE_REG);

    reg_base = vf->base + MATR_C_START;
    for (i = 0; i < len_matr; i++) {
        if(i < len_matr - 1)
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x,", readl_relaxed(reg_base + (i * 4)));
        else
            written_chars += scnprintf(buf+written_chars, PAGE_SIZE-written_chars, "0x%x\n", readl_relaxed(reg_base + (i * 4)));
    }

    return written_chars; // Restituiamo la lunghezza dei dati letti
}

static DEVICE_ATTR(control, S_IRUGO | S_IWUSR, vf_show_control, vf_store_control);
static DEVICE_ATTR(size,    S_IRUGO | S_IWUSR, vf_show_size,    vf_store_size);
static DEVICE_ATTR(status,  S_IRUGO, vf_show_status,    NULL);
static DEVICE_ATTR(id,      S_IRUGO, vf_show_id,        NULL);

static DEVICE_ATTR(matrA, S_IRUGO | S_IWUSR, matrA_show, matrA_store);
static DEVICE_ATTR(matrB, S_IRUGO | S_IWUSR, matrB_show, matrB_store);
static DEVICE_ATTR(matrC, S_IRUGO, matrC_show, NULL);

static struct attribute *vf_attributes[] = {
    &dev_attr_control.attr,
    &dev_attr_size.attr,
    &dev_attr_status.attr,
    &dev_attr_id.attr,
    &dev_attr_matrA.attr,
    &dev_attr_matrB.attr,
    &dev_attr_matrC.attr,
    NULL,
};

static const struct attribute_group vf_attr_group = {
    .attrs = vf_attributes,
};

static void vf_init(struct virt_mulmatr *vf)
{
    writel_relaxed(DEFAULT_CTRL_REG, vf->base + CONTROL_REG);
}

static irqreturn_t vf_irq_handler(int irq, void *data)
{
    struct virt_mulmatr *vf = (struct virt_mulmatr *)data;
    u32 status;

    status = readl_relaxed(vf->base + STATUS_REG);

    if (status & BIT_S_OP_ENDED)
        printk(KERN_DEBUG "KERNEL: IRQ Operation Terminated\n");

    return IRQ_HANDLED;
}

static int vf_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    struct virt_mulmatr *vf;
    int ret;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENOMEM;

    vf = devm_kzalloc(dev, sizeof(*vf), GFP_KERNEL);
    if (!vf)
        return -ENOMEM;

    vf->dev = dev;
    vf->base = devm_ioremap(dev, res->start, resource_size(res));
    if (!vf->base)
        return -EINVAL;

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res) {
        ret = devm_request_irq(dev, res->start, vf_irq_handler,
                       IRQF_TRIGGER_HIGH, "vf_irq", vf);
        if (ret)
            return ret;
    }

    platform_set_drvdata(pdev, vf);

    vf_init(vf);

    return sysfs_create_group(&dev->kobj, &vf_attr_group);
}

static int vf_remove(struct platform_device *pdev)
{
    struct virt_mulmatr *vf = platform_get_drvdata(pdev);

    sysfs_remove_group(&vf->dev->kobj, &vf_attr_group);
    return 0;
}

static const struct of_device_id vf_of_match[] = {
    { .compatible = "virt-mulmatr", },
    { }
};
MODULE_DEVICE_TABLE(of, vf_of_match);

static struct platform_driver vf_driver = {
    .probe = vf_probe,
    .remove = vf_remove,
    .driver = {
        .name = "virt_mulmatr",
        .of_match_table = vf_of_match,
    },
};
module_platform_driver(vf_driver);

MODULE_DESCRIPTION("MUL MATR Kernel Module");
MODULE_AUTHOR("OS - group8");
MODULE_LICENSE("GPL");