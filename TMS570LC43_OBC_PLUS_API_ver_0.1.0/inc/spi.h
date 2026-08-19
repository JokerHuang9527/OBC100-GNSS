/*
 * spi.h
 *
 *  Created on: 2024¦~1¤ë16¤é
 *      Author: user
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_

#include "HL_mibspi.h"
#include "HL_sys_dma.h"

#define SPI_DEVICE1 0
#define SPI_DEVICE2 1
#define SPI_DEVICE3 2


enum _IO_CTRL_SPI_INTFLAGS
{
    IO_SPI_RX_INT = 1,
    IO_SPI_TX_INT
};

enum _IO_CTRL_SPI_SPEED
{
    IO_SPI_SPEED_1M = 1,
    IO_SPI_SPEED_5M = 5,
    IO_SPI_SPEED_10M = 10,
    IO_SPI_SPEED_20M = 20,
};

typedef struct IO_SPI_DMA_HANDLE
{
    int channel;
    int request;
} io_spi_dma_handle;

typedef struct IO_SPI_DEVICES_HANDLE
{
    mibspiBASE_t *spi;
    mibspiRAM_t *ram;
    io_spi_dma_handle tx;
    io_spi_dma_handle rx;
    uint32 group;
    mibspiDFMT_t DFMT;
    uint8 blockflag;
    uint8 state;
    void (*init)();
} io_spi_devices_handle;

extern io_spi_devices_handle io_spi_devices_handle_table[];

void spi_info();
int spi_transfer(void *hnd, uint16 length, uint8* txbuffer ,uint8* rxbuffer);
int is_spi_ready(void *hnd);
void *spi_open(int device);
int spi_close(void *handle);
int spi_ioctl(void *param1, void *param2, void *param3, void *param4);
void mibspi1Init(void);
void mibspi2Init(void);
void mibspi3Init(void);
void mibspi5Init(void);

#endif /* INC_SPI_H_ */
