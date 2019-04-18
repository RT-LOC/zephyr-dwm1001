/*! ----------------------------------------------------------------------------
 *  @file    ex_11a_main.c
 *  @brief   Example of button usage. Simple callback on button press.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 *
 * All rights reserved.
 *
 */
#include <zephyr.h>
#include <device.h>
#include <gpio.h>

/* Defines */
#define APP_HEADER "\nDWM1001 & Zephyr\n"
#define APP_NAME "Example 11a - BUTTON\n"
#define APP_VERSION "Version - 1.3\n"
#define APP_LINE "=================\n"

#define PIN_BUTTON	2

static struct gpio_callback gpio_cb;
struct device *gpiob;

/* Button Pressed callback */
void button_pressed(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	printk("Button pressed at %d\n", k_cycle_get_32());
}

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_HEADER);
    printk(APP_NAME);
    printk(APP_VERSION);
    printk(APP_LINE);
	
	/* Get GPIO device binding */
	gpiob = device_get_binding(SW0_GPIO_CONTROLLER);
	if (!gpiob) {
		printk("error\n");
		return -1;
	}

	/* Init Button Interrupt */
	gpio_pin_configure(gpiob, PIN_BUTTON,
			   GPIO_DIR_IN | GPIO_INT |  GPIO_PUD_PULL_UP | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);
	gpio_init_callback(&gpio_cb, button_pressed, BIT(2));
	gpio_add_callback(gpiob, &gpio_cb);
	gpio_pin_enable_callback(gpiob, 2);

	return 0;
}