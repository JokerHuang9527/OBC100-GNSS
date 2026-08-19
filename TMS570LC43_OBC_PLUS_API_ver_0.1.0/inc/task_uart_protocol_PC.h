/*
 * task_uart_protocol_PC.h
 *
 *  Created on: 2023¦~11¤ë20¤é
 *      Author: DetroisHuang
 */

#ifndef INC_TASK_UART_PROTOCOL_PC_H_
#define INC_TASK_UART_PROTOCOL_PC_H_

#include "task_uart_protocol.h"

int TCTM_Info_2(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_ImageDownload_2(uint8_t command, uint16_t dataLength, uint8_t *data);

void SendPackage_2(uint8_t command, uint16_t length, uint8_t *data);
void CommandHandle_2(CP_PackagePacker *packer);
void GetPackageCallback_2(CP_PackagePacker *packer, CP_HandleResult result);
void PutByteToReceivePackage2();

void vTask_uart_protocol_2(void *pvParameters);

#endif /* INC_TASK_UART_PROTOCOL_PC_H_ */
