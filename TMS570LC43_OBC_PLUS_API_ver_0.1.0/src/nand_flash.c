/*
 * nand_flash.c
 *
 *  Created on: 2019¦~10¤ë21¤é
 *      Author: kusoyao
 */

#include <assert.h>

#include "utils.h"
#include "global.h"
#include "nand_flash.h"

nand_device_t NANDDEV1 = {
    .is_initialized = 0,
    .sector_size = 2048,
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

nand_device_t NANDDEV2 = {
    .is_initialized = 0,
    .sector_size = 2048,
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

nand_device_t *nand_devs[2] = {&NANDDEV1, &NANDDEV2};

//modify from spiTransmitAndReceiveData
uint8_t spiRWCS(nand_device_t *h, uint32_t blocksize, uint8_t * srcbuff, uint8_t * destbuff, boolean last_cshold)
{
    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

    while(blocksize != 0U)
    {
        if((spi->FLG & 0x000000FFU) != 0U)
        {
           break;
        }

        if(blocksize == 1U && last_cshold == false)
        {
           Chip_Select_Hold = 0U;
        }

        if(srcbuff != 0)
        {
            Tx_Data = *srcbuff;
            srcbuff++;
        }

        spi->DAT1 =((uint32_t)DataFormat << 24U) |
                   ((uint32_t)ChipSelect << 16U) |
                   (WDelay)           |
                   (Chip_Select_Hold) |
                   Tx_Data;


        /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
        while((spi->FLG & 0x00000100U) != 0x00000100U)
        {
        } /* Wait */

        if(destbuff != 0)
        {
            *destbuff = (uint8_t)spi->BUF;
            destbuff++;
        }
        else
        {
            dummy = (uint8_t)spi->BUF;
        }

        blocksize--;
    }

    return (spi->FLG & 0xFFU);
}

nand_device_t * NANDFLASH_Init(int id)
{
    //Only initial once time
    if(nand_devs[id]->is_initialized != 0)
        return nand_devs[id];

    vSemaphoreCreateBinary(nand_devs[id]->sem);
    //nand_devs[id]->spi = (id == 0) ? spiREG3 : spiREG4;
    nand_devs[id]->spi = spiREG4;
    nand_devs[id]->dc.CS_HOLD = true;
    //nand_devs[id]->dc.DFSEL = SPI_FMT_0; // 1Mbps
    nand_devs[id]->dc.DFSEL = SPI_FMT_1; // 5Mbps
    //nand_devs[id]->dc.DFSEL = SPI_FMT_2; // 22Mbps
    nand_devs[id]->dc.WDEL = 0;
    nand_devs[id]->dc.CSNR = (id == 0) ? SPI_CS_0:SPI_CS_1;
    //nand_devs[id]->dc.CSNR = SPI_CS_0;
    nand_devs[id]->timeout = 100000000;

    NANDFLASH_Reset(nand_devs[id]);
    NANDFLASH_ReadID(nand_devs[id]);
    //by default device lock all block, reset flash will not change lock state
    uint8_t err;
    int try = 300;
    while(try)
    {
        err = NANDFLASH_UnlockAllBlocks(nand_devs[id]);
        if(err == 0)
            break;
        vTaskDelay(1);
        try--;
        printk("retry UnlockAllBlocks  %d\n", try);
    }

    nand_devs[id]->is_initialized = 1;
//    printk("nand_devs (%x)", nand_devs[id]);
    return nand_devs[id];
}

void NANDFLASH_Reset(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[1] = {RESET};
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
}

void NANDFLASH_ReadID(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[4] = {READ_ID, 0x0, 0x0, 0x0};
    uint8_t sin[4] = {0};

    status = spiRWCS(h, sizeof(sout), sout, sin, false);

//    printk("NAND FLASH ID = 0x%x 0x%x\n", sin[2], sin[3]);
    if(sin[2] != 0x2C || sin[3] != 0x14)
        printk("READ ID FAIL.\n");
}

uint8_t NANDFLASH_GetStatus(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[3] = {GET_FEATURES, 0x0, 0x0};
    uint8_t sin[3] = {0xFF, 0xFF, 0xFF};

    //sout[1] = FEATURE_BLOCK_LOCK;
    //status = spiRWCS(h, sizeof(sout), sout, sin, false); // 0x7C - 1: BP3 BP2 BP1 BP0 TB.  0: BRWD WP#/HOLD# Disable
    //sout[1] = FEATURE_CONFIGURATION;
    //status = spiRWCS(h, sizeof(sout), sout, sin, false); // 0x10 - 1: ECC_EN.  0: CFG2 CFG1 CFG0 LOT_EN
    //sout[1] = FEATURE_DIE_SELECT;
    //status = spiRWCS(h, sizeof(sout), sout, sin, false); // 0x00 - 0: DS0
    sout[1] = FEATURE_STATUS;
    status = spiRWCS(h, sizeof(sout), sout, sin, false); // 0x00
    //printk("S%x %x\n", status, sin[2]);

    return sin[2];
}

uint8_t NANDFLASH_WriteEnable(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[1] = {WRITE_ENABLE};
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
    return status;
}

uint8_t NANDFLASH_WriteDisable(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[1] = {WRITE_DISABLE};
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
    return status;
}

uint8_t NANDFLASH_UnlockAllBlocks(nand_device_t *h)
{
    //  Hardware protect
    //  When the WP#/HOLD# disable bit is at the default value of 0, and with BRWD set to 1 and WP# LOW,
    //  block lock registers [7:2] cannot be changed.

    uint8_t status = 0;
    uint8_t sout[3] = {SET_FEATURES, FEATURE_BLOCK_LOCK, 0x0};
    uint8_t sin[3] = {0x0};

    status = NANDFLASH_WriteEnable(h);
    status = spiRWCS(h, sizeof(sout), sout, 0, false); // 0x7C - 1: BP3 BP2 BP1 BP0 TB.  0: BRWD WP#/HOLD# Disable

    sout[0] = GET_FEATURES;
    sout[1] = FEATURE_BLOCK_LOCK;
    status = spiRWCS(h, sizeof(sout), sout, sin, false);

    if((sin[2] & 0x7C) == 0)
    {
//        printk("NANDFLASH Unlock All Block success\n");
    }
    else
    {
//        printk("NANDFLASH Unlock All Block fail. status = %x reg = 0x%x\n", status, sin[2]);
        return 1;
    }
    return 0;
}

uint8_t NANDFLASH_LockAllBlocks(nand_device_t *h)
{
    //  Hardware protect
    //  When the WP#/HOLD# disable bit is at the default value of 0, and with BRWD set to 1 and WP# LOW,
    //  block lock registers [7:2] cannot be changed.

    uint8_t status = 0;
    uint8_t sout[3] = {SET_FEATURES, FEATURE_BLOCK_LOCK, 0x7C};
    uint8_t sin[3] = {0x0};

    status = NANDFLASH_WriteEnable(h);
    status = spiRWCS(h, sizeof(sout), sout, 0, false); // 0x7C - 1: BP3 BP2 BP1 BP0 TB.  0: BRWD WP#/HOLD# Disable

    sout[0] = GET_FEATURES;
    sout[1] = FEATURE_BLOCK_LOCK;
    status = spiRWCS(h, sizeof(sout), sout, sin, false);

    if((sin[2] & 0x7C) == 0x7C)
    {
        printk("NANDFLASH Lock All Block success\n");
    }
    else
    {
        printk("NANDFLASH Lock All Block fail. status = %x reg = 0x%x\n", status, sin[2]);
        return 1;
    }
    return 0;
}

uint8_t NANDFLASH_lockAllBlocks(nand_device_t *h)
{
    uint8_t status = 0;
    uint8_t sout[3] = {SET_FEATURES, FEATURE_BLOCK_LOCK, 0x7C};
    uint8_t sin[3] = {0};

    status = NANDFLASH_WriteEnable(h);
    status = spiRWCS(h, sizeof(sout), sout, sin, false);
    return status;
}

//Total 27 bit address
//blockpage_addr: 1024blocks + 64page need 16bit address
//column_addr: 2048B data + 128B ECC need 12 bit address. If access data only, need 11 bit.
NAND_STATUS NANDFLASH_ReadRaw(nand_device_t *h, uint32_t blockpage_addr, uint32_t column_addr, uint8_t * destbuff, uint32_t length)
{
    uint8_t status = 0;
    uint8_t status_ecc;
    uint8_t sout[4] = {0};
    uint32_t timeout = h->timeout;

    if(length == 0)
        return NAND_SUCCESS;

#if MEASURE_TIME
    rti_t t1 = {0}, t2 = {0};
    uint32_t td;
    uint32_t us;

    save_rti_time(&t1);
#endif

#if ((__little_endian__ == 1) || (__LITTLE_ENDIAN__ == 1))
    assert(0);
#else

    sout[0] = PAGE_READ;
    sout[1] = (blockpage_addr >> 16) & 0xff; // normally zero
    sout[2] = (blockpage_addr >> 8) & 0xff;
    sout[3] = blockpage_addr & 0xff;
#endif
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
    if(status != 0)
        return NAND_IOFAIL;

    //Datasheet: Page read: 25£gs (MAX) with on-die ECC disabled; 70£gs (MAX) with on-die ECC enabled
    while(timeout)
    {
        status = NANDFLASH_GetStatus(h);
        if((status & STATUS_OIP) != STATUS_OIP)
            break;
        timeout--;
    }
    if(timeout == 0)
    {
        printk("NANDFLASH_Read timeout.\n");
        return NAND_TIMEOUT;
    }

    status_ecc = (status & (STATUS_ECCS2|STATUS_ECCS1|STATUS_ECCS0)) >> ECC_SHIFT;

    sout[0] = PAGE_READ_CACHE_X1;
    sout[1] = (column_addr >> 8) & 0xff;
    sout[2] = column_addr & 0xff;
    sout[3] = 0; // dummy byte
    status = spiRWCS(h, sizeof(sout), sout, 0, true);
    status = spiRWCS(h, length, 0, destbuff, false);
    if(status != 0)
        return NAND_IOFAIL;

#if MEASURE_TIME
    save_rti_time(&t2);
    td = diff_rti(&t2, &t1);
    us = rti_to_microsecond(td);
    printk("R %uus\n", us);
#endif

    switch(status_ecc)
    {
    case ECC_123_BIT_CORRECTED:
        printk("NANDFLASH_Read ECC Error 123_BIT_CORRECTED.\n");
        break;

    case ECC_GT8_BIT_NOT_CORRECTED:
        printk("NANDFLASH_Read ECC Error GT8_BIT_NOT_CORRECTED.\n");
        return NAND_ECC_ERROR;
        break;

    case ECC_456_BIT_CORRECTED:
        printk("NANDFLASH_Read ECC Error 456_BIT_CORRECTED.\n");
        //might refreshment TODO
        break;

    case ECC_78_BIT_CORRECTED:
        printk("NANDFLASH_Read ECC Error 78_BIT_CORRECTED.\n");
        //refreshment required TODO
        break;

    case ECC_NO_ERROR:
    default:
        break;
    }

    return NAND_SUCCESS;
}

//length: 1 ~ 2176
NAND_STATUS NANDFLASH_Read(nand_device_t *h, uint32_t address, uint8_t * destbuff, uint32_t length)
{
    //TODO handle column_addr != 0, unalign access
    NAND_STATUS status;
    uint32_t blockpage_addr = (address >> 11) & 0xffff;
    uint32_t column_addr = address & (PAGE_SIZE-1);
    uint8_t *dest = destbuff;
    uint32_t l;

    //printk("read 0x%x len 0x%x to 0x%x\n", address, length, destbuff);
    while(length > 0)
    {
        if(length > PAGE_SIZE)
            l = PAGE_SIZE;
        else
            l = length;
        status = NANDFLASH_ReadRaw(h, blockpage_addr, column_addr, dest, l);
        //printk("read 0x%x len 0x%x to 0x%x\n", blockpage_addr << 11, l, dest);
        //printk(".");
        blockpage_addr++;
        length -= l;
        dest += l;
    }
    return status;
}

NAND_STATUS NANDFLASH_ReadSpare(nand_device_t *h, uint32_t address, uint8_t * destbuff, uint32_t length)
{
    //length: 1 ~ 128
    uint32_t blockpage_addr, column_addr;
    blockpage_addr = (address >> 11) & 0xffff;
    column_addr = PAGE_SIZE + (address & (SPARE_SIZE-1));
    return NANDFLASH_ReadRaw(h, blockpage_addr, column_addr, destbuff, length);
}

NAND_STATUS NANDFLASH_ReadSector(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata)
{
    //CALL BY FTL, TODO use full page size
    NAND_STATUS status = NAND_SUCCESS;
    if(data != 0)
        status = NANDFLASH_ReadRaw(h, psn, 0, data, h->sector_size);
    if(sparedata != 0)
        status = NANDFLASH_ReadRaw(h, psn, h->page_size, sparedata, h->spare_size);
    return status;
}

NAND_STATUS NANDFLASH_WriteRaw(nand_device_t *h, uint32_t blockpage_addr, uint32_t column_addr, uint8_t * srcbuff, uint32_t length)
{
    uint8_t status = 0;
    uint8_t sout[4] = {0};
    uint32_t timeout = h->timeout;

    if(length == 0)
        return NAND_SUCCESS;

#if MEASURE_TIME
    rti_t t1 = {0}, t2 = {0};
    uint32_t td;
    uint32_t us;
    save_rti_time(&t1);
#endif

    status = NANDFLASH_WriteEnable(h);
    if(status != 0)
        return NAND_IOFAIL;

#if ((__little_endian__ == 1) || (__LITTLE_ENDIAN__ == 1))
    assert(0);
#else
    //PROGRAM_LOAD_X1: cache will start from all 0xFF
    sout[0] = PROGRAM_LOAD_X1;

    //PROGRAM_RANDOM_X1: cache keep unchanged, only update the data bytes
    //sout[0] = PROGRAM_RANDOM_X1;

    sout[1] = (column_addr >> 8) & 0xff;
    sout[2] = column_addr & 0xff;
#endif
    status = spiRWCS(h, 3, sout, 0, true);
    status = spiRWCS(h, length, srcbuff, 0, false);

    sout[0] = PROGRAM_EXECUTE;
    sout[1] = (blockpage_addr >> 16) & 0xff;
    sout[2] = (blockpage_addr >> 8) & 0xff;
    sout[3] = blockpage_addr & 0xff;
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
    if(status != 0)
        return NAND_IOFAIL;

    //Datasheet: Page program: 200£gs (TYP) with on-die ECC disabled; 220£gs (TYP) with on-die ECC enabled
    while(timeout)
    {
        status = NANDFLASH_GetStatus(h);
        if((status & STATUS_OIP) != STATUS_OIP)
            break;
        timeout--;
    }
    if(timeout == 0)
    {
        printk("NANDFLASH_Write timeout\n");
        return NAND_TIMEOUT;
    }
    if((status & STATUS_P_FAIL) == STATUS_P_FAIL)
    {
        printk("NANDFLASH_Write Program fail.\n");
        return NAND_FAIL;
    }

#if MEASURE_TIME
    save_rti_time(&t2);
    td = diff_rti(&t2, &t1);
    us = rti_to_microsecond(td);
    printk("W %uus\n", us);
#endif

    return NAND_SUCCESS;
}

NAND_STATUS NANDFLASH_Write(nand_device_t *h, uint32_t address, uint8_t * srcbuff, uint32_t length)
{
    //TODO hanele column_addr != 0, unalign access
    NAND_STATUS status;
    uint32_t blockpage_addr = (address >> 11) & 0xffff;
    uint32_t column_addr = address & (PAGE_SIZE-1);
    uint8_t *src = srcbuff;
    uint32_t l;

    //printk("write 0x%x len 0x%x from 0x%x\n", address, length, srcbuff);
    while(length > 0)
    {
        if(length > PAGE_SIZE)
            l = PAGE_SIZE;
        else
            l = length;
        status = NANDFLASH_WriteRaw(h, blockpage_addr, column_addr, src, l);
        //printk("write 0x%x len 0x%x from 0x%x\n", blockpage_addr << 11, l, src);
        //printk(".");
        blockpage_addr++;
        length -= l;
        src += l;
    }
    return status;
}

NAND_STATUS NANDFLASH_WriteSpare(nand_device_t *h, uint32_t address, uint8_t * srcbuff, uint32_t length)
{
    uint32_t blockpage_addr, column_addr;
    blockpage_addr = (address >> 11) & 0xffff;
    column_addr = PAGE_SIZE + (address & (SPARE_SIZE-1));
    return NANDFLASH_WriteRaw(h, blockpage_addr, column_addr, srcbuff, length);
}

NAND_STATUS NANDFLASH_WriteSector(nand_device_t *h, uint32_t psn, uint8_t * data, uint8_t * sparedata)
{
    //CALL BY FTL, TODO use full page size
    NAND_STATUS status = NAND_SUCCESS;
    if(data != 0)
        status = NANDFLASH_WriteRaw(h, psn, 0, data, h->sector_size);
    if(sparedata != 0)
        status = NANDFLASH_WriteRaw(h, psn, h->page_size, sparedata, h->spare_size);
    return status;
}

//Will also erase spare area
NAND_STATUS NANDFLASH_EraseRaw(nand_device_t *h, uint32_t blockpage_addr)
{
    uint8_t status;
    uint8_t sout[4] = {0};
    uint32_t timeout = h->timeout;

#if MEASURE_TIME
    rti_t t1 = {0}, t2 = {0};
    uint32_t td;
    uint32_t us;
    save_rti_time(&t1);
#endif

    status = NANDFLASH_WriteEnable(h);
    if(status != 0)
        return NAND_IOFAIL;

    sout[0] = BLOCK_ERASE;
    sout[1] = (blockpage_addr >> 16) & 0xff;
    sout[2] = (blockpage_addr >> 8) & 0xff;
    sout[3] = blockpage_addr & 0xff;
    status = spiRWCS(h, sizeof(sout), sout, 0, false);
    if(status != 0)
        return NAND_IOFAIL;

    //Datasheet: Block erase: 2ms (TYP)
    while(timeout)
    {
        status = NANDFLASH_GetStatus(h);
        if((status & STATUS_OIP) != STATUS_OIP)
            break;
        timeout--;
    }

    if(timeout == 0)
    {
        printk("NANDFLASH_Write timeout\n");
        return NAND_TIMEOUT;
    }
    if((status & STATUS_E_FAIL) == STATUS_E_FAIL)
    {
        printk("NANDFLASH_Erase fail.\n");
        return NAND_FAIL;
    }

#if MEASURE_TIME
    save_rti_time(&t2);
    td = diff_rti(&t2, &t1);
    us = rti_to_microsecond(td);
    printk("E %uus\n", us);
#endif

    return NAND_SUCCESS;
}

NAND_STATUS NANDFLASH_Erase(nand_device_t *h, uint32_t address, uint32_t length)
{
    //TODO unalign access

    printk("nand_device_t %d %x\n",h->is_initialized, &(h->spi));
    uint32_t blockpage_addr = (address >> 11) & ~(h->pages_per_block - 1);
    uint32_t blockLength = length / BLOCK_SIZE;
    if((blockLength == 0) && (length != 0))
        return NAND_FAIL;
    while(blockLength--)
    {
        //printk("erase 0x%x\n", blockpage_addr << 11);
        if(NANDFLASH_EraseRaw(h, blockpage_addr) != NAND_SUCCESS)
            return NAND_FAIL;
        blockpage_addr += h->pages_per_block;
    }
    return NAND_SUCCESS;
}

NAND_STATUS NANDFLASH_EraseBlock(nand_device_t *h, uint32_t block)
{
    //CALL BY FTL
    uint32_t blockpage_addr = block * h->pages_per_block;
    return NANDFLASH_EraseRaw(h, blockpage_addr);

}
