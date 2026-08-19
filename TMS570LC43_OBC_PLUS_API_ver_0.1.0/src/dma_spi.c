/** @file HL_sys_main.c
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/*
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */
#include "dma_spi.h"
int F_SIZE;
int E_SIZE;

g_dmaCTRL g_dmaCTRLPKT_TX, g_dmaCTRLPKT_RX __SRAM2_SECTION__;     /* dma control packet configuration stack */


int mibspi_dma_transfer(void *hnd, uint8* txsadd, uint8* rxsadd, uint16 length){
    uint16 i;
    int index = (int)hnd;

    uint16 tgPSTART;

    io_spi_devices_handle* handle = (io_spi_devices_handle *)(spi_open(index));

    E_SIZE = get_mibspi_tg_size(handle->spi, handle->group);
    F_SIZE = length;


    tgPSTART = (handle->spi->TGCTRL[handle->group] >> 8) & 0xFF;

    /* - configuring dma control packets   */
    dmaConfigCtrlTxPacket((uint32)txsadd,
                          (uint32)&(handle->ram->tx[tgPSTART].data),
                          E_SIZE,
                          F_SIZE);

    dmaConfigCtrlRxPacket((uint32)&(handle->ram->rx[tgPSTART].data),
                          (uint32)rxsadd,
                          E_SIZE,
                          F_SIZE);
    /*****************************************************************/
    /* upto 32 control packets are supported. */
    dmaReqAssign(handle->rx.channel, handle->rx.request);
    dmaReqAssign(handle->tx.channel, handle->tx.request);
    /* - setting dma control packets */
    dmaSetCtrlPacket(handle->rx.channel, g_dmaCTRLPKT_RX);
    dmaSetCtrlPacket(handle->tx.channel, g_dmaCTRLPKT_TX);
    /* - setting the dma channel to trigger on h/w request */
    dmaSetChEnable(handle->rx.channel, DMA_HW);
    dmaSetChEnable(handle->tx.channel, DMA_HW);
    /* - configuring the mibspi dma , channel 0 , tx line -0 , rxline -1     */
    /* - refer to the device data sheet dma request source for mibspi tx/rx  */
    mibspiDmaConfig(handle->spi, handle->group, 0, 1, tgPSTART);
    mibspiPmodeSet(handle->spi, PMODE_NORMAL, handle->DFMT);
    /*****************************************************************/

    dmaEnableInterrupt(handle->rx.channel, BTC, DMA_INTA);
    register_dma_intr(BTC, handle->rx.channel, spiDMANotificationRX, (void *)handle);

    dmaEnable();


    /* - start the mibspi transfer tg */
    mibspiTransfer(handle->spi, handle->group);
    handle->state = 1;

     return 0;
}

void mibspiDmaConfig(mibspiBASE_t *mibspi,
                     uint32 channel,
                     uint32 txchannel,
                     uint32 rxchannel,
                     uint16 tgPSTART)
{
    uint32 bufid;


    bufid = tgPSTART + E_SIZE - 1;

    /* setting transmit and receive channels */
    mibspi->DMACTRL[channel] |= (rxchannel << 20) | (txchannel << 16);

    if (F_SIZE > 1) {
         mibspi->TGCTRL[channel] &= 0xBFFFFFFF; // Disable ONESHOT
    } else {
         mibspi->TGCTRL[channel] |= 0x40000000; // Enable ONESHOT
    }

    /* enabling transmit and receive dma */
    mibspi->DMACTRL[channel] |=  0x8000C000;

    /* setting Initial Count of DMA transfers and the buffer utilized for DMA transfer */
    mibspi->DMACTRL[channel] |=  (bufid<<24);

    /* Enable Large count transfer */
    mibspi->DMACNTLEN = 0x1;
    mibspi->DMACOUNT[channel] = (F_SIZE - 1) << 16;

}

void spiDMANotificationRX(void *parameter)
{
    io_spi_devices_handle* handle = parameter;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int index;
    if(handle->spi == mibspiREG2)
        index = 0;
    else if(handle->spi == mibspiREG3)
        index = 1;
    else if(handle->spi == mibspiREG5)
        index = 2;
    if(handle->blockflag != 0)
        printk_ni("SPI DEVICES[%d]:Transfer completed.\n", index + 1);
    handle->state = 0;

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void dmaConfigCtrlTxPacket(uint32 sadd,uint32 dadd,uint16 ElmntCnt, uint16 FrameCnt)
{
    g_dmaCTRLPKT_TX.SADD      = sadd;              /* source address             */
    g_dmaCTRLPKT_TX.DADD      = dadd;              /* destination  address       */
    g_dmaCTRLPKT_TX.CHCTRL    = 0;                 /* channel control            */
    g_dmaCTRLPKT_TX.FRCNT     = FrameCnt;                 /* frame count                */
    g_dmaCTRLPKT_TX.ELCNT     = ElmntCnt;             /* element count              */
    g_dmaCTRLPKT_TX.ELDOFFSET = 4;                 /* element destination offset */
    g_dmaCTRLPKT_TX.ELSOFFSET = 0;                 /* element source offset */
    g_dmaCTRLPKT_TX.FRDOFFSET = 0;                 /* frame destination offset   */
    g_dmaCTRLPKT_TX.FRSOFFSET = 0;                 /* frame destination offset   */
    g_dmaCTRLPKT_TX.PORTASGN  = PORTA_READ_PORTB_WRITE;                 /* port b                     */
    g_dmaCTRLPKT_TX.RDSIZE    = ACCESS_16_BIT;     /* read size                  */
    g_dmaCTRLPKT_TX.WRSIZE    = ACCESS_16_BIT;     /* write size                 */
    g_dmaCTRLPKT_TX.TTYPE     = FRAME_TRANSFER ;   /* transfer type              */
    g_dmaCTRLPKT_TX.ADDMODERD = ADDR_INC1;         /* address mode read          */
    g_dmaCTRLPKT_TX.ADDMODEWR = ADDR_OFFSET;       /* address mode write         */
    g_dmaCTRLPKT_TX.AUTOINIT  = AUTOINIT_OFF;       /* autoinit                   */
}

void dmaConfigCtrlRxPacket(uint32 sadd,uint32 dadd,uint16 ElmntCnt, uint16 FrameCnt)
{
    g_dmaCTRLPKT_RX.SADD      = sadd;              /* source address             */
    g_dmaCTRLPKT_RX.DADD      = dadd;              /* destination  address       */
    g_dmaCTRLPKT_RX.CHCTRL    = 0;                 /* channel control            */
    g_dmaCTRLPKT_RX.FRCNT     = FrameCnt;                 /* frame count                */
    g_dmaCTRLPKT_RX.ELCNT     = ElmntCnt;             /* element count              */
    g_dmaCTRLPKT_RX.ELDOFFSET = 0;                 /* element destination offset */
    g_dmaCTRLPKT_RX.ELSOFFSET = 4;                 /* element source offset */
    g_dmaCTRLPKT_RX.FRDOFFSET = 0;                 /* frame destination offset   */
    g_dmaCTRLPKT_RX.FRSOFFSET = 0;                 /* frame source offset   */
    g_dmaCTRLPKT_RX.PORTASGN  = PORTB_READ_PORTA_WRITE;
    g_dmaCTRLPKT_RX.RDSIZE    = ACCESS_16_BIT;     /* read size                  */
    g_dmaCTRLPKT_RX.WRSIZE    = ACCESS_16_BIT;     /* write size                 */
    g_dmaCTRLPKT_RX.TTYPE     = FRAME_TRANSFER ;   /* transfer type              */
    g_dmaCTRLPKT_RX.ADDMODERD = ADDR_OFFSET;       /* address mode read          */
    g_dmaCTRLPKT_RX.ADDMODEWR = ADDR_INC1;         /* address mode write         */
    g_dmaCTRLPKT_RX.AUTOINIT  = AUTOINIT_OFF;       /* autoinit                   */
}

int get_mibspi_tg_size(mibspiBASE_t *spi, int group)
{
    uint32 start  = (spi->TGCTRL[group] >> 8U) & 0xFFU;
    uint32 end    = (group == 7U) ? (((spi->LTGPEND & 0x00007F00U) >> 8U) + 1U) : ((spi->TGCTRL[group+1U] >> 8U) & 0xFFU);

    if (end == 0U) {end = 128U;}
    return end - start;
}
