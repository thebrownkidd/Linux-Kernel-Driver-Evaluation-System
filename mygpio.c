#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>

struct mygpio {
    struct gpio_chip gc;
};

static int mygpio_get(struct gpio_chip *gc, unsigned int offset)
{
    return 0; /* always 0 for now */
}

static void mygpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    /* do nothing */
}

static int mygpio_probe(struct platform_device *pdev)
{
    struct mygpio *chip;

    chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    chip->gc.label = "mygpio";
    chip->gc.parent = &pdev->dev;
    chip->gc.owner = THIS_MODULE;
    chip->gc.base = -1;
    chip->gc.ngpio = 4; /* 4 fake pins */
    chip->gc.get = mygpio_get;
    chip->gc.set = mygpio_set;

    return devm_gpiochip_add_data(&pdev->dev, &chip->gc, chip);
}

static int mygpio_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id mygpio_of_match[] = {
    { .compatible = "acme,mygpio", },
    { },
};
MODULE_DEVICE_TABLE(of, mygpio_of_match);

static struct platform_driver mygpio_driver = {
    .driver = {
        .name = "mygpio",
        .of_match_table = mygpio_of_match,
    },
    .probe = mygpio_probe,
    .remove = mygpio_remove,
};

module_platform_driver(mygpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Fake GPIO driver");
