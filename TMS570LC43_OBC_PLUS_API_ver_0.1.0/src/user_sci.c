/*
 * user_sci.c
 *
 *  Created on: 2019¦~5¤ë31¤é
 *      Author: kusoyao
 */
#include "user_sci.h"
#include "global.h"
#include <stdio.h>

// Addresses of SCI 8-bit TX/Rx data
#if ((__little_endian__ == 1) || (__LITTLE_ENDIAN__ == 1))
#define SCI1_TX_ADDR ((uint32_t)(&(sciREG1->TD)))
#define SCI1_RX_ADDR ((uint32_t)(&(sciREG1->RD)))
//#define SCI2_TX_ADDR ((uint32_t)(&(sciREG2->TD)))
//#define SCI2_RX_ADDR ((uint32_t)(&(sciREG2->RD)))
//#define SCI3_TX_ADDR ((uint32_t)(&(sciREG3->TD)))
//#define SCI3_RX_ADDR ((uint32_t)(&(sciREG3->RD)))
//#define SCI4_TX_ADDR ((uint32_t)(&(sciREG4->TD)))
//#define SCI4_RX_ADDR ((uint32_t)(&(sciREG4->RD)))
#else
#define SCI1_TX_ADDR ((uint32_t)(&(sciREG1->TD)) + 3)
#define SCI1_RX_ADDR ((uint32_t)(&(sciREG1->RD)) + 3)
//#define SCI2_TX_ADDR ((uint32_t)(&(sciREG2->TD)) + 3)
//#define SCI2_RX_ADDR ((uint32_t)(&(sciREG2->RD)) + 3)
//#define SCI3_TX_ADDR ((uint32_t)(&(sciREG3->TD)) + 3)
//#define SCI3_RX_ADDR ((uint32_t)(&(sciREG3->RD)) + 3)
//#define SCI4_TX_ADDR ((uint32_t)(&(sciREG4->TD)) + 3)
//#define SCI4_RX_ADDR ((uint32_t)(&(sciREG4->RD)) + 3)
#endif

//DMA Request Table: See datasheet page 124 (Table 6-41. DMA Request Line Connection)
#define DMA_SCI1_TX  DMA_REQ29 // LIN1
#define DMA_SCI1_RX  DMA_REQ28 // LIN1
//#define DMA_SCI2_TX  DMA_REQ41 // LIN2
//#define DMA_SCI2_RX  DMA_REQ40 // LIN2
//#define DMA_SCI3_TX  DMA_REQ31
//#define DMA_SCI3_RX  DMA_REQ30
//#define DMA_SCI4_TX  DMA_REQ43
//#define DMA_SCI4_RX  DMA_REQ42
#define SCI_SET_TX_INT      (1<<8)
#define SCI_SET_RX_INT      (1<<9)

#define SCI_SET_TX_DMA      (1<<16)
#define SCI_SET_RX_DMA      (1<<17)
#define SCI_SET_RX_DMA_ALL  (1<<18)

#define NUM_SCI 1 //4

//static uint32_t SCI_TX_ADDR[NUM_SCI] = { SCI1_TX_ADDR, SCI2_TX_ADDR, SCI3_TX_ADDR, SCI4_TX_ADDR};
//static uint32_t SCI_RX_ADDR[NUM_SCI] = { SCI1_RX_ADDR, SCI2_RX_ADDR, SCI3_RX_ADDR, SCI4_RX_ADDR};
//static dmaRequest_t DMA_REQ_TX[NUM_SCI] = { DMA_SCI1_TX, DMA_SCI2_TX, DMA_SCI3_TX, DMA_SCI4_TX};
//static dmaRequest_t DMA_REQ_RX[NUM_SCI] = { DMA_SCI1_RX, DMA_SCI2_RX, DMA_SCI3_RX, DMA_SCI4_RX};

static uint32_t SCI_TX_ADDR[NUM_SCI] = { SCI1_TX_ADDR};
static uint32_t SCI_RX_ADDR[NUM_SCI] = { SCI1_RX_ADDR};
static dmaRequest_t DMA_REQ_TX[NUM_SCI] = { DMA_SCI1_TX};
static dmaRequest_t DMA_REQ_RX[NUM_SCI] = { DMA_SCI1_RX};
//int RxOverflow = 0;

static int get_index(sciBASE_t *sci)
{
    uint32_t index = (sci  == sciREG1) ? 0U :
                   ((sci == sciREG2) ? 1U :
                   ((sci == sciREG3) ? 2U : 3U));
    return index;
}

//void sciPollTx(sciBASE_t *sci, uint8_t *text, uint32_t length)
//{
//    while(length--)
//    {
//        sciSendByte(sci,*text++);
//    }
//}
//
//void sciPollTxString(sciBASE_t *sci, uint8_t *text)
//{
//    while(*text != 0)
//    {
//        sciSendByte(sci, *text++);
//    }
//}
//
//void sciPollRx(sciBASE_t *sci, uint8_t *text, uint32_t length)
//{
//   while(length--) {
//   		*text++ = sciReceiveByte_Liscotech(sci);
//    }
//}

scidma_handle_t *sciDmaTxInit(sciBASE_t *sci, bool handle_cache)
{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();
    scidma_handle_t *sc = (scidma_handle_t *)pvPortMalloc(sizeof(scidma_handle_t));

    ASSERT(sc != NULL);

    sc->sci = sci;
    sc->handle_cache = handle_cache;
    vSemaphoreCreateBinary(sc->sem);
    sc->src = 0;
    sc->total_len = 0;
    sc->sended_len = 0;

    uint32_t index = get_index(sc->sci);

    sc->dma_channel = alloc_dma_channel(DMA_LOW_PRIORITY);
    ASSERT(sc->dma_channel != -1);

    // Configure control packet
    sc->dma_ctrlpkt.SADD      = 0;                    // source address
    sc->dma_ctrlpkt.DADD      = SCI_TX_ADDR[index];   // destination address
    sc->dma_ctrlpkt.CHCTRL    = 0;                    // channel control
    sc->dma_ctrlpkt.FRCNT     = 0;                    // frame count
    sc->dma_ctrlpkt.ELCNT     = 1;                    // element count
    sc->dma_ctrlpkt.FRSOFFSET = 0;                    // frame source offset
    sc->dma_ctrlpkt.FRDOFFSET = 0;                    // frame destination offset
    sc->dma_ctrlpkt.ELSOFFSET = 0;                    // element source offset
    sc->dma_ctrlpkt.ELDOFFSET = 0;                    // element destination offset
    sc->dma_ctrlpkt.PORTASGN  = PORTA_READ_PORTB_WRITE;
    sc->dma_ctrlpkt.RDSIZE    = ACCESS_8_BIT;         // read size
    sc->dma_ctrlpkt.WRSIZE    = ACCESS_8_BIT;         // write size
    sc->dma_ctrlpkt.TTYPE     = FRAME_TRANSFER;       // transfer type
    sc->dma_ctrlpkt.ADDMODERD = ADDR_INC1;            // address mode read
    sc->dma_ctrlpkt.ADDMODEWR = ADDR_FIXED;           // address mode write
    sc->dma_ctrlpkt.AUTOINIT  = AUTOINIT_OFF;         // autoinit


    dmaReqAssign(sc->dma_channel, DMA_REQ_TX[index]);
    dmaEnableInterrupt(sc->dma_channel, BTC, DMA_INTA);
    register_dma_intr(BTC, sc->dma_channel, sciDMANotificationTX, (void *)sc);
    portRESET_PRIVILEGE( xRunningPrivileged );

    return sc;
}

int sciDmaTx(scidma_handle_t *sc, const uint8_t *src, uint32_t size)
{
    ASSERT(sc != NULL);
    if( size == 0)
        return 0;

    if( xSemaphoreTake(sc->sem, ( TickType_t ) portMAX_DELAY) != pdTRUE )
    {
        // DMA is busying
        return -1;
    }

    BaseType_t xRunningPrivileged = prvRaisePrivilege();
    if(sc->handle_cache)
    {
        coreCleanDCByAddress(src, size);
    }

    sc->src = src;
    sc->total_len = size;

    // maximum FRCNT & ELCNT is 0x1FFF, for SCI ELCNT is limit to 1
    sc->dma_ctrlpkt.SADD  = src;
    if(size > 0x1FFF)
    {
        sc->dma_ctrlpkt.FRCNT = 0x1FFF;
        sc->sended_len = 0x1FFF;
        //will remain some byte , do the transfer in isr
    }
    else
    {
        sc->dma_ctrlpkt.FRCNT = size;
        sc->sended_len = size;
    }

    dmaSetCtrlPacket(sc->dma_channel, sc->dma_ctrlpkt);
    dmaSetChEnable(sc->dma_channel, DMA_HW);
    sc->sci->SETINT |= SCI_SET_TX_DMA;

    portRESET_PRIVILEGE( xRunningPrivileged );
    return 0;
}

int sciDmaTxSync(scidma_handle_t *sc)
{

    if( xSemaphoreTake(sc->sem, ( TickType_t ) portMAX_DELAY) != pdTRUE )
    {
        // DMA is busying
        return -1;
    }
    if( xSemaphoreGive(sc->sem) != pdTRUE )
    {
        //Give it back
        return -1;
    }
}

void sciDMANotificationTX(void *parameter)
{
    scidma_handle_t *sc = parameter;
    sciBASE_t *sci = sc->sci;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(sc->sended_len != sc->total_len)
    {
        sc->dma_ctrlpkt.SADD  = sc->src + sc->sended_len;
        uint32_t tosend = sc->total_len - sc->sended_len;
        if(tosend > 0x1FFF)
        {
            sc->dma_ctrlpkt.FRCNT = 0x1FFF;
        }
        else
        {
            sc->dma_ctrlpkt.FRCNT = tosend;
        }
        sc->sended_len += sc->dma_ctrlpkt.FRCNT;

        dmaSetCtrlPacket(sc->dma_channel, sc->dma_ctrlpkt);
        dmaSetChEnable(sc->dma_channel, DMA_HW);
        sc->sci->SETINT |= SCI_SET_TX_DMA;
    }
    else
    {
        xSemaphoreGiveFromISR(sc->sem, &xHigherPriorityTaskWoken);
        sci->CLEARINT = SCI_SET_TX_DMA;
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

//void sciNotification(sciBASE_t *sci, uint32 flags)
//{
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    if((sci == CONSOLE_PORT) && (flags == SCI_RX_INT))
//    {
//        if( xQueueSendFromISR( qin, &SCI1RXBUF, &xHigherPriorityTaskWoken ) != pdTRUE)
//            RxOverflow++;
//
//        sciReceive(sci, 1, &SCI1RXBUF); //start next receive
//        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//    }
//    // if((sci == CONSOLE_PORT || sci == STARTRACKER_PORT) && (flags == SCI_RX_INT))
//    //{
//    //    if( xQueueSendFromISR( qin, &SCI1RXBUF, &xHigherPriorityTaskWoken ) != pdTRUE)
//    //        RxOverflow++;
//    //
//    //    sciReceive(sci, 1, &SCI1RXBUF); //start next receive
//    //    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//    //}
//}
