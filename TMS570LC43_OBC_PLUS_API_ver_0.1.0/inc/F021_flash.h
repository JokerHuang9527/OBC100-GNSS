/*
 * F021_flash.h
 *
 *  Created on: 2019¦~11¤ë20¤é
 *      Author: kusoyao
 */

#ifndef INC_F021_FLASH_H_
#define INC_F021_FLASH_H_

#include "HL_sys_common.h"

#define _L2FMC
#include "F021.h"

Fapi_StatusType F021_Init();
Fapi_StatusType F021_Erase(uint32_t Sector);
Fapi_StatusType F021_Write(uint8_t *dest, uint8_t *src, uint32_t len);
Fapi_StatusType F021_Read(uint8_t *dest, uint8_t *src, uint32_t len);

#endif /* INC_F021_FLASH_H_ */
