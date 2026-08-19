/*
 * task_watchdog.c
 *
 *  Created on: 2019¦~12¤ë27¤é
 *      Author: kusoyao
 */
#include "FreeRTOS.h"
#include "os_task.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_rti.h"

#include "user_sci.h"
#include "global.h"
#include "task_watchdog.h"

#define RTICLK ( configCPU_CLOCK_HZ )

#define WATCHDOG_EXPIRED_TIME 16 // 0 ~ 16.777216 second
#define WATCHDOG_PRELOAD_VALUE (((WATCHDOG_EXPIRED_TIME*RTICLK) >> 13) - 1)

void vTask_watchdog(void *pvParameters)
{
    //Watchdog init
    dwdInit(rtiREG1, WATCHDOG_PRELOAD_VALUE);
    dwdCounterEnable(rtiREG1);

    while(1)
    {
        vTaskDelay((WATCHDOG_EXPIRED_TIME - 1) * 1000); // should large than zero
        printk("WATCHDOG RESET\n");
        dwdReset(rtiREG1);
    }

    vTaskDelete(NULL);
}
