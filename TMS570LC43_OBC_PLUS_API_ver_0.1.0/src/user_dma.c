/*
 * user_dma.c
 *
 *  Created on: 2019¦~1¤ë16¤é
 *      Author: kusoyao
 */

#include "user_dma.h"
#include "global.h"

/* four interrupt and 32 channel */
static func_ptr dma_intr_table[4][32];
static void *dma_intr_parameter[4][32];
static uint32_t dma_channel_used = 0;

void register_dma_intr(dmaInterrupt_t inttype, uint32 channel, func_ptr ptr, void *parameter)
{
    dma_intr_table[inttype][channel] = ptr;
    dma_intr_parameter[inttype][channel] = parameter;
}

void dmaGroupANotification(dmaInterrupt_t inttype, uint32 channel)
{
    if(dma_intr_table[inttype][channel] != 0)
        (*dma_intr_table[inttype][channel])(dma_intr_parameter[inttype][channel]);
}

void dmaSetChReset(dmaChannel_t channel, dmaTriggerType_t type)
{
    if(type == DMA_HW)
    {
        dmaREG->HWCHENAR = (uint32)1U << channel;
    }
    else
    {
        dmaREG->SWCHENAR = (uint32)1U << channel;
    }

}

dmaChannel_t alloc_dma_channel(dma_priority_t priority)
{
    /* channel 0 is the highest priority */
    taskENTER_CRITICAL();
    int ch;
    uint32_t mask;
    if(priority == DMA_HIGH_PRIORITY)
    {
        for(ch = 0; ch < 16 ; ++ch)
        {
            mask = 1 << ch;
            if((dma_channel_used & mask) == 0)
            {
                dma_channel_used |= mask;
                break;
            }
        }
        if(ch == 16)
            ch = -1;
    }
    else
    {
        for(ch = 16; ch < 32 ; ++ch)
        {
            mask = 1 << ch;
            if((dma_channel_used & mask) == 0)
            {
                dma_channel_used |= mask;
                break;
            }
        }
        if(ch == 32)
            ch = -1;
    }
    taskEXIT_CRITICAL();
    return DMA_CH0 + ch;
}
