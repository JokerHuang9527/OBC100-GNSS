/*
 * rtc.c
 *
 *  Created on: 2021¦~7¤ë27¤é
 *      Author: User
 */
#include <string.h>
#include "stdio.h"
#include "FreeRTOS.h"
#include "os_task.h"
#include "HL_i2c.h"
#include "HL_reg_i2c.h"

#include "rtc.h"
#include "time.h"
#include "ff_time.h"
#include "FreeRTOSTIMEConfig.h"
#include "xioctl.h"





/* Device i2c address */
#define Slave_Address  0x69
/* REGISTER MAP */
#define REGISTER_CONFIG_REG1 0x00
#define REGISTER_CONFIG_REG2 0x01
#define REGISTER_REGISTER_ 0x03
#define REGISTER_INT_EN_REG 0x04
#define REGISTER_INT_STATUS_REG 0x05
#define REGISTER_SECONDS 0x06
#define REGISTER_MINUTES 0x07
#define REGISTER_HOURS 0x08
#define REGISTER_DAY 0x09
#define REGISTER_DATE 0x0a
#define REGISTER_MONTH 0x0b
#define REGISTER_YEAR 0x0c
#define REGISTER_ALM1_SEC 0x0d
#define REGISTER_ALM1_MIN 0x0e
#define REGISTER_ALM1_HRS 0x0f
#define REGISTER_ALM1DAY_DATE 0x10
#define REGISTER_ALM1_MON 0x11
#define REGISTER_ALM1_YEAR 0x12
#define REGISTER_ALM2_MIN 0x13
#define REGISTER_ALM2_HRS 0x14
#define REGISTER_ALM2DAY_DATE 0x15
#define REGISTER_TIMER_COUNT 0x16
#define REGISTER_TIMER_INIT 0x17
#define REGISTER_CLOCK_SYNC_REG 0x58


#define SWRSTN_BIT 0x1
#define OSCONZ_BIT 0x8

#define READ_RTC_BIT 0x4
#define SET_RTC_BIT 0x2
#define DATA_RETEN_BIT 0x40
#define CENTRY_BIT 0x80

/* CLKSEL
 * 0: 1Hz
 * 1: 50Hz
 * 2: 60Hz
 * 3: 32.768KHz
 * */
#define CONFIG_CLKSEL 0

/* ECLK - enable external clock
 * 0: disable
 * 1: Enable
 * */
#define ECLK 0

/* USER CODE END */

/* Detrois test 2023/10/03  */
/* Read from a slave device */

//uint8_t reg_value=0x00;
int EXTERNAL_RTC_Init()
{
    int err=0;
    uint8_t reg_value=0x00;
    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG1, 1, &reg_value);


    if(err!=0){
        printk_ni("error i2c_read %d\n",err);
    }
    if((reg_value & 0x1) == 0)
    {
        //RTC is in reset mode
        //exit software reset & enable oscillator, set SWRSTN_BIT to 1 and OSCONZ bit to 0, set CONFIG_CLKSEL
        reg_value = SWRSTN_BIT | (CONFIG_CLKSEL << 4) | (ECLK << 7);

        err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG1, 1, &reg_value);
        time_t t;
        t = 1573441871;
        vTaskDelay(1000);
        err |= EXTERNAL_RTC_SetTime(t);

    }
    return err;
}

int EXTERNAL_RTC_SetTime(time_t t)
{
    FF_TimeStruct_t TimeBuf;
    t += 3600*configTIME_TIME_ZONE;
    FreeRTOS_gmtime_r(&t, &TimeBuf);
    return EXTERNAL_RTC_SetDateTime(&TimeBuf);
}

int EXTERNAL_RTC_GetTime(time_t *t)
{
    int err;
    FF_TimeStruct_t TimeBuf;

    err = EXTERNAL_RTC_GetDateTime(&TimeBuf);
    *t = FreeRTOS_mktime(&TimeBuf);
    return err;
}
int EXTERNAL_RTC_SetDateTime(FF_TimeStruct_t *TimeBuf)
{
    int err=0;
    uint8_t reg_value;
    printk("Set RTC Time¡G %4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", TimeBuf->tm_year+1900, TimeBuf->tm_mon+1, TimeBuf->tm_mday, TimeBuf->tm_hour, TimeBuf->tm_min, TimeBuf->tm_sec, configTIME_TIME_ZONE);
    // Enter Data Retention Mode
    //  err = I2C_Register_Read_8_8(&external_rtc0.config, REGISTER_CONFIG_REG2, &reg_value);
    //  reg_value |= DATA_RETEN_BIT;
    //  err = I2C_Register_Write_8_8(&external_rtc0.config, REGISTER_CONFIG_REG2, reg_value);

    //Enter software reset mode
    reg_value = 0;

    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG1, 1, &reg_value);
    //Exit Software reset mode

    reg_value = SWRSTN_BIT | (CONFIG_CLKSEL << 4) | (ECLK << 7);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG1, 1, &reg_value);

    //Write RTC time
    reg_value = (((TimeBuf->tm_sec / 10) & 0x7) << 4) | (TimeBuf->tm_sec % 10);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_SECONDS, 1, &reg_value);


    reg_value = (((TimeBuf->tm_min / 10) & 0x7) << 4) | (TimeBuf->tm_min % 10);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_MINUTES, 1, &reg_value);


    reg_value = (((TimeBuf->tm_hour / 10) & 0x3) << 4) | (TimeBuf->tm_hour % 10);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_HOURS, 1,&reg_value);


    reg_value = TimeBuf->tm_wday & 0x7;
    err = i2c_write(RTC_PORT, Slave_Address, REGISTER_DAY, 1, &reg_value);


    reg_value = (((TimeBuf->tm_mday / 10) & 0x3) << 4) | (TimeBuf->tm_mday % 10);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_DATE, 1, &reg_value);


    reg_value = (((TimeBuf->tm_mon / 10) & 0x1) << 4) | (TimeBuf->tm_mon % 10) + 1;
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_MONTH, 1, &reg_value);


    TimeBuf->tm_year -= 70;
    reg_value = (((TimeBuf->tm_year / 10) & 0xf) << 4) | (TimeBuf->tm_year % 10);
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_YEAR, 1, &reg_value);


    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG2, 1, &reg_value);


    reg_value |= SET_RTC_BIT;
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG2, 1, &reg_value);
    vTaskDelay(10);

    reg_value &= ~SET_RTC_BIT;
    err |= i2c_write(RTC_PORT, Slave_Address, REGISTER_CONFIG_REG2, 1, &reg_value);


    // Exit Data Retention Mode
    //err = I2C_Register_Read_8_8(&external_rtc0.config, REGISTER_CONFIG_REG2, &reg_value);
    //reg_value &= ~DATA_RETEN_BIT;
    //err = I2C_Register_Write_8_8(&external_rtc0.config, REGISTER_CONFIG_REG2, reg_value);

    //err = I2C_Register_Read_8_8(&external_rtc0.config, REGISTER_CONFIG_REG1, &reg_value);
    //reg_value &= ~OSCONZ_BIT;
    //err = I2C_Register_Write_8_8(&external_rtc0.config, REGISTER_CONFIG_REG1, reg_value);

    // Debug
    /*err = i2c_read(Slave_Address, REGISTER_CONFIG_REG1, 1, &reg_value);
    //printk("I2C Read REGISTER_CONFIG_REG1 value!![%02x]\n",reg_value);


    err = i2c_read(Slave_Address, REGISTER_CONFIG_REG2, 1, &reg_value);
    //printk("I2C Read REGISTER_CONFIG_REG2 value!![%02x]\n",reg_value);
    */

    return err;
}

int EXTERNAL_RTC_GetDateTime(FF_TimeStruct_t *TimeBuf)
{

    int err=0;
    uint8_t reg_value=0;

    //Read RTC time
    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_SECONDS, 1, &reg_value);

    TimeBuf->tm_sec = (reg_value >> 4) * 10 + (reg_value & 0xf);

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_MINUTES, 1, &reg_value);
    TimeBuf->tm_min = ((reg_value >> 4) * 10 + (reg_value & 0xf));

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_HOURS, 1, &reg_value);
    TimeBuf->tm_hour = ((reg_value >> 4) * 10 + (reg_value & 0xf));

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_DAY, 1, &reg_value);
    TimeBuf->tm_wday = reg_value & 0x7;

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_DATE, 1, &reg_value);
    TimeBuf->tm_mday = ((reg_value >> 4) * 10 + (reg_value & 0xf));

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_MONTH, 1, &reg_value);
    TimeBuf->tm_mon = ((reg_value >> 4) * 10 + (reg_value & 0xf)) - 1;

    err |= i2c_read(RTC_PORT, Slave_Address, REGISTER_YEAR, 1, &reg_value);
    TimeBuf->tm_year = ((reg_value >> 4) * 10 + (reg_value & 0xf)) + 70;


    return err;
}


void vTask_initRTC(void *param)
{
    i2c_init(RTC_PORT);
    int err;
    err = EXTERNAL_RTC_Init();
    if(err!=0){
        printk_ni("error EXTERNAL_RTC_Init %d\n",err);
    }
    vTaskDelete(0);
}

void setRTC(FF_TimeStruct_t *TimeBuf)
{
    time_t t;
    EXTERNAL_RTC_SetDateTime(TimeBuf);
    EXTERNAL_RTC_GetDateTime(TimeBuf);
    t = FreeRTOS_mktime(TimeBuf)-3600*configTIME_TIME_ZONE;
    FreeRTOS_settime(&t);
}
void getRTC(FF_TimeStruct_t *TimeBuf)
{
    EXTERNAL_RTC_GetDateTime(TimeBuf);
}

void set_system_time()
{
    FF_TimeStruct_t TimeBuf;
    time_t t;
    EXTERNAL_RTC_GetDateTime(&TimeBuf);
    t = FreeRTOS_mktime(&TimeBuf)-3600*configTIME_TIME_ZONE;
    FreeRTOS_settime(&t);
}
