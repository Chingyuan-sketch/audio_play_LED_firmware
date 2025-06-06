#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "ws2812"

static struct spi_device *ws2812_spi_dev;

static ssize_t ws2812_write(struct file *file, const char __user *buf,
                             size_t len, loff_t *offset) {
    u8 *kbuf;
    int ret;

    if (!ws2812_spi_dev)
        return -ENODEV;

    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    struct spi_transfer t = {
        .tx_buf = kbuf,
        .len = len,
        .cs_change = 0,
    };
    struct spi_message m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);

    ret = spi_sync(ws2812_spi_dev, &m);

    kfree(kbuf);
    return ret == 0 ? len : ret;
}

static const struct file_operations ws2812_fops = {
    .owner = THIS_MODULE,
    .write = ws2812_write,
};

static struct miscdevice ws2812_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &ws2812_fops,
};

static int ws2812_probe(struct spi_device *spi) {
    int ret;
    ws2812_spi_dev = spi;

    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz = 2400000; // 約 WS2812 相容速率
    ret = spi_setup(spi);
    if (ret) return ret;

    return misc_register(&ws2812_miscdev);
}

static int ws2812_remove(struct spi_device *spi) {
    misc_deregister(&ws2812_miscdev);
    return 0;
}

static const struct of_device_id ws2812_dt_ids[] = {
    { .compatible = "meme,ws2812" },
    { }
};
MODULE_DEVICE_TABLE(of, ws2812_dt_ids);

static struct spi_driver ws2812_driver = {
    .driver = {
        .name = "meme-ws2812",
        .of_match_table = ws2812_dt_ids,
    },
    .probe = ws2812_probe,
    .remove = ws2812_remove,
};

module_spi_driver(ws2812_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple SPI WS2812 Driver for /dev/ws2812");