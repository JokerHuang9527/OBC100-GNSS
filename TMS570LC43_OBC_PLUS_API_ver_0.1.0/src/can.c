#include <stdio.h>
#include "can.h"
#include "xioctl.h"
/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_time.h"

const io_can_bitrate_handle io_can_bitrates_handle_table[] = {
    {IO_CAN_SPEED_10, 10, 60, 6, 6, 4},
    {IO_CAN_SPEED_50, 2, 35, 5, 5, 4},
    {IO_CAN_SPEED_100, 1, 17, 5, 5, 4},
    {IO_CAN_SPEED_125, 1, 14, 4, 4, 4},
    {IO_CAN_SPEED_200, 0, 40, 5, 5, 4},
    {IO_CAN_SPEED_250, 0, 21, 8, 8, 4},
    {IO_CAN_SPEED_500, 0, 10, 8, 8, 4},
    {IO_CAN_SPEED_800, 0, 6, 8, 8, 4},
    {IO_CAN_SPEED_1000, 0, 9, 4, 4, 4},
};

io_can_devices_handle io_can_devices_handle_table[] = {
   {&io_can_bitrates_handle_table[8], {ENABLE, 1u, 0x000007FF, 8u}, {ENABLE, 2u, 0x000007FF, 8u}},
   {&io_can_bitrates_handle_table[8], {ENABLE, 1u, 0x000007FF, 8u}, {ENABLE, 2u, 0x000007FF, 8u}},};

void can_info(){
    int index;
    for(index = 0; index < 2; index++){
        io_can_devices_handle *handle = &io_can_devices_handle_table[index];
        printk_ni("CAN[%d]\n", index+1);
        printk_ni("Bit rate = [%04d] kbit/s\n", handle->bitehandle->bitrate);
        printk_ni("=====Tx=====\n");
        printk_ni("IDE : [%x], ID : [%08x], MASK : [%08x], Datalen : [%x]\n"
                ,handle->txmsgbox.ideflag
                ,handle->txmsgbox.id
                ,handle->txmsgbox.mask
                ,handle->txmsgbox.datalength);
        printk_ni("=====Rx=====\n");
        printk_ni("IDE : [%x], ID : [%08x], MASK : [%08x], Datalen : [%x]\n"
                ,handle->rxmsgbox.ideflag
                ,handle->rxmsgbox.id
                ,handle->rxmsgbox.mask
                ,handle->rxmsgbox.datalength);
    }
}

int can_get_index(canBASE_t *handle)
{
    int index;
    if (handle == canREG1)
        index = 0;
    else if (handle == canREG2)
        index = 1;
    else
        return NULL;

    return index;
}

int can_write(
        void *hnd,
        uint32 msgbox,
        uint8_t * tx_buffer,
        uint8 datalen
    )
{
    int index = (int)(hnd);
    int count = 10;
    int error = 0;
    canBASE_t *can = (canBASE_t *)(can_open(index));
    if (can == NULL)
            return E_INVALID_INPUT;
    /* Wait until Tx pending reques is cleared */
    while (canIsTxMessagePending(can, msgbox)){
        vTaskDelay(1000);
        count--;
        if(count == 0)
            return E_TIMEOUT;
    }
    if(io_can_devices_handle_table[index].txmsgbox.datalength != datalen)
        error = can_ioctl(index, CAN_CTL_DATALENGTH, IO_CAN_TX, datalen, NULL);
    if(error != E_SUCCESS)
        return error;
    canTransmit(can, msgbox, tx_buffer);
    return E_SUCCESS;
}

int can_read(
        void *hnd,
        uint32 msgbox,
        uint8_t * rx_buffer,
        uint8 datalen
    )
{
    int index = (int)hnd;
    int count = 10;
    int error = 0;
    canBASE_t *can = (canBASE_t *)(can_open(index));
    if (can == NULL)
        return E_INVALID_INPUT;

    /* Wait until Rx new message arrived */
    while (!canIsRxMessageArrived(can, msgbox)){
        vTaskDelay(1000);
        count--;
        if(count == 0)
            return E_TIMEOUT;
    }
    if(io_can_devices_handle_table[index].rxmsgbox.datalength != datalen)
        error = can_ioctl(index, CAN_CTL_DATALENGTH, IO_CAN_RX, datalen, NULL);
    if(error != E_SUCCESS)
        return error;
    canGetData(can, msgbox, rx_buffer);
    return E_SUCCESS;
}

void *can_open(int index)
{
    canBASE_t *handle;
    if (index == 0)
        handle = canREG1;
    else if (index == 1)
        handle = canREG2;
    else
        return NULL;

    return (void *)handle;
}

int can_close(void *handle)
{
    return 0;
}

int can_ioctl(void *param1, void *param2, void *param3, void *param4, void *param5)
{
    int index = (int)param1;
    if(!(index == 0 || index == 1))
        return E_INVALID_INPUT;

    int ctrl_type = (io_ctrl_t) param2;
    canBASE_t *can = (canBASE_t *)(can_open(index));
    if((can->IF1STAT & 0x80U) ==0x80U || (can->IF2STAT & 0x80U) ==0x80U){
        return E_BUSY;
    }
    switch (ctrl_type){

    case CAN_CTL_ID:
    {
        int flag = (int)param3;
        uint8 ideflag = (int)param4;
        uint32 id = (uint32)param5;

        if((ideflag == ENABLE && id > IDE)||
           (ideflag == DISABLE && id > IDS))
            return E_INVALID_INPUT;

        if(flag == IO_CAN_TX){
            io_can_devices_handle_table[index].txmsgbox.id = id;
            io_can_devices_handle_table[index].txmsgbox.ideflag = ideflag;
        }
        else if(flag == IO_CAN_RX){
            io_can_devices_handle_table[index].rxmsgbox.id = id;
            io_can_devices_handle_table[index].rxmsgbox.ideflag = ideflag;
        }
        else
            return E_INVALID_INPUT;
        break;
    }

    case CAN_CTL_MASK:
    {
        int flag = (int)param3;
        uint32 mask = (uint32)param4;
        if(flag == IO_CAN_TX)
            io_can_devices_handle_table[index].txmsgbox.mask = mask;
        else if(flag == IO_CAN_RX)
            io_can_devices_handle_table[index].rxmsgbox.mask = mask;
        else
            return E_INVALID_INPUT;
        break;
    }

    case CAN_CTL_BAUDRATE:
    {
        int baud = (int)param3;
        if (baud !=  IO_CAN_SPEED_10
            && baud != IO_CAN_SPEED_50
            && baud != IO_CAN_SPEED_100
            && baud != IO_CAN_SPEED_125
            && baud != IO_CAN_SPEED_200
            && baud != IO_CAN_SPEED_250
            && baud != IO_CAN_SPEED_500
            && baud != IO_CAN_SPEED_800
            && baud != IO_CAN_SPEED_1000)
            return E_INVALID_INPUT;

        int i;
        for(i = 0; i < 9; i++){
            if(io_can_bitrates_handle_table[i].bitrate == baud){
                io_can_devices_handle_table[index].bitehandle = &io_can_bitrates_handle_table[i];
                break;
            }
        }
        break;
    }

    case CAN_CTL_DATALENGTH:
    {
        int flag = (int)param3;
        uint8 datalen = (uint8)param4;
        if(datalen < 0 || datalen > 8)
            return E_INVALID_INPUT;

        if(flag == IO_CAN_TX)
            io_can_devices_handle_table[index].txmsgbox.datalength = datalen;
        else if(flag == IO_CAN_RX)
            io_can_devices_handle_table[index].rxmsgbox.datalength = datalen;
        else
            return E_INVALID_INPUT;
        break;
    }
    default:
    {
        return(E_NOT_SUPPORT);
    }

    }
    io_can_init(index);
    return E_SUCCESS;
}

void io_can_init(int index)
{
    canBASE_t *can = (canBASE_t *)(can_open(index));

    can->CTL = (uint32)0x00000200U
                 | (uint32)0x00000000U
                 | (uint32)((uint32)0x00000005U  << 10U)
                 | (uint32)0x00020043U;

    /** - Clear all pending error flags and reset current status */
    can->ES |= 0xFFFFFFFFU;

    /** - Assign interrupt level for messages */
    can->INTMUXx[0U] = (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U;

    can->INTMUXx[1U] = (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U
                         | (uint32)0x00000000U;

    /** - Setup auto bus on timer period */
    can->ABOTR = (uint32)98133U;

    /** - Initialize message 1
    *     - Wait until IF1 is ready for use
    *     - Set message mask
    *     - Set message control word
    *     - Set message arbitration
    *     - Set IF1 control byte
    *     - Set IF1 message number
    */
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Potentially infinite loop found - Hardware Status check for execution sequence" */
    while ((can->IF1STAT & 0x80U) ==0x80U)
    {
    } /* Wait */

    if(io_can_devices_handle_table[index].txmsgbox.ideflag == 1){
        can->IF1MSK  = 0xC0000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].txmsgbox.mask & (uint32)IDE) << (uint32)0U);
        can->IF1ARB  = (uint32)0x80000000U | (uint32)0x40000000U | (uint32)0x20000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].txmsgbox.id & (uint32)IDE) << (uint32)0U);
        can->IF1MCTL = 0x00001000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)io_can_devices_handle_table[index].txmsgbox.datalength;
    }
    else{
        can->IF1MSK  = 0xC0000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].txmsgbox.mask & (uint32)IDS) << (uint32)18U);
        can->IF1ARB  = (uint32)0x80000000U | (uint32)0x00000000U | (uint32)0x20000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].txmsgbox.id & (uint32)IDS) << (uint32)18U);
        can->IF1MCTL = 0x00001000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)io_can_devices_handle_table[index].txmsgbox.datalength;
    }
    can->IF1CMD  = (uint8) 0xF8U;
    can->IF1NO   = 1U;

    /** - Initialize message 2
    *     - Wait until IF2 is ready for use
    *     - Set message mask
    *     - Set message control word
    *     - Set message arbitration
    *     - Set IF2 control byte
    *     - Set IF2 message number
    */
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Potentially infinite loop found - Hardware Status check for execution sequence" */
    while ((can->IF2STAT & 0x80U) ==0x80U)
    {
    } /* Wait */

    if(io_can_devices_handle_table[index].rxmsgbox.ideflag == 1){
        can->IF2MSK  = 0xC0000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].rxmsgbox.mask & (uint32)IDE) << (uint32)0U);
        can->IF2ARB  = (uint32)0x80000000U | (uint32)0x40000000U | (uint32)0x00000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].rxmsgbox.id & (uint32)IDE) << (uint32)0U);
        can->IF2MCTL = 0x00001000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)io_can_devices_handle_table[index].rxmsgbox.datalength;
    }
    else{
        can->IF2MSK  = 0xC0000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].rxmsgbox.mask & (uint32)IDS) << (uint32)18U);
        can->IF2ARB  = (uint32)0x80000000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)((uint32)((uint32)io_can_devices_handle_table[index].rxmsgbox.id & (uint32)IDS) << (uint32)18U);
        can->IF2MCTL = 0x00001000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)0x00000000U | (uint32)io_can_devices_handle_table[index].rxmsgbox.datalength;
    }
    can->IF2CMD  = (uint8) 0xF8U;
    can->IF2NO   = 2U;

    /** - Setup IF1 for data transmission
    *     - Wait until IF1 is ready for use
    *     - Set IF1 control byte
    */
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Potentially infinite loop found - Hardware Status check for execution sequence" */
    while ((can->IF1STAT & 0x80U) ==0x80U)
    {
    } /* Wait */
    can->IF1CMD  = 0x87U;

    /** - Setup IF2 for reading data
    *     - Wait until IF1 is ready for use
    *     - Set IF1 control byte
    */
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Potentially infinite loop found - Hardware Status check for execution sequence" */
    while ((can->IF2STAT & 0x80U) ==0x80U)
    {
    } /* Wait */
    can->IF2CMD = 0x17U;

    /** - Setup bit timing
    *     - Setup baud rate prescaler extension
    *     - Setup TSeg2
    *     - Setup TSeg1
    *     - Setup sample jump width
    *     - Setup baud rate prescaler
    */
    can->BTR = (uint32)((uint32)io_can_devices_handle_table[index].bitehandle->btre << 16U) |
                   (uint32)((uint32)(io_can_devices_handle_table[index].bitehandle->tseg2 - 1U) << 12U) |
                   (uint32)((uint32)((1U + io_can_devices_handle_table[index].bitehandle->tseg1) - 1U) << 8U) |
                   (uint32)((uint32)(io_can_devices_handle_table[index].bitehandle->sjw - 1U) << 6U) |
                   (uint32)io_can_devices_handle_table[index].bitehandle->btr;
//    can->BTR = (uint32)((uint32)0U << 16U) |
//               (uint32)((uint32)(4U - 1U) << 12U) |
//               (uint32)((uint32)((1U + 4U) - 1U) << 8U) |
//               (uint32)((uint32)(4U - 1U) << 6U) |
//               (uint32)9U;


     /** - CAN1 Port output values */
    can->TIOC =  (uint32)((uint32)1U  << 18U )
                   | (uint32)((uint32)0U  << 17U )
                   | (uint32)((uint32)0U  << 16U )
                   | (uint32)((uint32)1U  << 3U )
                   | (uint32)((uint32)0U  << 2U )
                   | (uint32)((uint32)0U << 1U );

    can->RIOC =  (uint32)((uint32)1U  << 18U )
                   | (uint32)((uint32)0U  << 17U )
                   | (uint32)((uint32)0U  << 16U )
                   | (uint32)((uint32)1U  << 3U )
                   | (uint32)((uint32)0U  << 2U )
                   | (uint32)((uint32)0U <<1U );

    /** - Leave configuration and initialization mode  */
    can->CTL &= ~(uint32)(0x00000041U);
}
