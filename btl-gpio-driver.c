#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/uaccess.h>

#define MAX_USER_SIZE 1024

#define BCM2837_GPIO_ADDRESS 0x3F200000
#define BCM2711_GPIO_ADDRESS 0xfe200000

static struct proc_dir_entry *proc = NULL;
static char data_buffer[MAX_USER_SIZE + 1] = {0};
static unsigned int *gpio_registers = NULL;

static void gpio_pin_on(unsigned int pin)
{
    unsigned int fsel_index = pin / 10;
    unsigned int fsel_bitpos = pin % 10;
    unsigned int *gpio_fsel = gpio_registers + fsel_index;
    unsigned int *gpio_on_register = (unsigned int *)((char *)gpio_registers + 0x1c);

    *gpio_fsel &= ~(7 << (fsel_bitpos * 3));
    *gpio_fsel |= (1 << (fsel_bitpos * 3));
    *gpio_on_register = (1 << pin);
}

static void gpio_pin_off(unsigned int pin)
{
    unsigned int *gpio_off_register = (unsigned int *)((char *)gpio_registers + 0x28);
    *gpio_off_register = (1 << pin);
}

ssize_t read(struct file *file, char __user *user, size_t size, loff_t *off)
{
    const char *msg = "Hello!\n";
    size_t len = strlen(msg);
    return copy_to_user(user, msg, len) ? -EFAULT : len; // Trả về EFAULT nếu copy_to_user gặp lỗi
}

ssize_t write(struct file *file, const char __user *user, size_t size, loff_t *off)
{
    unsigned int pin = UINT_MAX;
    unsigned int value = UINT_MAX;

    if (size > MAX_USER_SIZE)
        size = MAX_USER_SIZE;

    memset(data_buffer, 0x0, sizeof(data_buffer));

    if (copy_from_user(data_buffer, user, size))
        return -EFAULT;

    data_buffer[size] = '\0'; // Đảm bảo chuỗi kết thúc bằng null
    printk("Data buffer: %s\n", data_buffer);

    if (sscanf(data_buffer, "%u,%u", &pin, &value) != 2) // Sử dụng %u cho unsigned int
    {
        printk("Dữ liệu không đúng định dạng\n");
        return size;
    }

    if (pin < 0 || pin > 28) // Kiểm tra phạm vi chân GPIO hợp lệ
    {
        printk("Chân GPIO không hợp lệ\n");
        return size;
    }

    if (value != 0 && value != 1) // Kiểm tra giá trị bật/tắt hợp lệ
    {
        printk("Giá trị bật/tắt không hợp lệ\n");
        return size;
    }

    printk("Bạn đã nhập pin %u, giá trị %u\n", pin, value);

    // Thực hiện bật hoặc tắt chân GPIO tương ứng
    if (value == 1)
    {
        gpio_pin_on(pin);
    }
    else if (value == 0)
    {
        gpio_pin_off(pin);
    }

    return size;
}

static const struct proc_ops proc_fops =
{
    .proc_read = read,
    .proc_write = write,
};

static int __init gpio_driver_init(void)
{
    printk("Chào mừng đến với driver của tôi!\n");

    gpio_registers = (unsigned int *)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);

    if (gpio_registers == NULL)
    {
        printk("Không thể ánh xạ bộ nhớ GPIO cho driver\n");
        return -ENOMEM;
    }

    printk("Ánh xạ bộ nhớ GPIO thành công\n");

    proc = proc_create("gpio", 0666, NULL, &proc_fops);
    if (proc == NULL)
    {
        iounmap(gpio_registers);
        return -ENOMEM;
    }

    return 0;
}

static void __exit gpio_driver_exit(void)
{
    printk("Đang thoát driver của tôi!\n");
    if (gpio_registers)
        iounmap(gpio_registers);
    proc_remove(proc);
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Low Level Learning");
MODULE_DESCRIPTION("Thử nghiệm viết driver cho Raspberry Pi");
MODULE_VERSION("1.0");
