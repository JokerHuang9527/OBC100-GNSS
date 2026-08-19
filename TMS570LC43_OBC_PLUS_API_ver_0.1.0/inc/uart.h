/*
 * uart.h
 *
 *  Created on: 2023¦~10¤ë20¤é
 *      Author: user
 */

#ifndef INC_UART_H_
#define INC_UART_H_
#include "HL_sci.h"

#define UART_COM1 0
#define UART_COM2 1
#define UART_COM3 2
#define UART_COM4 3

#define CONSOLE_PORT UART_COM1
#define RSI2000_PORT UART_COM3
//#define PC_PORT UART_COM4

enum _IO_CTRL_UART_SPEED
{
    IO_UART_SPEED_5 = 5,
    IO_UART_SPEED_200 = 200,
    IO_UART_SPEED_4800 = 4800,
    IO_UART_SPEED_9600 = 9600,
    IO_UART_SPEED_10400 = 10400,
    IO_UART_SPEED_19200 = 19200,
    IO_UART_SPEED_38400 = 38400,
    IO_UART_SPEED_57600 = 57600,
    IO_UART_SPEED_115200 = 115200,
    IO_UART_MAX_SPEED = 115200,

};

enum _IO_CTRL_UART_INTFLAGS
{
    IO_SCI_RX_INT    = 0x00000200U,  /* receive buffer ready */
    IO_SCI_TX_INT    = 0x00000100U,  /* transmit buffer ready */
};

void uart_init();
int uart_get_index(sciBASE_t *handle);
int uart_write(void *hnd,uint16_t tx_length,uint8_t * tx_buffer);
int uart_tx_complete(void *hnd);
int uart_rx_Ready(void *hnd);
int uart_read(void *hnd,uint16_t rx_length,uint8_t * rx_buffer);
void *uart_open(int index);
int uart_close(void *handle);


int uart_ioctl(void *param1, void *param2, void *param3, void *param4);



#endif /* INC_UART_H_ */
