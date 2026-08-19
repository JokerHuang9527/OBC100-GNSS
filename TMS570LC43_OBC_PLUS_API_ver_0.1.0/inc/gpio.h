/*
 * gpio.h
 *
 *  Created on: 2024¦~2¤ë2¤é
 *      Author: user
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include "HL_gio.h"
#include "HL_reg_het.h"


#define GPIO_COM1 0
#define GPIO_COM2 1
#define GPIO_COM3 2
#define GPIO_COM4 3
#define GPIO_COM5 4
#define GPIO_COM6 5
#define GPIO_COM7 6
#define GPIO_COM8 7
#define GPIO_COM9 8

enum _IO_CTRL_GPIO_IO_PRSET_FLAGS
{
    IO_GPIO_PRSET_INPUT = 0,
    IO_GPIO_PRSET_OUTPUT,
    IO_GPIO_PRSET_GENERIC
};

enum _IO_CTRL_GPIO_IO_DIR_FLAGS
{
    IO_GPIO_DIR_INPUT = 0,
    IO_GPIO_DIR_OUTPUT,
};

enum _IO_CTRL_GPIO_OUTPUT_DRAIN_FLAGS
{
    IO_GPIO_OUTPUT_DRAIN_CLOSE = 0,
    IO_GPIO_OUTPUT_DRAIN_OPEN,
};

enum _IO_CTRL_GPIO_INPUT_PULL_FLAGS
{
    IO_GPIO_INPUT_PULL_CLOSE = 0,
    IO_GPIO_INPUT_PULL_OPEN,
};

enum _IO_CTRL_GPIO_PULL_DIR_FLAGS
{
    IO_GPIO_PULL_DIR_DOWN = 0,
    IO_GPIO_PULL_DIR_UP,
};

enum _IO_CTRL_GPIO_STATUS_FLAGS
{
    IO_GPIO_LOW = 0,
    IO_GPIO_HIGH,
};

typedef struct IO_GPIO_DEVICES_HANDLE
{
    gioPORT_t * gpio;
    uint8 pinbit;
    uint8 ioflag;
} io_gpio_devices_handle;

void gpio_info();
void gpio_init();
int gpio_output(int index, int status);
int gpio_input(int index);
void *gpio_open(int index);
int gpio_ioctl(void *param1, void *param2, void *param3, void *param4);

#endif /* INC_GPIO_H_ */
