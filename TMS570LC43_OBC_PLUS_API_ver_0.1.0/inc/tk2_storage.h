/*
 * tk2_storage.h
 *
 *  Created on: 2021¦~8¤ë31¤é
 *      Author: User
 */

#ifndef INC_TK2_STORAGE_H_
#define INC_TK2_STORAGE_H_

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "nand_flash.h"
#include "ftl.h"
#include "global.h"
#include "task_uart_protocol.h"

typedef enum {
  NAND1_ID = 0,
  NAND2_ID,
  NAND_ID_MAX
} NAND_ID_TBL;

typedef struct {
    ftl_handle_t * ftl;
    int sector_nums;
    int sector_size;
    int sector_per_image;
    int sector_per_label;
    int max_images;
    uint8_t update_flag;
}nand_ctrl_t;

int init_local_storage();
void update_image_info_table();
int get_partial_img(uint16_t rsi_img_index, uint32 segmentindex, int nand_sector_size);
int transmit_partial_img(uint16_t index, uint16_t semgmentindex);
int transmit_partial_label(uint16_t index);
int save_image(int rsi_img_index, int obc_img_index, uint32_t size);
int save_label(int obc_img_index);
int find_storage_index();
void lock_storage_image(int obc_img_index);
void delete_storage_image(int obc_img_index);
void save_image_to_sd(int obc_img_index);


#endif /* INC_TK2_STORAGE_H_ */
