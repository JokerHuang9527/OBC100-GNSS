/*
* mmc-hercules.h
*
*  Created on: Oct 4, 2015
*      Author: Jan
*/


#ifndef FATFS_PORT_MMC_HERCULES_H_
#define FATFS_PORT_MMC_HERCULES_H_

#include <nand_flash.h>



void mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg);
void lisco_DESELECT (nand_device_t *h);


#endif /* FATFS_PORT_MMC_HERCULES_H_ */
