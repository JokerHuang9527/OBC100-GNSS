/*
 * task_i2c.h
 *
 *  Created on: 2021¦~7¤ë27¤é
 *      Author: User
 */
/*
#ifndef INC_TASK_I2C_H_
#define INC_TASK_I2C_H_

#include "time.h"
#include "ff_time.h"
#include "FreeRTOSTIMEConfig.h"
#include "HL_reg_i2c.h"

void vTask_i2c_initRTC(void *param);
void vTask_i2c_setRTC(FF_TimeStruct_t *TimeBuf);
void vTask_i2c_getRTC(FF_TimeStruct_t *TimeBuf);
void vTask_i2c_set_system_time();
int EXTERNAL_RTC_Init();
int EXTERNAL_RTC_SetTime(time_t t);
int EXTERNAL_RTC_GetTime(time_t *t);
int EXTERNAL_RTC_SetDateTime(FF_TimeStruct_t *TimeBuf);
int EXTERNAL_RTC_GetDateTime(FF_TimeStruct_t *TimeBuf);
int I2C_Read_M(short Slave_Add, short Read_Add, short Count, uint8 *buff);
int I2C_Write_M(short Slave_Add, short Write_Add, short Count, uint8 *buff );
#endif /* INC_TASK_I2C_H_ */
