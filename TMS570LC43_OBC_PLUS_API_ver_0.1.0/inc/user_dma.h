/*
 * user_dma.h
 *
 *  Created on: 2019¦~1¤ë16¤é
 *      Author: kusoyao
 */

#ifndef INC_USER_DMA_H_
#define INC_USER_DMA_H_

#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_sys_dma.h"

typedef enum dma_priority
{
    DMA_HIGH_PRIORITY = 0U,
    DMA_LOW_PRIORITY
} dma_priority_t;

typedef void (*func_ptr)(void *parameter);
void register_dma_intr(dmaInterrupt_t inttype, uint32 channel, func_ptr ptr, void *parameter);
void dmaSetChReset(dmaChannel_t channel, dmaTriggerType_t type);
dmaChannel_t alloc_dma_channel(dma_priority_t priority);

#endif
