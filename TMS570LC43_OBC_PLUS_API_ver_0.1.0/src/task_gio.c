///*
// * task_gio.c
// *
// *  Created on: 2019¦~12¤ë10¤é
// *      Author: kusoyao
// */
//#include <stdio.h>
//#include <assert.h>
//#include <string.h>
//#include <stdint.h>
//#include <stdlib.h>
//
//#include "FreeRTOS.h"
//#include "os_task.h"
//#include "os_semphr.h"
//#include "os_queue.h"
//
//#include "HL_sys_common.h"
//#include "HL_system.h"
//#include "HL_gio.h"
//
//#include "utils.h"
//#include "user_sci.h"
//#include "global.h"
//#include "UART_API.h"
//
//extern QueueHandle_t queue_gio; // initial at global_init
//uint8_t gio3_lock = 0;
//uint8_t gio4_lock = 0;
//
//void tk2_reset_sf2_power()
//{
//    printk("=====SF2 reset!!!=====\n");
//    //turn off sf2 power
//    gioSetBit(gioPORTA, 0, 1);
//    //@@watch ADC 10 then close gioSetBit(gioPORTA, 1, 1);
//    vTaskDelay(10000);
//    //turn on sf2 power
//    gioSetBit(gioPORTA, 0, 0);
//    gioSetBit(gioPORTA, 1, 0);
//    vTaskDelay(1000);
//}
//
//void vTask_gio(void *pvParameters)
//{
//    uint32_t intr_limit = 100;
//    //uint32_t gio_intr_count[16] = {0};
//    //enable all Error interrupt
//    //gioEnableNotification(gioPORTB, 0);
//    //gioEnableNotification(gioPORTB, 1);
//    //gioEnableNotification(gioPORTB, 2);
// 	gioEnableNotification(gioPORTB, 6);
// 	gioEnableNotification(gioPORTB, 3);
// 	gioEnableNotification(gioPORTB, 4);
//
//    uint8_t channel;
//    TickType_t timeoutInf = portMAX_DELAY;
//    TickType_t timeout1000 = 1000;
//
//	//vTaskDelay(1000);
//
//	/* control gpio to turn on SF2 board power */
//    vTaskDelay(1000);
//#ifdef TK2_OBC
//    printk("Trun On TK2 OBC\n");
//
//    gioSetBit(gioPORTA, 0, 0);
//    vTaskDelay(100);
//    gioSetBit(gioPORTA, 1, 0);
//#else
//    gioSetBit(gioPORTA, 1, 1);
//#endif
//    vTaskDelay(500);
//#ifdef TK2_OBC
//            uint32_t b;
//            b = gioGetBit(gioPORTB, 0);
//            printk("[%x]\n",b);
//#endif
//
//	if (radiation_test==1) {
//		vTaskDelay(10);
//		printk(" === Radiation Test Mode ===\n");
//		goutmode = 5;
//	}
//
//    while(1)
//    {
//        if( xQueueReceive( queue_gio, &channel, timeoutInf ) )
//        {
//            //do something process
//            //gio_intr_count[channel]++;
//#ifdef TK2_OBC
//            uint32_t b;
//            b = gioGetBit(gioPORTB, 0);
//            printk("[%x]\n",b);
//#endif
//            if(channel < 8)
//            {
//                //GIOA
//#if 0
//                if(gio_intr_count[channel] > intr_limit)
//                {
//                    gioDisableNotification(gioPORTA, channel);
//                    printk("GIOA channel %d interrupt over limit %d, disabled.\n",  channel - 8, gio_intr_count[channel]);
//                }
//#endif
//            }
//            else
//            {
//                //GIOB
//#if 0
//                if(gio_intr_count[channel] > intr_limit)
//                {
//                    gioDisableNotification(gioPORTB, channel - 8);
//                    printk("GIOB channel %d interrupt over limit %d, disabled.\n",  channel - 8, gio_intr_count[channel]);
//                }
//#endif
//                if(channel == 8 ) {
//                    printk("SRAM ECC ERROR\n");
//                } else if(channel == 9) {
//                    printk("OBC_CSC_OC#\n");
//                } else if(channel == 10) {
//                    printk("DSP_ADC1_EVT\n");
//                } else if(channel == 11) {
//					/* PortB gio3 falling edge to trigger*/
//					printk("gioB3 ISR\n");
//					gio3_lock = 0;
//                } else if(channel == 12) {
//					/* PortB gio4 falling edge to trigger*/
//					printk("gioB4 ISR\n");
//					gio4_lock = 0;
//				} else if(channel == 14) {
//					/* gioPortB gio6 is this interrupt input pin, which controls by SF2
//					 * When image sensor(on ths SF2 board) take a picture,
//					 * this pin will be created a high to low pluse.
//					 * high voltage continuous time equals exposure time.
//					 * please using mode 2 0 to show this message(coludn't use printk() in the mode 2 3)
//					 * otherwise will cause CVUART failed
//					 */
//					if (goutmode == 0) {
//						printk("gioB6 ISR\n");
//					}
//				}
//			}
//        }
//    }
//
//    vTaskDelete(NULL);
//}
//
//void gioNotification(gioPORT_t *port, uint32_t bit)
//{
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    uint8_t ch = bit;
//    if(port == gioPORTB)
//        ch += 8;
//
//	if (ch == 11) {
//		if(gio3_lock == 0) {
//			gio3_lock = 1 ;
//    		xQueueSendToBackFromISR( queue_gio, &ch, &xHigherPriorityTaskWoken );
//		}
//	} else if (ch == 12) {
//		if(gio4_lock == 0) {
//			gio4_lock = 1;
//    		xQueueSendToBackFromISR( queue_gio, &ch, &xHigherPriorityTaskWoken );
//		}
//	} else {
//    	xQueueSendToBackFromISR( queue_gio, &ch, &xHigherPriorityTaskWoken );
//	}
//    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//}
