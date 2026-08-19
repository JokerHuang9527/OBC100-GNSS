#ifndef INC_XIOCTL_H_
#define INC_XIOCTL_H_

#include "uart.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"
#include "can.h"

#define CTL_COMMON_BASE		0
#define CTL_UART_BASE		0x1000
#define CTL_I2C_BASE		0x2000
#define CTL_SPI_BASE		0x3000
#define CTL_GPIO_BASE       0x4000
#define CTL_CAN_BASE       0x5000
#define BLOCKING        0
#define NONE_BLOCKING   1

enum _IO_CTRL_COMMON
{
    DISABLE = 0,
    ENABLE,
};

enum _IO_CTRL_T
{
    UART_CTL_BLOCK_MODE = CTL_UART_BASE,
    UART_CTL_BAUDRATE,
    I2C_CTL_XADDRESS = CTL_I2C_BASE,
    I2C_CTL_BITCOUNT,
    I2C_CTL_BLOCK_MODE,
    I2C_CTL_BAUDRATE,
    I2C_CTL_INGNORE_NACK_MODE,
    SPI_CTL_BLOCK_MODE = CTL_SPI_BASE,
    SPI_CTL_BAUDRATE,
    GPIO_CTL_DIR_MODE = CTL_GPIO_BASE,
    GPIO_CTL_OUTPUT_DRAIN,
    GPIO_CTL_INPUT_PULL,
    GPIO_CTL_PULL_MODE,
    CAN_CTL_ID = CTL_CAN_BASE,
    CAN_CTL_MASK,
    CAN_CTL_DATALENGTH,
    CAN_CTL_BAUDRATE
};
typedef enum _IO_CTRL_T io_ctrl_t;

enum _IO_CTRL_ERR_T
{
    E_SUCCESS = 0,
    E_INVALID_INPUT = -1,
    E_BUSY = -2,
    E_TIMEOUT = -3,
    E_NOT_SUPPORT = -4,
};
typedef enum _IO_CTRL_ERR_T io_ctrl_err_t;

typedef int (*ioctl_func_t)(void *, void *, void *, void *);


typedef struct CLASS_FUNC
{
	int num_of_funcs;
	ioctl_func_t *class_funcs;
} class_func_t;


#define do_ioctl(a, b, c, d, e) xioctl(a, (void *)b, (void *)c, (void *)d, (void *)e)
void xioresponse(int error);

#endif /* INC_XIOCTL_H_ */
