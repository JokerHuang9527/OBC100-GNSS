#include <stdio.h>
#include <string.h>
#include <time.h>
#include "FreeRTOS.h"
#include "os_task.h"
//#include "rti_runtimestats.h"
#include "ma_date_and_time.h"
#include "FreeRTOSTIMEConfig.h"
//RTC_SETTING
#include "rtc.h"

int iTimeZone = 3600 * configTIME_TIME_ZONE;

extern volatile time_t xSysTimeSeconds;		/* Rendszerid��蕭 (EPOCH) m��percek */
extern volatile unsigned int  xSysTimeMsec;	/* Rendszerid��蕭 ezred m��percek */

/* FreeRTOS time() implement���脣�蕭
 * az 1970. janu�嚙� 1. 0:00:00 嚙踝蕭� eltelt m��percek sz�鈭�� adja vissza
 * A 32 bites time_t probl������ jelenthetet 2038-ban. Ez a megval嚙踝蕭嚙踝蕭�憟� m�嚙�
 * ennek figyelembe v��l嚙踝蕭豏始 t�撌�.
 */
time_t FreeRTOS_time(time_t *pxTime)
{
    set_system_time();
/* 64 bites hozz���悸����蕭 kritikus szakasz. */
#if(__time64_t)
	portTICK_TYPE_ENTER_CRITICAL();
	time_t xNow = (time_t)xSysTimeSeconds;
	portTICK_TYPE_EXIT_CRITICAL();
#else
	time_t xNow = (time_t)xSysTimeSeconds;
#endif
	if(pxTime != NULL)
	{
		*pxTime = xNow;
	}
	return xNow;
}
/*-----------------------------------------------------------*/

void FreeRTOS_settime(time_t *pxTime)
{
	portTICK_TYPE_ENTER_CRITICAL();
	{
		xSysTimeSeconds = (time_t) *pxTime;
		xSysTimeMsec = (uint32_t) 0;
	}
	portTICK_TYPE_EXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

time_t FreeRTOS_get_secs_msec(time_t *pulMsec)
{
	time_t uxReturn;
	portTICK_TYPE_ENTER_CRITICAL();
	{
		uxReturn = (time_t) xSysTimeSeconds;
		if(pulMsec != NULL)
		{
			*pulMsec = xSysTimeMsec;
		}
	}
	portTICK_TYPE_EXIT_CRITICAL();

	return uxReturn;
}
/*-----------------------------------------------------------*/

void FreeRTOS_set_secs_msec(time_t *pulSeconds, time_t *pulMsec)
{
	portTICK_TYPE_ENTER_CRITICAL();
	{
		xSysTimeSeconds = *pulSeconds;
		if( pulMsec != NULL )
		{
			xSysTimeMsec = *pulMsec;
		}
	}
	portTICK_TYPE_EXIT_CRITICAL();
}
/*-----------------------------------------------------------*/
