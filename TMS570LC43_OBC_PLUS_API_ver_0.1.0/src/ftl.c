/*
 * ftl.c
 *
 *  Created on: 2019¦~12¤ë3¤é
 *      Author: kusoyao
 */

#include "nand_flash.h"
#include "ftl.h"
#include "global.h"

#define FTL_DEBUG 1
#if !FTL_DEBUG
#define printk
#endif

#define MAP_BITS 16 //log2(TOTAL_PAGE)
#define KEEP_FREE_BLOCK 2
#define INVALID_LSN_NUMBER 0xFFFFFFFF
#define INVALID_UNIQUE_ID_NUMBER 0xFFFFFFFF
#define INVALID_PSN_NUMBER 0xFFFF
#define BLOCK_GOOD_MARK 0xFF
#define MAGIC_NUMBER 0x4c495343 // LISC
#define MAX_NANDFLASH   2

ftl_handle_t xftl[MAX_NANDFLASH] = {0};

void ftl_dump_spare(char *tag, ftl_spare_t *spare)
{
    int i;
    printk("%s %2x %2x%2x%2x magic %8x uid %8x lsn %4x psn %4x", tag, spare->badmark, spare->flags[0], spare->flags[1], spare->flags[2], spare->magic, spare->unique_id, spare->lsn, spare->psn);
    for(i = 0; i < 12 ; ++i)
        printk(" %02x", spare->unused[i]);
    printk(" p");
    for(i = MAP_BITS - 1; i >= 0 ; --i)
        printk(" %04x", spare->pointer[i]);
    printk("\n");
}

ftl_handle_t *ftl_init(int nand_idx)
{
    NAND_STATUS nand_status;
    uint32_t psn, firstpsn;
    uint32_t id_max = 0, id_min = 0xFFFFFFFF;
    int i;
    if(xftl[nand_idx].is_initialized != 0)
        return &xftl;

    //xftl.dev = NANDFLASH_Init(0);
    xftl[nand_idx].dev = NANDFLASH_Init(nand_idx);

    ASSERT( xftl.dev != 0);

    xftl[nand_idx].data_buffer = pvPortMalloc( xftl[nand_idx].dev->page_size + xftl[nand_idx].dev->spare_size);
    xftl[nand_idx].spare_buffer = xftl[nand_idx].data_buffer + xftl[nand_idx].dev->page_size;

    xftl[nand_idx].magic = MAGIC_NUMBER; // LISC
    xftl[nand_idx].unique_id = INVALID_UNIQUE_ID_NUMBER;

    //find first used block from flash
    for(i = 0; i < KEEP_FREE_BLOCK+1; ++i)
    {
        firstpsn = i * xftl[nand_idx].dev->pages_per_block;
        nand_status = xftl[nand_idx].dev->read(xftl[nand_idx].dev, firstpsn, 0, xftl[nand_idx].spare_buffer);
        if(nand_status != NAND_SUCCESS)
            return 0;
        if(xftl[nand_idx].spare_buffer->badmark != BLOCK_GOOD_MARK)
            continue;
        if(xftl[nand_idx].spare_buffer->unique_id != INVALID_UNIQUE_ID_NUMBER)
        {
            xftl[nand_idx].unique_id = xftl[nand_idx].spare_buffer->unique_id;
            break;
        }
    }

    if(xftl[nand_idx].unique_id == INVALID_UNIQUE_ID_NUMBER)
    {
        xftl[nand_idx].newest_psn = INVALID_PSN_NUMBER;
        xftl[nand_idx].block_head = 0xFFFFFFFF; // let the first write to new block, page 0
        xftl[nand_idx].block_tail = 0;
        xftl[nand_idx].unique_id = 0;
    }
    else
    {
        for(i = 0; i < xftl[nand_idx].dev->num_blocks; ++i)
        {
            psn = i * xftl[nand_idx].dev->pages_per_block;
            nand_status = xftl[nand_idx].dev->read(xftl[nand_idx].dev, psn, 0, xftl[nand_idx].spare_buffer);
            if(nand_status != NAND_SUCCESS)
                return 0;
            if(xftl[nand_idx].spare_buffer->badmark != BLOCK_GOOD_MARK)
                continue;
            if(xftl[nand_idx].spare_buffer->unique_id == INVALID_UNIQUE_ID_NUMBER)
                continue;
            if( xftl[nand_idx].spare_buffer->unique_id >= id_max)
            {
                id_max = xftl[nand_idx].spare_buffer->unique_id;
                xftl[nand_idx].block_head = i;
            }
            if( xftl[nand_idx].spare_buffer->unique_id < id_min)
            {
                id_min = xftl[nand_idx].spare_buffer->unique_id;
                xftl[nand_idx].block_tail = i;
            }
        }

        //find maxinum unique id from flash
        psn = xftl[nand_idx].block_head * xftl[nand_idx].dev->pages_per_block;
        for(i = 5; i >= 0; --i) //  i = log2(pages_per_block) - 1
        {
            psn |= 1 << i;
            nand_status = xftl[nand_idx].dev->read(xftl[nand_idx].dev, psn, 0, xftl[nand_idx].spare_buffer);
            if(nand_status != NAND_SUCCESS)
                return 0;

            if((xftl[nand_idx].spare_buffer->unique_id == INVALID_UNIQUE_ID_NUMBER) ||  // invalid unique id, it mean it is not used yet page
                xftl[nand_idx].spare_buffer->unique_id < id_max)
            {
                    psn &= ~(1 << i);
            }
            else
            {
                id_max = xftl[nand_idx].spare_buffer->unique_id;
            }
        }
        xftl[nand_idx].unique_id = id_max + 1;
        xftl[nand_idx].newest_psn = psn;

    }

    xftl[nand_idx].is_initialized = 1;
//    ftl_dump_spare("FTL INIT ", xftl[nand_idx].spare_buffer);
//    printk("newestpsn %x block_head %x block_tail %x\n", xftl[nand_idx].newest_psn, xftl[nand_idx].block_head, xftl[nand_idx].block_tail);

    return &xftl[nand_idx];
}

void ftl_reset(int nand_idx)
{
    if(xftl[nand_idx].is_initialized == 0)
        return ;

    vPortFree(xftl[nand_idx].data_buffer);
    xftl[nand_idx].is_initialized = 0;
}

FTL_STATUS ftl_lsn_to_psn(ftl_handle_t *ftl, uint32_t lsn, uint32_t *psn)
{
    uint32_t lsnxor;
    NAND_STATUS nand_status;
    int i;

    *psn = ftl->newest_psn;
    if(*psn == INVALID_PSN_NUMBER)
        return FTL_PAGE_NOT_FOUND;

    nand_status = ftl->dev->read(ftl->dev, ftl->newest_psn, 0, ftl->spare_buffer);
    if(nand_status != NAND_SUCCESS)
        return FTL_IOFAIL;

    if(ftl->spare_buffer->lsn == lsn)
        return FTL_PAGE_FOUND;

    lsnxor = ftl->spare_buffer->lsn ^ lsn;
    for(i = MAP_BITS - 1; i >= 0; --i)
    {
        //ftl_dump_spare("S ", ftl->spare_buffer);
        if(lsnxor & (1 << i))
        {
            // bit difference, follow pointer
            *psn = ftl->spare_buffer->pointer[i];
            if(*psn == INVALID_PSN_NUMBER)
                return FTL_PAGE_NOT_FOUND;

            nand_status = ftl->dev->read(ftl->dev, *psn, 0, ftl->spare_buffer);
            if(nand_status != NAND_SUCCESS)
                return FTL_IOFAIL;

            if(ftl->spare_buffer->lsn == lsn)
                return FTL_PAGE_FOUND;

            lsnxor = ftl->spare_buffer->lsn ^ lsn;
        }
    }

    return FTL_PAGE_FOUND;
}

FTL_STATUS ftl_read_page(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf)
{
    uint32_t psn;
    NAND_STATUS nand_status;
    FTL_STATUS ftl_status;
    ftl_status = ftl_lsn_to_psn(ftl, lsn, &psn);
    if(ftl_status == FTL_PAGE_FOUND)
    {
        //printk("FTL R lsn 0x%x from psn 0x%x\n", lsn, psn);
        nand_status = ftl->dev->read(ftl->dev, psn, buf, 0);
        if(nand_status != NAND_SUCCESS)
            return FTL_IOFAIL;
        else
            ftl_status = FTL_SUCCESS;
    }
    else
    {
        //printk("FTL RN 0x%x\n", lsn);
        memset(buf, 0, ftl->dev->page_size);
    }
    return ftl_status;
}

FTL_STATUS ftl_write_page(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf)
{
    FTL_STATUS ftl_status;
    uint32_t free_blocks;

    ftl_status = ftl_write_page__(ftl, lsn, buf);
    if(ftl_status == FTL_SUCCESS)
    {
        do
        {
            //treat boundary
            if(ftl->block_head > ftl->block_tail)
                free_blocks = ftl->dev->num_blocks - (ftl->block_head - ftl->block_tail + 1);
            else
                free_blocks = ftl->block_tail - ftl->block_head - 1;

            if(free_blocks < KEEP_FREE_BLOCK)
                ftl_status = ftl_garbage_collection(ftl);
            else
                break;
            //after garbage collection, it may not release any space.
            //KEEP_FREE_BLOCK must large than one, otherwise when "the worst case" there is no enough space for garbage collection
            //repeat until reach KEEP_FREE_BLOCK
        }while(ftl_status == FTL_SUCCESS);
    }
    return ftl_status;
}

FTL_STATUS ftl_write_page__(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf)
{
    uint32_t psn;
    uint32_t newpsn = ftl->newest_psn + 1;
    ftl_spare_t newspare;
    NAND_STATUS nand_status;
    FTL_STATUS ftl_status;
    uint32_t lsnxor;
    uint32_t timeout;
    int i;

    uint32_t newblockn = newpsn/ftl->dev->pages_per_block;

    if(newblockn != ftl->block_head)
    {
        //find a good block
        timeout = ftl->dev->num_blocks;
        while(1)
        {
            if(newblockn >= ftl->dev->num_blocks)
                newblockn = 0;
            ftl_status = ftl_block_status(ftl, newblockn);
            if(ftl_status == FTL_BLOCK_FREE)
                break;
            else if(ftl_status == FTL_BLOCK_BAD)
                newblockn++;
            else if(ftl_status == FTL_BLOCK_DIRTY)
                return FTL_FAIL;
            else
                //FTL_IOFAIL
                return FTL_IOFAIL;

            timeout--;
            if(timeout == 0)
                return FTL_FAIL;
        }

        newpsn = newblockn*ftl->dev->pages_per_block;
    }

    memset(&newspare, 0xFF, ftl->dev->spare_size);
    psn = ftl->newest_psn;
    if(psn != INVALID_PSN_NUMBER)
    {
        nand_status = ftl->dev->read(ftl->dev, psn, 0, ftl->spare_buffer);
        if(nand_status != NAND_SUCCESS)
            return FTL_IOFAIL;

        lsnxor = ftl->spare_buffer->lsn ^ lsn;
        for(i = MAP_BITS - 1; i >= 0; --i)
        {
            //ftl_dump_spare("P ", &newspare);

            // psn is valid and bit difference, point to it and follow pointer, otherwise copy pointer
            if((psn != INVALID_PSN_NUMBER) && (lsnxor & (1 << i)))
            {
                newspare.pointer[i] = psn;
                psn = ftl->spare_buffer->pointer[i];

                //if reach end , copy all pointer
                if(psn == INVALID_PSN_NUMBER)
                    continue;

                nand_status = ftl->dev->read(ftl->dev, psn, 0, ftl->spare_buffer);
                if(nand_status != NAND_SUCCESS)
                    return FTL_IOFAIL;

                lsnxor = ftl->spare_buffer->lsn ^ lsn;
            }
            else
            {
                newspare.pointer[i] = ftl->spare_buffer->pointer[i];
            }
        }
    }

    newspare.badmark = BLOCK_GOOD_MARK;
    newspare.magic = ftl->magic;
    newspare.lsn = lsn;
    newspare.psn = newpsn;
    newspare.unique_id = ftl->unique_id;

    ftl->unique_id++;
    //ftl_dump_spare("N ", &newspare);

    //printk("FTL W lsn 0x%x to psn 0x%x\n", lsn, newpsn);
    nand_status = ftl->dev->write(ftl->dev, newpsn, buf, &newspare);
    if(nand_status != NAND_SUCCESS)
        return FTL_IOFAIL;

    ftl->block_head = newblockn;
    ftl->newest_psn = newpsn;
    if((ftl->newest_psn + 1) == INVALID_PSN_NUMBER)
        ftl->newest_psn = INVALID_PSN_NUMBER;

    return FTL_SUCCESS;
}

FTL_STATUS ftl_garbage_collection(ftl_handle_t *ftl)
{
    /* THIS GARBAGE COLLECTION is NOT tested yet!
     * THIS GARBAGE COLLECTION is NOT tested yet!
     * THIS GARBAGE COLLECTION is NOT tested yet!
     * */
    FTL_STATUS ftl_status;
    NAND_STATUS nand_status;
    uint32_t psn = ftl->block_tail * ftl->dev->pages_per_block;
    uint32_t psnlookup;
    int i;

    for(i = 0; i < ftl->dev->pages_per_block ; ++i)
    {
        nand_status = ftl->dev->read(ftl->dev, psn + i, 0, ftl->spare_buffer);
        if(nand_status != NAND_SUCCESS)
            return FTL_IOFAIL;

        ftl_status = ftl_lsn_to_psn(ftl, ftl->spare_buffer->lsn, &psnlookup);
        if(ftl_status == FTL_IOFAIL)
            return FTL_IOFAIL;

        if(ftl_status == FTL_PAGE_FOUND)
        {
            if((psn + i) == psnlookup)
            {
                printk("Copy psn %04x\n", psn + i);
                nand_status = ftl->dev->read(ftl->dev, psn + i, ftl->data_buffer, 0);
                if(nand_status != NAND_SUCCESS)
                    return FTL_IOFAIL;

                //we must guarantee there are 64 page is useable
                ftl_status = ftl_write_page__(ftl, ftl->spare_buffer->lsn, ftl->data_buffer);
                if(ftl_status != FTL_SUCCESS)
                    return FTL_DEVICE_FULL;
            }
            else
            {
//                printk("clean old psn %04x, newer at %x\n", psn + i, psnlookup);
            }
        }
        else
        {
//            printk("psn %04x not found\n", psn + i);
        }
    }

    printk("FTL E block %d\n", ftl->block_tail);
    nand_status = ftl->dev->erase(ftl->dev, ftl->block_tail);
    if(nand_status != NAND_SUCCESS)
        return FTL_IOFAIL;

    ftl->block_tail++;
    if(ftl->block_tail >= ftl->dev->num_blocks)
        ftl->block_tail = 0;

    return FTL_SUCCESS;
}

FTL_STATUS ftl_block_status(ftl_handle_t *ftl, uint32_t block)
{
    //only first page had mark
    NAND_STATUS nand_status;
    uint32_t psn = block * ftl->dev->pages_per_block;

    nand_status = ftl->dev->read(ftl->dev, psn, 0, ftl->spare_buffer);
    if(nand_status != NAND_SUCCESS)
        return FTL_IOFAIL;

    if(ftl->spare_buffer->badmark != BLOCK_GOOD_MARK)
    {
        printk("S %x BAD\n", block);
        return FTL_BLOCK_BAD;
    }

    if(ftl->spare_buffer->magic == 0xFFFFFFFF)
    {
        //printk("S %x FREE\n", block);
        return FTL_BLOCK_FREE;
    }

    printk("S %x DIRTY\n", block);
    return FTL_BLOCK_DIRTY;
}
