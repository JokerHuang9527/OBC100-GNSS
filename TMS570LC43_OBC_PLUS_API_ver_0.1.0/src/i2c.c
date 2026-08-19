/*
 * I2C.c
 *
 *  Created on: 2024¦~1¤ë11¤é
 *      Author: user
 */




#include "i2c.h"

#include "xioctl.h"

io_i2c_setting io_i2c_settable[] = {
    {IO_I2C_7BIT_AMODE, IO_I2C_8_BIT, DISABLE},
    {IO_I2C_7BIT_AMODE, IO_I2C_8_BIT, DISABLE}, };

void i2c_init(void *hnd)
{
    int index = (int)(hnd);
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
        return E_INVALID_INPUT;
    io_i2c_init(index, i2c);
}

int i2c_get_index(i2cBASE_t *handle)
{
    int index;
    if (handle == i2cREG1)
        index = 0;
    else if (handle == i2cREG2)
        index = 1;
    else
        return NULL;

    return index;
}

int i2c_tx_complete(void *hnd)
{
    int index = (int)(hnd);
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
        return E_INVALID_INPUT;

    return i2cIsTxReady(i2c);
}

int i2c_rx_Ready(void *hnd)
{
    int index = (int)(hnd);
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
        return E_INVALID_INPUT;

    return i2cIsRxReady(i2c);
}

int i2c_write(
        void *hnd,
        uint32 slave_add,
        uint32 write_add,
        uint32 length,
        uint8 * data
    )
{
    int index = (int)(hnd);
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
            return E_INVALID_INPUT;

    i2c_init(index);
    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, slave_add);

    /* Set direction to Transmitter */
    i2cSetDirection(i2c, I2C_TRANSMITTER);

    /* Configure Data count */
    i2cSetCount(i2c, length+1);

    /* Set mode as Master */
    i2cSetMode(i2c, I2C_MASTER);

    /* Set Stop after programmed Count */
    i2cSetStop(i2c);

    /* Transmit Start Condition */
    i2cSetStart(i2c);

    /* Send the Word Address */
    i2cSendByte(i2c, write_add);


    /* Tranmit DATA_COUNT number of data in Polling mode */
    i2cSend(i2c, length, data);

    /* Wait until Bus Busy is cleared */
    while(i2cIsBusBusy(i2c) == true);


    /* Wait until Stop is detected */
    while(i2cIsStopDetected(i2c) == 0);

    i2cClearSCD(i2c);

    return E_SUCCESS;
}

int i2c_read(
        void *hnd,
        uint32 slave_add,
        uint32 read_add,
        uint32 length,
        uint8 * data
    )
{
    int i;
    int index = (int)hnd;
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
        return E_INVALID_INPUT;

    i2c_init(index);
    /* Clear Stop bit */

    i2c->MDR &= ~(I2C_STOP_COND);

    /* Clear Start bit */

    i2c->MDR &= ~(I2C_START_COND);

    /* Clear Repeat mode bit */

    i2c->MDR &= ~(I2C_REPEATMODE);
    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, slave_add);

    /* Set direction to Transmitter */
    i2cSetDirection(i2c, I2C_TRANSMITTER);

    /* Configure Data count */
    /* Slave address + Word address write operation before reading */
    i2cSetCount(i2c, 0x01);

    /* Set mode as Master */
    i2cSetMode(i2c, I2C_MASTER);

    /* Transmit Start Condition */
    i2cSetStart(i2c);

    /* Send the Word Address */
    i2cSendByte(i2c, read_add);


    while( ((i2c->STR & I2C_NACK) == 0) && ((i2c->STR & I2C_ARDY) == 0) );
    /* wait until MST bit gets cleared, this takes
     * few cycles after Bus Busy is cleared */

    i2cSetMode(i2c, I2C_MASTER); // Switch to Master Receiver

    i2cSetDirection(i2c, I2C_RECEIVER);

    /* Set Repeat start mode */

    i2c->MDR |= I2C_REPEATMODE;

    /* Start transmit*/

    i2cSetStart(i2c); //I2C BUS: Start--Slave Addr

    for (i=0; i<length + 1; i++)

    {

        data[i] = i2cReceiveByte(i2c); // Read incoming data and store in array

        if ((i == length - 1) || length == 0){

            i2cSetStop(i2c); //to generate a STOP

        }

    }

    while((i2c->MDR & I2C_STOP_COND) ==1);

    i2cClearSCD(i2c);

    return E_SUCCESS;
}

void *i2c_open(int index)
{
    i2cBASE_t *handle;
    if (index == 0)
        handle = i2cREG1;
    else if (index == 1)
        handle = i2cREG2;
    else
        return NULL;

    return (void *)handle;
}

int i2c_close(void *handle)
{
    return 0;
}

int i2c_ioctl(void *param1, void *param2, void *param3, void *param4)
{
    int index = (int)param1;
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
            return E_INVALID_INPUT;

    int ctrl_type = (io_ctrl_t) param2;

    switch (ctrl_type){

    case I2C_CTL_BLOCK_MODE:
    {
        int mode = (int) param3;
        uint32 flag = (uint32)param4;
        if ((mode != NONE_BLOCKING) && (mode != BLOCKING))
            return E_INVALID_INPUT;

        if ((flag != IO_I2C_RX_INT) && (flag != IO_I2C_TX_INT))
            return E_INVALID_INPUT;

        if(mode == NONE_BLOCKING)
            i2cEnableNotification(i2c, flag);
        else
            i2cDisableNotification(i2c, flag);
        return(E_SUCCESS);
    }
    case I2C_CTL_BAUDRATE:
    {
        uint32 baud = (uint32)param3;
        if (baud < IO_I2C_MIN_SPEED ||
            baud > IO_I2C_MAX_SPEED   )
            return E_INVALID_INPUT;

        i2cSetBaudrate(i2c, baud);
        return(E_SUCCESS);
    }
    case I2C_CTL_XADDRESS:
    {
        uint32 xaddress = (uint32)param3;
        if (xaddress != IO_I2C_7BIT_AMODE
            && xaddress != IO_I2C_10BIT_AMODE)
            return E_INVALID_INPUT;

        io_i2c_settable[index].address_mode = xaddress;
        return(E_SUCCESS);
    }
    case I2C_CTL_BITCOUNT:
    {
        uint32 bitcount = (uint32)param3;
        if (bitcount != IO_I2C_2_BIT
            && bitcount != IO_I2C_3_BIT
            && bitcount != IO_I2C_4_BIT
            && bitcount != IO_I2C_5_BIT
            && bitcount != IO_I2C_6_BIT
            && bitcount != IO_I2C_7_BIT
            && bitcount != IO_I2C_8_BIT)
            return E_INVALID_INPUT;

        io_i2c_settable[index].bitcount = bitcount;
        return(E_SUCCESS);
    }
    case I2C_CTL_INGNORE_NACK_MODE:
    {
        uint32 mode = (uint32)param3;
        if (mode != DISABLE
            && mode != ENABLE)
            return E_INVALID_INPUT;

        io_i2c_settable[index].ignore_nack = mode;
        return(E_SUCCESS);
    }
    default:
    {
        return(E_NOT_SUPPORT);
    }

    }


}

int i2c_transfer(
        void *hnd,
        uint32 slave_add,
        uint32 length_tx,
        uint32 length_rx,
        uint8 * tx_data,
        uint8 * rx_data
    )
{
    int i;
    int index = (int)(hnd);
    i2cBASE_t *i2c = (i2cBASE_t *)(i2c_open(index));
    if (i2c == NULL)
            return E_INVALID_INPUT;

    i2c_init(index);
    /* Clear Stop bit */

    i2c->MDR &= ~(I2C_STOP_COND);

    /* Clear Start bit */

    i2c->MDR &= ~(I2C_START_COND);

    /* Clear Repeat mode bit */

    i2c->MDR &= ~(I2C_REPEATMODE);
    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, slave_add);

    i2cSetMode(i2c, I2C_MASTER); // Switch to Master Receiver

    i2cSetDirection(i2c, I2C_TRANSMITTER);

    /* Set Repeat start mode */

    i2c->MDR |= I2C_REPEATMODE;

    /* Start transmit*/

    i2cSetStart(i2c); //I2C BUS: Start--Slave Addr

    for (i=0; i<length_tx; i++)

    {

        i2cSendByte(i2c, tx_data[i]); // Read incoming data and store in array

        if (i == length_tx-1){

            i2cSetStop(i2c); //to generate a STOP

        }

    }

    /* Wait until Bus Busy is cleared */
    while(i2cIsBusBusy(i2c) == true);

    while((i2c->MDR & I2C_STOP_COND) ==1);

    i2cClearSCD(i2c);
    vTaskDelay(1);

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, slave_add);

    i2cSetMode(i2c, I2C_MASTER); // Switch to Master Receiver

    i2cSetDirection(i2c, I2C_RECEIVER);

    /* Set Repeat start mode */

    i2c->MDR |= I2C_REPEATMODE;

    /* Start transmit*/

    i2cSetStart(i2c); //I2C BUS: Start--Slave Addr

    for (i=0; i<length_rx; i++)

    {

        rx_data[i] = i2cReceiveByte(i2c); // Read incoming data and store in array

        if (i == length_rx-1){

            i2cSetStop(i2c); //to generate a STOP

        }

    }
    /* Wait until Bus Busy is cleared */
    while(i2cIsBusBusy(i2c) == true);

    while((i2c->MDR & I2C_STOP_COND) ==1);

    i2cClearSCD(i2c);


    return E_SUCCESS;
}
