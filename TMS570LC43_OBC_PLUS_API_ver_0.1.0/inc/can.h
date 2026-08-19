/*
 * can.h
 *
 *  Created on: 2024¦~2¤ë22¤é
 *      Author: user
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "HL_can.h"

#define CAN_COM1 0
#define CAN_COM2 1

#define IDS (0x000007FFU) //ID Standard
#define IDE (0x1FFFFFFFU) //ID Extended

enum _IO_CTRL_CAN_DIR_FLAGS
{
    IO_CAN_TX = 0,
    IO_CAN_RX,
};

enum _IO_CTRL_CAN_SPEED
{
    IO_CAN_SPEED_10 = 0,
    IO_CAN_SPEED_50 = 50,
    IO_CAN_SPEED_100 = 100,
    IO_CAN_SPEED_125 = 125,
    IO_CAN_SPEED_200 = 200,
    IO_CAN_SPEED_250 = 250,
    IO_CAN_SPEED_500 = 500,
    IO_CAN_SPEED_800 = 800,
    IO_CAN_SPEED_1000 = 1000,
    IO_CAN_MAX_SPEED = 1000,
};

typedef struct IO_CAN_MSGBOX_HANDLE
{
    uint8 ideflag;
    uint32 id;
    uint32 mask;
    uint8 datalength;
} io_can_msgbox_handle;

typedef struct IO_CAN_BITRATE_HANDLE
{
    uint16 bitrate;
    uint8 btre;
    uint8 btr;
    uint8 tseg2;
    uint8 tseg1;
    uint8 sjw;
} io_can_bitrate_handle;

typedef struct IO_CAN_DEVICES_HANDLE
{
    io_can_bitrate_handle *bitehandle;
    io_can_msgbox_handle txmsgbox;
    io_can_msgbox_handle rxmsgbox;
} io_can_devices_handle;

void can_info();
int can_get_index(canBASE_t *handle);
int can_write( void *hnd, uint32 messageBox, uint8_t * tx_buffer, uint8 datalen);
int can_read(void *hnd, uint32 messageBox, uint8_t * rx_buffer, uint8 datalen);
void *can_open(int index);
int can_close(void *handle);
int can_ioctl(void *param1, void *param2, void *param3, void *param4, void *param5);
void io_can_init(int index);

#endif /* INC_CAN_H_ */
