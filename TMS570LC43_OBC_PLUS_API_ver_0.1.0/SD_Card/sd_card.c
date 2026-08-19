/*
 * task_sd_test.c
 *
 *  Created on: 2022¦~09¤ë21¤é
 *      Author: Leo Wu
 */

#include "FreeRTOS.h"
#include "os_task.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "HL_sys_common.h"
#include "HL_system.h"


#include "UART_API.h"
#include "global.h"
#include <assert.h>
#include <SD_Card/sd_card.h>
#include <SD_Card/fatfs/diskio.h>       /* Declarations of device I/O functions */
#include "utils.h"
#include "nand_flash.h"

#include "mmc-hercules.h""
//#include "task_SD_test.h"

nand_device_t SD_DEV1 = {
    .is_initialized = 0,
    .sector_size = 512,
    .page_size = PAGE_SIZE,
    .block_size = BLOCK_SIZE,
    .spare_size = 64,
    .pages_per_block = PAGES,
    .num_pages = BLOCKS*PAGES,
    .num_blocks = BLOCKS,
    .read = NANDFLASH_ReadSector,
    .write = NANDFLASH_WriteSector,
    .erase = NANDFLASH_EraseBlock,
};

nand_device_t *SD_devs = &SD_DEV1;

void spi_send_test(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[1] = {RESET};
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
}

nand_device_t * SD_CARD_FLASH_Init()
{
    //printk("into SD_CARD_FLASH_Init()\n");
    //Only initial once time
    if(SD_devs->is_initialized != 0){
        printk("Exit SD_CARD_FLASH_Init()\n");
        return SD_devs;
    }


    //printk("SD_CARD_FLASH_Init() 1\n");
    SD_devs->spi = spiREG4;
    SD_devs->dc.CS_HOLD = true;
    //SD_devs->dc.DFSEL = SPI_FMT_0; // 1Mbps
    //SD_devs->dc.DFSEL = SPI_FMT_1; // 5Mbps
    SD_devs->dc.DFSEL = SPI_FMT_2; // 22Mbps
    SD_devs->dc.WDEL = 0;
    SD_devs->dc.CSNR = SPI_CS_2;
    SD_devs->timeout = 100000000;

    spi_send_test(SD_devs);

    //TickType_t delay = 10;
    //vTaskDelay(delay);
//SD    NANDFLASH_ReadID(nand_devs[id]);
    //by default device lock all block, reset flash will not change lock state
//    uint8_t err;
//    int try = 3;
    //printk("SD_CARD_FLASH_Init() 2  try=%d\n",try);
//    while(try)
//    {
////SD        err = NANDFLASH_UnlockAllBlocks(nand_devs[id]);
//        //SD init function
//
//        if(err == 0)
//            break;
//        //vTaskDelay(1);
//        vTaskDelay(10); //for fixed error return 3
//        try--;
//        printk("retry UnlockAllBlocks  %d\n", try);
//    }

    SD_devs->is_initialized = 1;

    disk_initialize (1);
    //printk("SD_CARD_FLASH_Init() 3\n");
    return SD_devs;
}

int lisco_disk_read (
        unsigned char *buff,            /* Pointer to the data buffer to store read data */
        unsigned long sector,        /* Start sector number (LBA) */
        unsigned int count            /* Sector count (1..255) */
)
{
    int ret;
    nand_device_t *dev;
    dev = SD_CARD_FLASH_Init();

    printk("lisco_disk_read dev(%d) buff(0x%08x) sector(%d) count(%d)\n",dev,buff,sector,count);

    ret=disk_read (0,buff,sector,count);
    return ret;
}

int lisco_disk_write (
        const unsigned char *buff,    /* Pointer to the data to be written */
        unsigned long sector,        /* Start sector number (LBA) */
        unsigned int count            /* Sector count (1..255) */
)
{
    int ret;
    nand_device_t *dev;
    dev = SD_CARD_FLASH_Init();

    printk("lisco_disk_write dev(%d) buff(0x%08x) sector(%d) count(%d)\n",dev,buff,sector,count);

    ret=disk_write (0,buff,sector,count );
    return ret;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vTask_sd_test(void *param)
{

    vPortTaskUsesFPU();

    TickType_t delay = 100;

    vTaskDelay(delay);


    unsigned char writebuffer[1024];
    int count;
    for(count=0;count<255;count++)
    {
        //writebuffer[count]=count-3;
    }
    unsigned char readbuffer[1024];
    int ret;
    //printk("readbuffer[101] (%x)\n",readbuffer[101]);
    //lisco_disk_read(&readbuffer[0],0,2);
    //ret=disk_read (0,&readbuffer[0],0,2);

    //printk("ret=%d readbuffer[101] (%x)\n",ret,readbuffer[101]);
    //lisco_disk_write(&writebuffer[0],0,1);
    //disk_write (0,&writebuffer[0],0,1);

    //lisco_disk_read(&readbuffer[0],0,2);
    //ret=disk_read (0,&readbuffer[0],0,2);
    for(count=0;count<255;count++)
    {
        //printk("readbuffer[%d] (%d)\n",count,readbuffer[count]);
    }

    //printk("readbuffer[101] (%x)\n",readbuffer[101]);
    //printk("readbuffer[%d]\n",ret);

    testbuf[0]=0x12;
    testbuf[1]=0x34;
    testbuf[2]=0x56;
    testbuf[3]=0x99;
   //ret=lisco_disk_write(0x64144180,0,1);
   // printk("lisco_disk_write ret=%d\n",ret);
    //ret=lisco_disk_read(0x6414419c,0,2);
   // printk("lisco_disk_read ret=%d\n",ret);

    while(1)
    {
		vTaskDelay(delay);

    }
    vTaskDelete(0);
}

uint32_t sd_get_sector_count()
{
    DWORD sz_vol=0UL;   /* Size for volume, fat, dir, data */
    if (disk_ioctl(0, GET_SECTOR_COUNT, &sz_vol) != RES_OK)
        return 0;

    return (uint32_t)sz_vol;
}
