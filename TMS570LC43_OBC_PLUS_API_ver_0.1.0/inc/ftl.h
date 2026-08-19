/*
 * ftl.h
 *
 *  Created on: 2019¦~12¤ë4¤é
 *      Author: kusoyao
 */

#ifndef INC_FTL_H_
#define INC_FTL_H_

#include "nand_flash.h"

typedef enum
{
    FTL_SUCCESS,
    FTL_FAIL,
    FTL_IOFAIL,
    FTL_PAGE_FOUND,
    FTL_PAGE_NOT_FOUND,
    FTL_PAGE_WRITED,
    FTL_BLOCK_FREE,
    FTL_BLOCK_DIRTY,
    FTL_BLOCK_BAD,
    FTL_DEVICE_FULL,
}FTL_STATUS;

typedef struct ftl_page {
    uint16_t lsn;
    uint16_t psn;
    uint8_t *data;
    uint8_t status;
} ftl_page_t;

/*
    first byte of spare area is mark as bad mark from the vendor.
    0-31 byte is not protected by ECC
    31-63 byte is protected by ECC, should be write with page data
    64-127 byte is ECC, not include, should not write

    total 1024*64 sectors(pages)
    pointer point to physical sector number. physical sector number 0 is not valid and the first sector is preserved.
*/
typedef struct ftl_spare {
    uint8_t badmark; // block bad mark
    uint8_t flags[3];
    uint32_t magic; // LISC OTECH
    uint32_t unique_id; // always increment, used for search head and tail.
    uint32_t lsn; // logical sector number
    uint32_t psn; // physical sector number
    uint8_t unused[12];
    uint16_t pointer[16];
    //uint8_t ECC[64]; // auto calculate by hardware
} ftl_spare_t;

typedef struct ftl_handle
{
    int is_initialized;
    uint32_t magic;
    nand_device_t *dev;
    uint32_t unique_id;
    uint32_t newest_psn;
    uint32_t block_head;
    uint32_t block_tail;

    // buffer
    uint8_t *data_buffer; // 1 page
    ftl_spare_t *spare_buffer;
}ftl_handle_t ;

ftl_handle_t *ftl_init();
void ftl_reset();
FTL_STATUS ftl_lsn_to_psn(ftl_handle_t *ftl, uint32_t lsn, uint32_t *psn);
FTL_STATUS ftl_read_page(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf);
FTL_STATUS ftl_write_page(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf);
FTL_STATUS ftl_write_page__(ftl_handle_t *ftl, uint32_t lsn, uint8_t *buf);
FTL_STATUS ftl_garbage_collection(ftl_handle_t *ftl);
FTL_STATUS ftl_block_status(ftl_handle_t *ftl,uint32_t block);

#endif /* INC_FTL_H_ */
