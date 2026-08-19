/*
 * user_nand_flash.h
 *
 *  Created on: 2021¦~11¤ë18¤é
 *      Author: User
 */

#ifndef INC_USER_NAND_FLASH_H_
#define INC_USER_NAND_FLASH_H_

#define JPEG_1P3MB_STORAGE_SIZE 1

#if(JPEG_1P3MB_STORAGE_SIZE == 1)
    #define TK2_MAX_PIC             30
    #define TK2_MAX_PIC_ONE_FLASH   15
#else
    #define TK2_MAX_PIC             34
    #define TK2_MAX_PIC_ONE_FLASH   17
#endif

//update @2022/1/11
#define INFO_FLASH_ADDRESS 0x0

#define PIC_INFO_SIZE BLOCK_SIZE
#define PIC_THUM_SIZE BLOCK_SIZE
#define PIC_RGB_TOTAL_SIZE (BLOCK_SIZE*9*6)
#define PIC_RGB_1_6_SIZE (BLOCK_SIZE*9)
#if(JPEG_1P3MB_STORAGE_SIZE == 1)
    #define PIC_JPEG_TOTLE_SIZE (BLOCK_SIZE*11)
#else
    #define PIC_JPEG_TOTLE_SIZE (BLOCK_SIZE*5)
#endif
#define SHIFT_1_PICTURE_SIZE (PIC_THUM_SIZE+PIC_RGB_TOTAL_SIZE+PIC_JPEG_TOTLE_SIZE)
#define RSI_1_PICTURE_SIZE 2880000
#define RSI_1_BUFFER_SIZE 60000

#define THUM_OFFSET 0
#define RGB_TOTAL_OFFSET (THUM_OFFSET+PIC_THUM_SIZE)

#define RGB_1_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*0)
#define RGB_2_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*1)
#define RGB_3_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*2)
#define RGB_4_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*3)
#define RGB_5_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*4)
#define RGB_6_6_OFFSET (THUM_OFFSET+PIC_THUM_SIZE+PIC_RGB_1_6_SIZE*5)

#define JPEG_OFFSET (THUM_OFFSET+PIC_THUM_SIZE)

#define NAND_1 0
#define NAND_2 1

#define PHOTO_TYPE_RGB_0 3
#define PHOTO_TYPE_RGB_1 4
#define PHOTO_TYPE_RGB_2 5
#define PHOTO_TYPE_RGB_3 6
#define PHOTO_TYPE_RGB_4 7
#define PHOTO_TYPE_RGB_5 8

#define JPEG_MAX_SIZE PIC_JPEG_TOTLE_SIZE
#define JPEG_OVER_SIZE_FLAG 1<<1
#define JPEG_COMPRESSFAIL_FLAG 1<<0

#pragma pack(1)
typedef union{
    uint32_t jpeg_size;
    struct {
        //uint16_t jpeg_info_12_bit   : 12;
        uint8_t jpeg_info_4_bit   : 2;
        uint32_t jpeg_size_20_bit   : 22;
        uint8_t parameter_MSB   :8
    };
}JpegInfoFormat;

#pragma pack(1)
typedef struct pic_info_struct
{
    uint16_t index;
    uint16_t sequence;
    uint8_t type;
    uint8_t mode;
    uint8_t flag;
    uint32_t timestamp;
    uint32_t size;

}PictureInfoStruct;

typedef union
{
    PictureInfoStruct picInfo[TK2_MAX_PIC];
    struct{
        PictureInfoStruct nand1Info[TK2_MAX_PIC_ONE_FLASH];
        PictureInfoStruct nand2Info[TK2_MAX_PIC_ONE_FLASH];
    };
}InfoTableUnion;

extern InfoTableUnion infoTable;

void nandflash_test_write(uint32_t pattern);
void nandflash_test_read(uint32_t* pattern);

void nandflash_erase();
int nandflash_erase_all();

int write_thumbnail(uint8_t photo_number);
int read_thumbnail(uint8_t photo_number);
int write_jpeg(uint8_t photo_number);
int read_jpeg(uint8_t photo_number);
int write_rgb(uint8_t photo_number,uint8_t photo_type);
int read_rgb(uint8_t photo_number,uint8_t photo_type);
int write_photo(uint16_t photo_number, uint16_t segamentindex);
int read_photo(uint16_t photo_number, uint8_t segamentindex);

void table_flash_init();
int read_table(uint8_t table_number);
int read_table_RSI(uint16_t photo_number);
int write_table(uint8_t table_number);
int write_table_RSI(uint16_t photo_number);
void update_table_info(uint8_t photo_number);

int nandflash_erase_single_flash(uint8_t flash_number);
int nandflash_erase_photo(uint16_t photo_number);
int nandflash_erase_table(uint8_t table_number);
#endif /* INC_USER_NAND_FLASH_H_ */
