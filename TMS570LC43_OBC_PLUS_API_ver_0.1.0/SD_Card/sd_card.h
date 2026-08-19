/*
* task_sd_test.h
*
*  Created on: Oct 19, 2022
*      Author: Leo
*/


#ifndef TASK_SD_TEST_H_
#define TASK_SD_TEST_H_

#include "nand_flash.h"

int lisco_disk_read (
        unsigned char *buff,            /* Pointer to the data buffer to store read data */
        unsigned long sector,        /* Start sector number (LBA) */
        unsigned int count            /* Sector count (1..255) */
);

int lisco_disk_write (
        const unsigned char *buff,    /* Pointer to the data to be written */
        unsigned long sector,        /* Start sector number (LBA) */
        unsigned int count            /* Sector count (1..255) */
);
void vTask_sd_test(void *param);


extern nand_device_t *SD_devs;
#endif /* TASK_SD_TEST_H_ */
