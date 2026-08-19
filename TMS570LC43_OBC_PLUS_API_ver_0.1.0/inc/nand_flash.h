/*
 * nand_flash.h
 *
 *  Created on: 2019¦~10¤ë21¤é
 *      Author: kusoyao
 */

#ifndef INC_NAND_FLASH_H_
#define INC_NAND_FLASH_H_

#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

#include "HL_sys_common.h"
#include "HL_spi.h"


typedef enum
{
    NAND_SUCCESS,
    NAND_FAIL,
    NAND_IOFAIL,
    NAND_ECC_ERROR,
    NAND_TIMEOUT,
}NAND_STATUS;

typedef struct nand_device nand_device_t;

typedef NAND_STATUS (*nand_read_fp)(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata);
typedef NAND_STATUS (*nand_write_fp)(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata);
typedef NAND_STATUS (*nand_erase_fp)(nand_device_t *h, uint32_t block);

typedef struct nand_device {
    int is_initialized;

    // parameters
    uint32_t sector_size; // 512B , software define
    uint32_t page_size; // 2048B
    uint32_t block_size; // 128KB
    uint32_t spare_size; // 64B not include ECC area
    uint32_t pages_per_block; // 64
    uint32_t num_pages; // 1024 * 64
    uint32_t num_blocks; // 1024

    // spi
    spiBASE_t *spi;
    spiDAT1_t dc;
    SemaphoreHandle_t sem;
    uint32_t timeout;

    // functions
    nand_read_fp read;
    nand_write_fp write;
    nand_erase_fp erase;
} nand_device_t;

#define MEASURE_TIME 0

// Device MT29F1G01ABAFDSF 1Gb.
// SPI interface support Mode0 or Mode3 (Use mode0 here).

#define RESET           0xFF    //Reset the device

#define GET_FEATURES    0x0F    //Get features
#define SET_FEATURES    0x1F    //Set features
#define READ_ID         0x9F    //Read ID

#define PAGE_READ       0x13    //Array read
#define PAGE_READ_CACHE_RANDOM  0x30    //Cache read
#define PAGE_READ_CACHE_LAST    0x3F    //Ending of cache read
#define PAGE_READ_CACHE_X1      0x03    // or 0x0B, Output cache data at column address
#define PAGE_READ_CACHE_X2      0x3B    //Output cache data on IO[1:0]
#define PAGE_READ_CACHE_X4      0x6B    //Output cache data on IO[3:0]
#define PAGE_READ_CACHE_DUAL_IO  0xBB   //Input address/Output cache data on IO[1:0]
#define PAGE_READ_CACHE_QUAD_IO  0xEB   //Input address/Output cache data on IO[3:0]

#define WRITE_ENABLE    0x06    //Sets the WEL bit in the status register to 1; required to enable operations that change the content of the memory array
#define WRITE_DISABLE   0x04    //Clears the WEL bit in the status register to 0; required to disable operations that change the content of the memory array

#define BLOCK_ERASE     0xD8    //Bllock erase

#define PROGRAM_EXECUTE  0x10   //Array program
#define PROGRAM_LOAD_X1  0x02   //Load program data into cache register on SI
#define PROGRAM_LOAD_X4  0x32   //Load program data into cache register on SO[3:0]
#define PROGRAM_RANDOM_X1  0x84 //Overwrite cache register with input data on SI
#define PROGRAM_RANDOM_X4  0x34 //Overwrite cache register with input data on SO[3:0]
#define PERMANENT_BLOCK_LOCK_PROTECTION 0x2C //Permanently protect a specific group of blocks

#define FEATURE_BLOCK_LOCK 0xA0
#define FEATURE_CONFIGURATION 0xB0
#define FEATURE_STATUS 0xC0
#define FEATURE_DIE_SELECT 0xD0

#define BAD_BLOCK_MARK 0x00 // stored at the first byte of spare area

#define STATUS_CRBSY  0x80  //Cache read busy
#define STATUS_ECCS2  0x40  //ECC status register
#define STATUS_ECCS1  0x20  //
#define STATUS_ECCS0  0x10  //
#define STATUS_P_FAIL 0x08  //
#define STATUS_E_FAIL 0x04  //
#define STATUS_WEL    0x02  //
#define STATUS_OIP    0x01  //

#define ECC_SHIFT     4
#define ECC_NO_ERROR  0x00  //No error
#define ECC_123_BIT_CORRECTED   (0x01 << ECC_SHIFT)  //1-3 bit errors detected and corrected
#define ECC_GT8_BIT_NOT_CORRECTED    (0x02 << ECC_SHIFT)  //Bit errors greater than 8 bits detected and not corrected
#define ECC_456_BIT_CORRECTED   (0x03 << ECC_SHIFT)  //4-6 bit errors detected and corrected. Indicates data refreshment might be taken
#define ECC_78_BIT_CORRECTED    (0x05 << ECC_SHIFT)  //7-8 bit errors detected and corrected. Indicates data refreshment must be taken to guarantee data retention

#define DIE 1
#define PLANES 1
#define BLOCKS 1024
#define PAGES 64
#define PAGE_SIZE 2048
#define BLOCK_SIZE (PAGES*PAGE_SIZE)
#define PLANE_SIZE (BLOCKS*BLOCK_SIZE)

#define SPARE_SIZE 128

void NANDFLASH_Reset(nand_device_t *h);
void NANDFLASH_ReadID(nand_device_t *h);
uint8_t NANDFLASH_GetStatus(nand_device_t *h);
uint8_t NANDFLASH_WriteEnable(nand_device_t *h);
uint8_t NANDFLASH_WriteDisable(nand_device_t *h);
uint8_t NANDFLASH_UnlockAllBlocks(nand_device_t *h);
NAND_STATUS NANDFLASH_ReadRaw(nand_device_t *h, uint32_t blockpage_addr, uint32_t column_addr, uint8_t * destbuff, uint32_t length);
NAND_STATUS NANDFLASH_Read(nand_device_t *h, uint32_t address, uint8_t * destbuff, uint32_t length);
NAND_STATUS NANDFLASH_ReadSpare(nand_device_t *h, uint32_t address, uint8_t * destbuff, uint32_t length);
NAND_STATUS NANDFLASH_ReadSector(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata);
NAND_STATUS NANDFLASH_WriteRaw(nand_device_t *h, uint32_t blockpage_addr, uint32_t column_addr, uint8_t * srcbuff, uint32_t length);
NAND_STATUS NANDFLASH_Write(nand_device_t *h, uint32_t address, uint8_t * srcbuff, uint32_t length);
NAND_STATUS NANDFLASH_WriteSpare(nand_device_t *h, uint32_t address, uint8_t * srcbuff, uint32_t length);
NAND_STATUS NANDFLASH_WriteSector(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata);
NAND_STATUS NANDFLASH_EraseRaw(nand_device_t *h, uint32_t blockpage_addr);
NAND_STATUS NANDFLASH_Erase(nand_device_t *h, uint32_t address, uint32_t length);
NAND_STATUS NANDFLASH_EraseBlock(nand_device_t *h, uint32_t block);
nand_device_t * NANDFLASH_Init(int id);

#endif /* INC_NAND_FLASH_H_ */
