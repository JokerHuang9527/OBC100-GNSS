/*
 * rtc.h
 *
 *  Created on: 2021¦~7¤ë27¤é
 *      Author: User
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include "time.h"
#include "ff_time.h"
#include "FreeRTOSTIMEConfig.h"
#include "HL_reg_i2c.h"

void vTask_initRTC(void *param);
void setRTC(FF_TimeStruct_t *TimeBuf);
void getRTC(FF_TimeStruct_t *TimeBuf);
void set_system_time();
int EXTERNAL_RTC_Init();
int EXTERNAL_RTC_SetTime(time_t t);
int EXTERNAL_RTC_GetTime(time_t *t);
int EXTERNAL_RTC_SetDateTime(FF_TimeStruct_t *TimeBuf);
int EXTERNAL_RTC_GetDateTime(FF_TimeStruct_t *TimeBuf);
#endif /* INC_RTC_H_ */
