/*
 * i2c.h
 *
 *  Created on: 2024¦~1¤ë11¤é
 *      Author: user
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#include "HL_i2c.h"

#define I2C_COM1 0
#define I2C_COM2 1

#define RTC_PORT I2C_COM1

typedef struct IO_I2C_SETTING
{
    uint32 address_mode;
    uint32 bitcount;
    uint32 ignore_nack;
} io_i2c_setting;

extern io_i2c_setting io_i2c_settable[];

enum _IO_CTRL_I2C_SPEED
{
    IO_I2C_MIN_SPEED = 100,
    IO_I2C_MAX_SPEED = 400,

};

enum _IO_CTRL_I2C_MODE
{
    IO_I2C_7BIT_AMODE = 0x0000U,
    IO_I2C_10BIT_AMODE = 0x0100U,
};

enum _IO_CTRL_I2C_BITCOUNT
{
    IO_I2C_2_BIT   = 0x2U,
    IO_I2C_3_BIT   = 0x3U,
    IO_I2C_4_BIT   = 0x4U,
    IO_I2C_5_BIT   = 0x5U,
    IO_I2C_6_BIT   = 0x6U,
    IO_I2C_7_BIT   = 0x7U,
    IO_I2C_8_BIT   = 0x0U
};


enum _IO_CTRL_I2C_INTFLAGS
{
    IO_I2C_RX_INT     = 0x0008U,  /* receive data ready    */
    IO_I2C_TX_INT     = 0x0010U,  /* transmit data ready   */
};

void i2c_init(void *hnd);
int i2c_get_index(i2cBASE_t *handle);
int i2c_tx_complete(void *hnd);
int i2c_rx_Ready(void *hnd);
int i2c_write(void *hnd, uint32 slave_add, uint32 read_add, uint32 length, uint8 * data);
int i2c_read(void *hnd, uint32 slave_add, uint32 read_add, uint32 length, uint8 * data);
void *i2c_open(int index);
int i2c_close(void *handle);
int i2c_ioctl(void *param1, void *param2, void *param3, void *param4);
int i2c_transfer(void *hnd,uint32 slave_add,uint32 length_tx,uint32 length_rx,uint8 * tx_data,uint8 * rx_data);
#endif /* INC_I2C_H_ */
