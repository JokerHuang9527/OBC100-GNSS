/*
 * nand_disk.h
 *
 *  Created on: 2019¦~12¤ë10¤é
 *      Author: kusoyao
 */

#ifndef INC_NAND_DISK_H_
#define INC_NAND_DISK_H_

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
//#include "portable.h"

#include "ff_headers.h"
#include "ff_devices.h"

int32_t prvWriteNAND( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );
int32_t prvReadNAND( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );
BaseType_t nand_disk_delete( FF_Disk_t *pxDisk );
FF_Error_t nand_disk_mount(FF_Disk_t *pxDisk);
FF_Error_t nand_disk_format(FF_Disk_t *pxDisk);
FF_Disk_t * nand_disk_init();

#endif /* INC_NAND_DISK_H_ */
