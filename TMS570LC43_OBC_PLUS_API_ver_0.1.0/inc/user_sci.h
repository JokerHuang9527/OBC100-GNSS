/*
 * user_sci.h
 *
 *  Created on: 2019¦~5¤ë31¤é
 *      Author: kusoyao
 */

#ifndef INC_USER_SCI_H_
#define INC_USER_SCI_H_

#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_sys_dma.h"
#include "HL_sci.h"
#include "HL_sys_cache.h"

#include "user_dma.h"

void sciPollTx(sciBASE_t *sci, uint8_t *text, uint32_t length);
void sciPollTxString(sciBASE_t *sci, uint8_t *text);
void sciPollRx(sciBASE_t *sci, uint8_t *text, uint32_t length);

typedef struct scidma_handle {
    sciBASE_t *sci;
    dmaChannel_t dma_channel; // used to transfer large block of data
    g_dmaCTRL dma_ctrlpkt;
    bool handle_cache;
    SemaphoreHandle_t sem;
    uint8_t *src; // data to be send;
    uint32_t total_len; // data length to be send
    uint32_t sended_len; // data length sending / sended
} scidma_handle_t;

scidma_handle_t *sciDmaTxInit(sciBASE_t *sci, bool handle_cache);
int sciDmaTx(scidma_handle_t *sc, const uint8_t *src, uint32_t size);
int sciDmaTxSync(scidma_handle_t *sc);
void sciDMANotificationTX(void *parameter);

#endif /* INC_USER_SCI_H_ */
