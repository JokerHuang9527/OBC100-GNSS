/*
 * task_esm.c
 *
 *  Created on: 2019¦~11¤ë21¤é
 *      Author: kusoyao
 */


#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"
#include "os_queue.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_esm.h"

#include "utils.h"
#include "user_sci.h"
#include "global.h"
#include "task_esm.h"

extern QueueHandle_t queue_esm; // initial at global_init
static uint32_t esm_count = 0;

void vTask_esm(void *pvParameters)
{
    //IF warm reset, ESMSR1, ESMSR4, ESMSR7, ESMSSR2, ESMSR3 and ESMEPSR register values remains un-changed.
    //printk("ESM GROUP 0: SR1 %08x, SR4 %08x, SR7 %08x\n", esmREG->SR1[0], esmREG->SR4[0], esmREG->SR7[0]);
    //printk("ESM GROUP 1: SSR2 %08x\n", esmREG->SSR2);
    //printk("ESM GROUP 2: SR3 %08x\n", esmREG->SR1[2]);
    //printk("ERROR PIN %d\n", esmREG->EPSR);

    if(esmREG->EPSR == 0)
        esmTriggerErrorPinReset();

    //ESM GROUP Definition in datasheet spns195c page 129, not in TRM

    //enable all Error interrupt
    //esmEnableInterrupt(0xFFFFFFFFFFFFFFFF);
    //esmEnableInterruptUpper(0xFFFFFFFFFFFFFFFF);

    uint8_t channel;
    TickType_t timeoutInf = portMAX_DELAY;
    TickType_t timeout1000 = 1000;

    while(1)
    {
        if( xQueueReceive( queue_esm, &channel, timeout1000 ) )
        {
            //do something process
            printk("ESM: %d\n", channel);
            houseKeepingData.esmStatus = channel;
            houseKeepingData.errorTimes++;
        }
    }

    vTaskDelete(NULL);
}

void esmGroup1Notification(esmBASE_t *esm, uint32 channel)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ch = channel;
    //96 Group1 (low severity) channels with configurable interrupt generation and configurable ERROR pin behavior
    //channel 0 ~ 95
    esm_count++;
    esmTriggerErrorPinReset();
    xQueueSendToBackFromISR( queue_esm, &ch, &xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void esmGroup2Notification(esmBASE_t *esm, uint32 channel)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ch = channel + 96;
    //32 Group2 (high severity) channels with predefined interrupt generation and predefined ERROR pin behavior
    //channel 0 ~ 31 , High Level only
    esm_count++;
    esmTriggerErrorPinReset();
    xQueueSendToBackFromISR( queue_esm, &ch, &xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void esmGroup3Notification(esmBASE_t *esm, uint32 channel)
{
    /*
    32 Group3 (high severity) channels with no interrupt generation and predefined ERROR pin behavior.
    These channels have no interrupt response as they are reserved for CPU based
    diagnostics that generate aborts directly to the CPU.
    */
    //never be called..
}
