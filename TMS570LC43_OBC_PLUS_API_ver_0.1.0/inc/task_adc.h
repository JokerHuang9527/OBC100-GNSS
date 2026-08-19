/*
 * task_adc.h
 *
 *  Created on: 2019¦~10¤ë30¤é
 *      Author: kusoyao
 */

#ifndef INC_TASK_ADC_H_
#define INC_TASK_ADC_H_

#define ADC_REFERENCE_VOLTAGE 3.44
#define ADC_RESOLUTION 4095.0
#define THERMAL_RESISDANCE_BETA 3435.0

#define OBC_ADC_CHANNELS 26 // ADC1: 13 channels + ADC2: 13 channels
#define ADC_CHANNELS (OBC_ADC_CHANNELS+SF2_ADC_CHANNELS)
#define OBC_ADC_INDEX 0

#define STATISTIC_COUNT 100

typedef struct
{
    float avg;
    float std;
    float max;
    float min;
}ADCStruct;

extern float healthData[OBC_ADC_CHANNELS];

void vTask_adc(void *param);
void adc_print();
float calculate_temp(uint16_t *vol);

#endif /* INC_TASK_ADC_H_ */
