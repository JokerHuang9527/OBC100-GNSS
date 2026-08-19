/*
 * task_adc.c
 *
 *  Created on: 2019ฆ~10ค๋30ค้
 *      Author: kusoyao
 */

#include "FreeRTOS.h"
#include "os_task.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_adc.h"
#include "HL_reg_adc.h"
#include "HL_pinmux.h"
#include "task_gio.h"

#include "task_adc.h"
#include "UART_API.h"
#include "global.h"

float healthData[OBC_ADC_CHANNELS];
ADCStruct adcData[OBC_ADC_CHANNELS] = {0};
adcData_t rawData[OBC_ADC_CHANNELS] = {0};



float ThermalTemperatureCalaulate(float raw, float beta, float resolution)
{
    return (1.0/( log(raw / (resolution-raw)) / beta + 0.003354 ) - 273.15);
}
// This structure is for TMS570 internal temperature sensor
// More info, please check below
// PDF: TMS570LC4357 and RM57L843 On-Chip Temperature Sensor Measurements
// URL: https://www.ti.com/lit/an/spna216/spna216.pdf
#pragma pack(1)
typedef volatile struct
{
    uint16_t TEMP1VAL; /* The value read from the ADC for this sensor at the first calibration temperature. */
    uint16_t TEMP1;    /* The temperature in degrees Kelvin 303 = 30C  */
    uint16_t TEMP2VAL;
    uint16_t TEMP2;    /* The temperature in degrees Kelvin 233 = -40C  */
    uint16_t TEMP3VAL;
    uint16_t TEMP3;    /* The temperature in degrees Kelvin 398K = 125C */
    uint32_t Reserved;
} OnDieTemperatureCalibStruct;

#define temp_calib1 ((OnDieTemperatureCalibStruct *)0xF0080310)
#define temp_calib2 ((OnDieTemperatureCalibStruct *)0xF0080320)
#define temp_calib3 ((OnDieTemperatureCalibStruct *)0xF0080330)
float temp_slope[3] = {0};

void InitOnDieTempSensorSlope()
{
    temp_slope[0] = (float)(temp_calib1->TEMP3 - temp_calib1->TEMP2) / (temp_calib1->TEMP3VAL - temp_calib1->TEMP2VAL);
    temp_slope[1] = (float)(temp_calib2->TEMP3 - temp_calib2->TEMP2) / (temp_calib2->TEMP3VAL - temp_calib2->TEMP2VAL);
    temp_slope[2] = (float)(temp_calib3->TEMP3 - temp_calib3->TEMP2) / (temp_calib3->TEMP3VAL - temp_calib3->TEMP2VAL);
}

void adc_get_raw(adcData_t *adata)
{

    adcStartConversion(adcREG1, adcGROUP1);
    adcStartConversion(adcREG2, adcGROUP1);

    while((adcIsConversionComplete(adcREG1, adcGROUP1))==0)
        vTaskDelay(1);
    while((adcIsConversionComplete(adcREG2, adcGROUP1))==0)
        vTaskDelay(1);

    adcGetData(adcREG1, adcGROUP1, &adata[0]); //13 channel
    adcGetData(adcREG2, adcGROUP1, &adata[13]); //13 channel

}

void adc_conversion(adcData_t *rawData, ADCStruct *adcData)
{
    /* ADC1
     * CH0: OBC_3V3_SYS Thermal Sensor
     * CH1: OBC_1V2_SYS, 1.2V
     * CH2: OBC_3V3_SYS, 3.3V
     * CH3: OBC_3V3A_ADC, 3.3V
     * CH4: OBC_EXT_3V3I
     * CH5: OBC_EXT_3V3O
     * CH6: X
     * CH7: OBC_5V_ISNS, VOUT = (Gain) (R_sense)(I_load), I = V/(Gain*R_sense) = V / (50*0.01)
     * CH8: OBC_EXT5VO_ISNS = V / (50*0.01)
     * CH9: OBC_5V_IN, 2.5V*2 => 5V
     * CH10: OBC_5VO_eF, 2.5V*2 => 5V
     * CH11: OBC_EXT_5VI
     * CH31: On-Die Temperature Sensors 1
     *
     * ADC2 , CH30 & 31 exist internally - http://e2e.ti.com/support/microcontrollers/hercules/f/312/t/743064?TMS570LC4357-Special-Multiplexed-Controls-Temperature-Sensor-Select
     * CH0: OBC_H2_AN0
     * CH1: OBC_H2_AN1
     * CH2: OBC_H2_AN2
     * CH3: OBC_H2_AN3
     * CH4: OBC_H2_AN4
     * CH5: OBC_H2_AN5
     * CH6: OBC_H2_AN6
     * CH7: OBC_H2_AN7
     * CH16: H1_ALT_Out4
     * CH17: H1_ALT_Out5
     * CH18: H1_AGPIO_Rx
     * CH30: On-Die Temperature Sensors 2
     * CH31: On-Die Temperature Sensors 3
     *
     * TI OTP Calibration Information
     * f0080310 079b012f 05f500e9 09be018e ffffffff
     * f0080320 0769012f 05a400e9 09b2018e ffffffff
     * f0080330 077d012f 05f000e9 099e018e ffffffff
     *
     * */
    int ch, i;
    ADCStruct adata[OBC_ADC_CHANNELS];
    memset(adata, 0, sizeof(adata));
    for(ch = 0;ch < OBC_ADC_CHANNELS; ch++)
        adata[ch].min = 9999.0;

    for(i = 0;i < STATISTIC_COUNT;++i)
    {
        adc_get_raw(rawData);

        for(ch = 0;ch < OBC_ADC_CHANNELS; ch++)
        {
            adata[ch].std += rawData[ch].value * rawData[ch].value;
            adata[ch].avg += rawData[ch].value;

            if(rawData[ch].value > adata[ch].max)
                adata[ch].max = rawData[ch].value;
            else if(rawData[ch].value < adata[ch].min)
                adata[ch].min = rawData[ch].value;
        }
    }

    for(ch = 0;ch < OBC_ADC_CHANNELS; ch++)
    {
        adata[ch].std /= STATISTIC_COUNT;
        adata[ch].avg /= STATISTIC_COUNT;
        adata[ch].std = sqrtf(fabsf(adata[ch].std - adata[ch].avg * adata[ch].avg));
    }

    //protect todo
    memcpy(adcData, adata, sizeof(adata));
    //protect end
}

void adc_dump()
{
    int ch;
    printk("Channel  ---value---\n");
    printk("OBC ADC Data\n");
    for(ch = OBC_ADC_INDEX; ch < OBC_ADC_CHANNELS; ch++)
        printk("%7d  %11.6f\n", ch, healthData[ch]);
}

void adc_print()
{
    int ch;
    float std, avg, max, min;
    printk("Channel  ----avg----  ----max----  ----min----\n");
    for(ch = 0; ch < OBC_ADC_CHANNELS; ch++){
        avg = ADC_REFERENCE_VOLTAGE*(adcData[ch].avg/ADC_RESOLUTION);
        max = ADC_REFERENCE_VOLTAGE*(adcData[ch].max/ADC_RESOLUTION);
        min = ADC_REFERENCE_VOLTAGE*(adcData[ch].min/ADC_RESOLUTION);
        if(ch == 0){
            avg = ThermalTemperatureCalaulate(adcData[ch].avg, THERMAL_RESISDANCE_BETA, ADC_RESOLUTION);
            max = ThermalTemperatureCalaulate(adcData[ch].max, THERMAL_RESISDANCE_BETA, ADC_RESOLUTION);
            min = ThermalTemperatureCalaulate(adcData[ch].min, THERMAL_RESISDANCE_BETA, ADC_RESOLUTION);
        }
        else if( ch == 7 || ch == 8 || ch == 9 || ch == 10){
            avg = (ADC_REFERENCE_VOLTAGE*(adcData[ch].avg/ADC_RESOLUTION)) * 2.0;
            max = (ADC_REFERENCE_VOLTAGE*(adcData[ch].max/ADC_RESOLUTION)) * 2.0;
            min = (ADC_REFERENCE_VOLTAGE*(adcData[ch].min/ADC_RESOLUTION)) * 2.0;
        }
        else if(ch == 12){
            avg = temp_slope[0] * adcData[ch].avg - 273.15;
            max = temp_slope[0] * adcData[ch].max - 273.15;
            min = temp_slope[0] * adcData[ch].min - 273.15;
        }
        else if(ch == 24 || ch == 25){
            avg = temp_slope[ch - 23] * adcData[ch].avg - 273.15;
            max = temp_slope[ch - 23] * adcData[ch].max - 273.15;
            min = temp_slope[ch - 23] * adcData[ch].min - 273.15;
        }
        printk("%7d  %11.6f  %11.6f  %11.6f\n", ch, avg, max, min);
    }
}

float calculate_temp(uint16_t *vol) {
    double V = *vol+ *(vol+1)/10000;
    //Rt = (V*10)/(5000-V)
    double Rt = (V*10);
    double lnRt = log10(Rt);
    double lnRt2 = lnRt*lnRt;
    double lnRt3 = lnRt2*lnRt;
    double lnRt4 = lnRt3*lnRt;
    double fitTemp = -0.5633 * lnRt4 + 1.2418 * lnRt3 + 10.992 * lnRt2 - 84.997 * lnRt + 98.317;
    return fitTemp;
}

void adc_calculate(ADCStruct *adcData)
{
    int ch;
    for(ch = 0; ch < OBC_ADC_CHANNELS; ch++)
    {
        healthData[ch] = ADC_REFERENCE_VOLTAGE*(adcData[ch].avg/ADC_RESOLUTION);
    }
    // LOBC Temperature sensor
    healthData[0] = ThermalTemperatureCalaulate(adcData[0].avg, THERMAL_RESISDANCE_BETA, ADC_RESOLUTION);
    // OBC_5V_ISNS, V = (GAIN)*(RSENSE)*(ILOAD) = 50 * 10m * I, I = V/0.5 = V*2
    healthData[7] = (ADC_REFERENCE_VOLTAGE*(adcData[7].avg/ADC_RESOLUTION)) * 2.0;
    // OBC_EXT5VO_ISNS, V = (GAIN)*(RSENSE)*(ILOAD) = 50 * 10m * I, I = V/0.5 = V*2
    healthData[8] = (ADC_REFERENCE_VOLTAGE*(adcData[8].avg/ADC_RESOLUTION)) * 2.0;
    // OBC_5V_IN, 2.5V*2 => 5V
    healthData[9] = (ADC_REFERENCE_VOLTAGE*(adcData[9].avg/ADC_RESOLUTION)) * 2.0;
    // OBC_5VO_eF, 2.5V*2 => 5V
    healthData[10] = (ADC_REFERENCE_VOLTAGE*(adcData[10].avg/ADC_RESOLUTION)) * 2.0;

    healthData[12]  = temp_slope[0] * adcData[12].avg - 273.15;
    healthData[24]  = temp_slope[1] * adcData[24].avg - 273.15;
    healthData[25]  = temp_slope[2] * adcData[25].avg - 273.15;
}

void health_limit(uint32_t channel, float min, float max)
{
    if((healthData[channel] <= min) || (max <= healthData[channel]))
        houseKeepingData.healthStatus.flag |= 1U << channel;
    else
        houseKeepingData.healthStatus.flag &= ~(1U << channel);
}

void health_check(ADCStruct *adcData)
{
    // -40 ~ 85 Temperature Limit
    // +- 5% Voltage Limit
    // TMS570
    health_limit(0, -40.0, 85.0);
    health_limit(1, 1.2*0.95, 1.2*1.05); // 1.2V
    health_limit(2, 3.3*0.95, 3.3*1.05); // 3.3V
    health_limit(3, 3.3*0.95, 3.3*1.05); // 3.3V
    health_limit(7, 0.0, 0.33); // 0.33A
    health_limit(9, 5.0*0.95, 5.0*1.05); // 5.0V
    health_limit(12, -40.0, 85.0);
    health_limit(24, -40.0, 85.0);
    health_limit(25, -40.0, 85.0);

}


void vTask_adc(void *param)
{

    vPortTaskUsesFPU();

    TickType_t delay = 10000;
    vTaskDelay(delay);
    adcMidPointCalibration(adcREG1);
    adcMidPointCalibration(adcREG2);
    InitOnDieTempSensorSlope();
    uint8_t asked_LVDS = 0;
    uint8_t LVDS_status = 1;
    uint8_t LVDS_error_count = 0;

    while(1)
    {
        if( LVDS_status == 1) {
            vTaskDelay(delay);
            adc_conversion(rawData, adcData);
            adc_calculate(adcData);
            health_check(adcData);
            if(adc_enable)
            {
                adc_dump();
            }
        }

    }
    vTaskDelete(0);
}

