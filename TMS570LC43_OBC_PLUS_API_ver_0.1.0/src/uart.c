
#include <stdio.h>
#include "uart.h"
#include "xioctl.h"

void uart_init(void *hnd)
{
    int index = (int)(hnd);
    sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
    if (sci == NULL)
        return E_INVALID_INPUT;
    io_uart_init(index, sci);
}

int uart_get_index(sciBASE_t *handle)
{
    int index;
    if (handle == sciREG1)
        index = 0;
    else if (handle == sciREG2)
        index = 1;
    else if (handle == sciREG3)
        index = 2;
    else if (handle == sciREG4)
        index = 3;
    else
        return NULL;

    return index;
}

int uart_tx_complete(void *hnd)
{
    int index = (int)(hnd);
    sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
    if (sci == NULL)
        return E_INVALID_INPUT;

	return sciIsTxReady(sci);
}

int uart_rx_Ready(void *hnd)
{
    int index = (int)(hnd);
    sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
    if (sci == NULL)
        return E_INVALID_INPUT;

	return (sciIsRxReady(sci) != (uint32)SCI_RX_INT);
}

int uart_write(
        void *hnd,
        uint16_t tx_length,
        uint8_t * tx_buffer
    )
{
    int index = (int)(hnd);
    sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
    if (sci == NULL)
            return E_INVALID_INPUT;
#if 0
    int i;
    for(i=0;i<tx_length;i++)
        printk("Txbuffer[%02d] = %02x\n", i, tx_buffer[i]);
    return E_SUCCESS;
#endif
    sciSend(sci, tx_length, tx_buffer);
    return E_SUCCESS;
}

int uart_read(
        void *hnd,
        uint16_t rx_length,
        uint8_t * rx_buffer

    )  
{
    int index = (int)hnd;
    sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
    if (sci == NULL)
        return E_INVALID_INPUT;

	sciReceive(sci, rx_length, rx_buffer);
	return E_SUCCESS;
}

void *uart_open(int index)
{
	sciBASE_t *handle;
	if (index == 0)
		handle = sciREG1;
	else if (index == 1)
		handle = sciREG2;
	else if (index == 2)
		handle = sciREG3;
	else if (index == 3)
		handle = sciREG4;
	else
	    return NULL;
		
	return (void *)handle;
}

int uart_close(void *handle)
{
	return 0;
}

int uart_ioctl(void *param1, void *param2, void *param3, void *param4)
{
    int index = (int)param1;
	sciBASE_t *sci = (sciBASE_t *)(uart_open(index));
	if (sci == NULL)
	        return E_INVALID_INPUT;

	int ctrl_type = (io_ctrl_t) param2;

	switch (ctrl_type){

	case UART_CTL_BLOCK_MODE:
	{
	    uint32 mode = (int) param3;
	    uint32 flag = (uint32) param4;
        if ((mode != NONE_BLOCKING) && (mode != BLOCKING))
            return E_INVALID_INPUT;
        if ((flag != IO_SCI_RX_INT) && (flag != IO_SCI_TX_INT))
            return E_INVALID_INPUT;

        if(mode == NONE_BLOCKING)
            sciEnableNotification(sci, flag);
        else
            sciDisableNotification(sci, flag);
        return(E_SUCCESS);
	}
	case UART_CTL_BAUDRATE:
	{
	    uint32 baud = (uint32)param3;
	    if (baud != IO_UART_SPEED_5
            && baud != IO_UART_SPEED_200
            && baud != IO_UART_SPEED_4800
            && baud != IO_UART_SPEED_9600
            && baud != IO_UART_SPEED_10400
            && baud != IO_UART_SPEED_19200
            && baud != IO_UART_SPEED_38400
            && baud != IO_UART_SPEED_57600
            && baud != IO_UART_SPEED_115200)
            return E_INVALID_INPUT;

	    sciSetBaudrate(sci, baud);
	    return(E_SUCCESS);
	}
	default:
	{
        return(E_NOT_SUPPORT);
	}

	}

		
}
