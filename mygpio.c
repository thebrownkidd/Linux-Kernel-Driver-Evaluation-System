// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>

#define MYGPIO_REG_DATA     0x00
#define MYGPIO_REG_DIR      0x04
#define MYGPIO_REG_IRQ_STAT 0x08
#define MYGPIO_REG_IRQ_MASK 0x0C

struct mygpio {
    void __iomem *base;
    int irq;
    struct gpio_chip gc;
};

static int mygpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct mygpio *chip = gpiochip_get_data(gc);
    u32 val = readl(chip->base + MYGPIO_REG_DATA);
    return !!(val & BIT(offset));
}

static void mygpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct mygpio *chip = gpiochip_get_data(gc);
    u32 val = readl(chip->base + MYGPIO_REG_DATA);

    if (value)
        val |= BIT(offset);
    else
        val &= ~BIT(offset);

    writel(val, chip->base + MYGPIO_REG_DATA);
}

static int mygpio_direction_output(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct mygpio *chip = gpiochip_get_data(gc);
    u32 dir = readl(chip->base + MYGPIO_REG_DIR);

    dir |= BIT(offset); // mark as output
    writel(dir, chip->base + MYGPIO_REG_DIR);

    mygpio_set(gc, offset, value);
    return 0;
}

static int mygpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct mygpio *chip = gpiochip_get_data(gc);
    u32 dir = readl(chip->base + MYGPIO_REG_DIR);

    dir &= ~BIT(offset); // mark as input
    writel(dir, chip->base + MYGPIO_REG_DIR);
    return 0;
}

static irqreturn_t mygpio_irq_handler(int irq, void *dev_id)
{
    struct mygpio *chip = dev_id;
    u32 stat = readl(chip->base + MYGPIO_REG_IRQ_STAT);

    if (!stat)
        return IRQ_NONE;

    pr_info("mygpio: IRQ! status=0x%x\n", stat);

    writel(stat, chip->base + MYGPIO_REG_IRQ_STAT); // clear
    return IRQ_HANDLED;
}

static int mygpio_probe(struct platform_device *pdev)
{
    struct mygpio *chip;
    struct resource *res;
    int ret;

    chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    chip->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(chip->base))
        return PTR_ERR(chip->base);

    chip->irq = platform_get_irq(pdev, 0);
    if (chip->irq < 0)
        return chip->irq;

    chip->gc.label = dev_name(&pdev->dev);
    chip->gc.parent = &pdev->dev;
    chip->gc.owner = THIS_MODULE;
    chip->gc.base = -1;
    chip->gc.ngpio = 8;
    chip->gc.get = mygpio_get;
    chip->gc.set = mygpio_set;
    chip->gc.direction_input = mygpio_direction_input;
    chip->gc.direction_output = mygpio_direction_output;

    ret = devm_gpiochip_add_data(&pdev->dev, &chip->gc, chip);
    if (ret)
        return ret;

    ret = devm_request_irq(&pdev->dev, chip->irq, mygpio_irq_handler,
                           0, dev_name(&pdev->dev), chip);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "mygpio probed\n");
    return 0;
}

static const struct of_device_id mygpio_of_match[] = {
    { .compatible = "acme,mygpio" },
    {}
};
MODULE_DEVICE_TABLE(of, mygpio_of_match);

static struct platform_driver mygpio_driver = {
    .probe  = mygpio_probe,
    .driver = {
        .name = "mygpio",
        .of_match_table = mygpio_of_match,
    },
};
module_platform_driver(mygpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Platform driver for memory-mapped GPIO with IRQ");
