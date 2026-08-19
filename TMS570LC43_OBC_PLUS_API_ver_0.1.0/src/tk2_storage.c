/*
 * tk2_storage.c
 *
 *  Created on: 2023ï¿½~12ï¿½ï¿½04ï¿½ï¿½
 *      Author: Bruce
 */



#include "tk2_storage.h"

/* FreeRTOS+FAT includes. */
#include "ff_time.h"
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_time.h"
#include "ff_sddisk.h"
#include "ff_sys.h"

#define IMG_SIZE    (1920 * 1200 * 10 /8)// (2592 * 2048 * 10 /8)//
#define LABEL_SIZE  17
#define RESERVED_SECTOR_FOR_BADBLOCK  10



nand_ctrl_t nand_ctrl[NAND_ID_MAX] = {0};
ImageInfo *image_info_table;
int total_image = 0;
#define DEBUG_PRINT printk
#define GENERATE_PATTERN
int init_local_storage()
{
    int i;
    if(nand_ctrl[0].update_flag == 0 || nand_ctrl[1].update_flag == 0){
        for(i=0; i<NAND_ID_MAX; i++)
        {
            nand_ctrl[i].ftl = ftl_init(i);
            if(nand_ctrl[i].ftl != NULL)
            {
                nand_ctrl[i].sector_nums = nand_ctrl[i].ftl->dev->num_pages - RESERVED_SECTOR_FOR_BADBLOCK;
                nand_ctrl[i].sector_size = nand_ctrl[i].ftl->dev->sector_size;
                nand_ctrl[i].sector_per_image = (IMG_SIZE + nand_ctrl[i].sector_size) / nand_ctrl[i].sector_size;
                nand_ctrl[i].sector_per_label = (LABEL_SIZE + nand_ctrl[i].sector_size) / nand_ctrl[i].sector_size ;
                nand_ctrl[i].max_images = nand_ctrl[i].sector_nums / (nand_ctrl[i].sector_per_image + nand_ctrl[i].sector_per_label);
                nand_ctrl[i].update_flag = 1;
                total_image += nand_ctrl[i].max_images;
            }
            else
            {
                printk("NAND%d initial failed\n", i);
            }
        }
        image_info_table = (ImageInfo*)pvPortMalloc(sizeof(ImageInfo)*total_image);

        for(i=0; i<total_image; i++){
            transmit_partial_label(i);
            if(imageinfo_buffer[16] == 1)
                    memcpy(&image_info_table[i], &imageinfo_buffer[0], 17);
        }

    }

}

void update_image_info_table(){
    uint16_t i,j = 0;
    FF_TimeStruct_t tempTimeBuff;
    for(i=0; i<total_image; i++){
        transmit_partial_label(i);
        if(imageinfo_buffer[16] == 1){
            j++;
            memcpy(&image_info_table[i], &imageinfo_buffer[0], 17);
            printk_ni(" - Index: %u,  Mode = %u, Flag = %u, Size = %u,  Lock = %u, \n"
                      , i, image_info_table[i].mode,
                      image_info_table[i].flag,
                      image_info_table[i].size,
                      image_info_table[i].lockflag);
            FreeRTOS_gmtime_r((const time_t*)&image_info_table[i].timestamp, &tempTimeBuff);
            printk_ni("Time¡G%4d-%02d-%02d %02d:%02d:%02d\n", tempTimeBuff.tm_year+1900, tempTimeBuff.tm_mon+1, tempTimeBuff.tm_mday, tempTimeBuff.tm_hour+8, tempTimeBuff.tm_min, tempTimeBuff.tm_sec);
        }
    }
    if(j < 1)
        printk_ni("No image in Nandflash\n");
}

#ifdef GENERATE_PATTERN
int get_partial_img(uint16_t rsi_img_index, uint32 segmentindex, int nand_sector_size)
{
    int try = 0;
    int size = (SEGMENTSIZE / nand_sector_size ) * nand_sector_size ;
    uint16_t *buffer_Download = (uint16_t*)pvPortMalloc(sizeof(uint16_t)*2);
    memset(buffer_Download, 0, sizeof(uint16_t)*2);
    buffer_Download[0] = rsi_img_index;
    buffer_Download[1] = segmentindex;
    while(try < 3){
        SendPackage(TCTM_CMD_IMAGE_DOWNLOAD, 4, (uint8_t*)buffer_Download,(TCTM_SendPackageOptions){1,10000});
        if(GetOperateResponse() == CP_FAIL){
            printk_ni(" No get response, retry : [%d] \n", try);
            try++;
        }
        else if( GetOperateResult() == DOWNLOAD_CHECKSUM_FAIL){
            printk_ni(" Checksum fail, retry : [%d] \n", try);
            try++;
        }
        else{
            vPortFree(buffer_Download);
            return size;
        }
        vTaskDelay(1000);
    }
    vPortFree(buffer_Download);
    return -1;
}

int transmit_partial_img(uint16_t obc_img_index, uint16_t semgmentindex)
{
    int start;
    int received_length = 0;
    int remain;
    int i;
    uint16_t read_count;
    uint8_t *buffer = &spi_tx_buffer[0];
    uint8_t status = 0;

    nand_ctrl_t *nand=NULL;
    int nand_sector_size;

    for(i=0; i<NAND_ID_MAX; i++)
    {
        if(nand_ctrl[i].ftl == NULL)
            continue;

        if(obc_img_index < nand_ctrl[i].max_images)
        {
            nand = nand_ctrl[i].ftl;
            nand_sector_size = nand_ctrl[i].sector_size;
            read_count = (SEGMENTSIZE / nand_sector_size );
            start = obc_img_index * (nand_ctrl[i].sector_per_image + nand_ctrl[i].sector_per_label)+ nand_ctrl[i].sector_per_label + semgmentindex * read_count;
            remain = nand_ctrl[i].sector_per_image;
            break;
        }
        else
            obc_img_index -= nand_ctrl[i].max_images;
    }


    for(i=0; i<read_count; i++)
    {
        status = ftl_read_page( nand, start , buffer + (nand_sector_size  * i) );
        if(status != FTL_SUCCESS)
        {
            DEBUG_PRINT("ftl_read_page: error code=%d", status);
            return -1;
        }
        start++;
    }
    return 0;
}

int transmit_partial_label(uint16_t obc_img_index)
{
    int start;
    int received_length = 0;
    int i;
    uint16_t read_count = 1 ;
    uint8_t status = 0;
    uint8_t *buffer = &imageinfo_buffer[0];

    nand_ctrl_t *nand=NULL;
    int nand_sector_size;

    for(i=0; i<NAND_ID_MAX; i++)
    {
        if(nand_ctrl[i].ftl == NULL)
            continue;

        if(obc_img_index < nand_ctrl[i].max_images)
        {
            nand = nand_ctrl[i].ftl;
            nand_sector_size = nand_ctrl[i].sector_size;
            start = obc_img_index * (nand_ctrl[i].sector_per_image+ nand_ctrl[i].sector_per_label);
            break;
        }
        else{
            obc_img_index -= nand_ctrl[i].max_images;
        }
    }

    for(i=0; i<read_count; i++)
    {

     status = ftl_read_page( nand, start , buffer + (nand_sector_size  * i) );
     if(status != FTL_SUCCESS)
     {
//         DEBUG_PRINT("ftl_read_page: error code=%d", status);
         return -1;
     }
     start++;
    }
    return 0;
}

#endif

int save_image(int rsi_img_index, int obc_img_index, uint32_t size)
{
    int start, remain, i, nand_sector_size, received_length = 0, segmentindex = 0, current, total, count, freq;
    uint8_t *buffer = &spi_rx_buffer[0];
    nand_ctrl_t *nand=NULL;
    if(obc_img_index < 0){
        printk_ni("Out of memory, please delete image\n");
        return -1;
    }

    for(i=0; i<NAND_ID_MAX; i++)
    {
        if(nand_ctrl[i].ftl == NULL)
            continue;

        if(obc_img_index < nand_ctrl[i].max_images)
        {
            nand = nand_ctrl[i].ftl;
            nand_sector_size = nand_ctrl[i].sector_size;
            start = obc_img_index * (nand_ctrl[i].sector_per_image + nand_ctrl[i].sector_per_label)+ nand_ctrl[i].sector_per_label;
            remain = (size + nand_ctrl[i].sector_size) / nand_ctrl[i].sector_size;;
            break;
        }
        else
            obc_img_index -= nand_ctrl[i].max_images;
    }
    total = remain;
    current = 0;
    count = 0;
    freq = 3;
    while(remain > 0)
    {
        int write_count;
        int i;
        uint8_t status = 0;
        received_length = get_partial_img(rsi_img_index, segmentindex++, nand_sector_size);
        if( (received_length < 0) || ((received_length % nand_sector_size ) !=0) )
        {
         if( received_length < 0 )
             DEBUG_PRINT("get_partial_img: error!!\n");
         else
             DEBUG_PRINT("get_partial_img: size not in nandflash boundary!!\n");
         return -1;
        }
        write_count = (received_length / nand_sector_size) < remain ?(received_length / nand_sector_size) : remain;
        current += write_count;
        if((count++ % freq) == 0)
            printk_ni("Download : [%.1f%%]\n", 100.0 * current / total);
        for(i=0; i<write_count; i++)
        {
         status = ftl_write_page( nand, start , buffer + (nand_sector_size  * i) );
         if(status != FTL_SUCCESS)
         {
             DEBUG_PRINT("ftl_write_page: error code=%d", status);
             return -1;
         }

         start++;
         remain--;
        }
        if(i != write_count)
            return -1;
    }
    return 1;
}

int save_label(int obc_img_index)
{
    int start, remain, i, received_length = 0;
    if(obc_img_index < 0){
        printk_ni("Out of memory, please delete image\n");
        return -1;
    }
    uint8_t *buffer = &imageinfo_buffer[0];
    memcpy(&image_info_table[obc_img_index], &imageinfo_buffer[0], 17);
    nand_ctrl_t *nand=NULL;
    int nand_sector_size;

    for(i=0; i<NAND_ID_MAX; i++)
    {
        if(nand_ctrl[i].ftl == NULL)
            continue;

        if(obc_img_index < nand_ctrl[i].max_images)
        {
            nand = nand_ctrl[i].ftl;
            nand_sector_size = nand_ctrl[i].sector_size;
            start = obc_img_index * (nand_ctrl[i].sector_per_image + nand_ctrl[i].sector_per_label);
            remain = nand_ctrl[i].sector_per_label;
            break;
        }
        else
            obc_img_index -= nand_ctrl[i].max_images;
    }


    while(remain > 0)
    {
      int write_count;
      int i;
      uint8_t status = 0;
      received_length = nand_sector_size;
      if( (received_length < 0) || ((received_length % nand_sector_size ) !=0) )
      {
          if( (received_length % nand_sector_size ) !=0 )
              DEBUG_PRINT("get_partial_img: size not in nandflash boundary!!\n");
          else
              DEBUG_PRINT("get_partial_img: error!!\n");
          break;
      }

      write_count = (received_length / nand_sector_size );
      for(i=0; i<write_count; i++)
      {
          status = ftl_write_page( nand, start , buffer + (nand_sector_size  * i) );
          if(status != FTL_SUCCESS)
          {
              DEBUG_PRINT("ftl_write_page: error code=%d", status);
              break;
          }

          start++;
          remain--;
      }
      if(i != write_count)
          break;
    }
}

int find_storage_index(){
    int i,ret = -1;
    for(i = 0;i < total_image; i++){
        if(image_info_table[i].showflag == 0 || image_info_table[i].showflag == 0xa5){
            ret = i;
            break;
        }
    }

    if(ret < 0){
        for(i = 0;i < total_image; i++){
            if(image_info_table[i].lockflag == 0){
                ret = i;
                break;
            }
        }
    }
//    printk("ret = %d\n", ret);
    return ret;
}

void lock_storage_image(int obc_img_index){
    if(image_info_table[obc_img_index].showflag == 0){
        printk("Image[%02d] no exist \n", obc_img_index);
        return;
    }
    if (image_info_table[obc_img_index].lockflag == 0){
        image_info_table[obc_img_index].lockflag = 1;
        printk("Lock image[%02d]\n", obc_img_index);
    }
    else{
        image_info_table[obc_img_index].lockflag = 0;
        printk("Unlock image[%02d]\n", obc_img_index);
    }
    memcpy(&imageinfo_buffer[0], &image_info_table[obc_img_index], 17);
    save_label(obc_img_index);
}

void delete_storage_image(int obc_img_index){
    if(image_info_table[obc_img_index].showflag == 0){
        printk("Image[%02d] no exist \n", obc_img_index);
        return;
    }
    else if(image_info_table[obc_img_index].lockflag == 1){
        printk("Image[%02d] had locked\n", obc_img_index);
        return;
    }
    else{
        image_info_table[obc_img_index].showflag = 0;
        printk("Delete image[%02d]\n", obc_img_index);
    }
    memcpy(&imageinfo_buffer[0], &image_info_table[obc_img_index], 17);
    save_label(obc_img_index);
}

void save_image_to_sd(int obc_img_index){
    int remain, bytes2send, pattern_len, fsRAM_BUFFER_SIZE, i;
    fsRAM_BUFFER_SIZE = 1000;
    pattern_len = 512;
    i = obc_img_index;
    FF_FILE *pxFile;
    char  *pcFileName, *pcRAMBuffer;
    FF_TimeStruct_t tempTimeBuff;
    uint8_t* localbuffer;
    /* Allocate buffers used to hold date written to/from the disk, and the
            file names. */
    pcRAMBuffer = ( char * ) pvPortMalloc( fsRAM_BUFFER_SIZE );
    pcFileName = ( char * ) pvPortMalloc( ffconfigMAX_FILENAME );

    FreeRTOS_gmtime_r((const time_t*)&image_info_table[i].timestamp, &tempTimeBuff);

    /* Generate a file name. */
    ff_chdir( "/sd" );
    snprintf( pcFileName, ffconfigMAX_FILENAME, "%04d%02d%02d_%02d%02d%02d_%d_%d.bin", tempTimeBuff.tm_year+1900, tempTimeBuff.tm_mon+1, tempTimeBuff.tm_mday, tempTimeBuff.tm_hour+8, tempTimeBuff.tm_min, tempTimeBuff.tm_sec, image_info_table[i].mode, image_info_table[i].type);

    /* Obtain the current working directory and print out the file name and
            the directory into which the file is being written. */
    ff_getcwd( pcRAMBuffer, fsRAM_BUFFER_SIZE );
    printk_ni( "\nCreating file %s in %s\r\n", pcFileName, pcRAMBuffer );

    /*Calculate Segment Amount*/
    printk_ni("\n# === Image Download === \n");
    int writeSize = SEGMENTSIZE;
    int currentSize = 0;
    int freq = 3;
    int Local_IMG_SIZE = image_info_table[i].size;
    uint32 totalSegment = (Local_IMG_SIZE / SEGMENTSIZE) + 1;
    uint32 startSegment = 0;
    printk_ni(" -IMG_SIZE %d, Total Segment: %d, Start Segment: %d\n", image_info_table[i].size, totalSegment, startSegment);
//    printk_ni(" - Total Segment: %d, Start Segment: %d\n", totalSegment, startSegment);
    /* Open the file, creating the file if it does not already exist. */
    pxFile = ff_fopen( pcFileName, "w" );

    for(; startSegment < totalSegment; startSegment++){
        localbuffer = &spi_tx_buffer[0];
        writeSize = (Local_IMG_SIZE - currentSize) < SEGMENTSIZE ? (Local_IMG_SIZE - currentSize) : SEGMENTSIZE;
        currentSize += writeSize;
        if( pxFile != NULL )
        {
            transmit_partial_img(i, startSegment);
//            printk_ni("index = [%02d], startSegment = [%02d]\n", i, startSegment);
//            printk_ni(".");
            while (writeSize) {
                bytes2send = writeSize > pattern_len ? pattern_len :writeSize;
                remain = bytes2send;
                while(remain) {
                    remain -= ff_fwrite( localbuffer, 1, remain, pxFile );
                }
                writeSize -= bytes2send;
                localbuffer += bytes2send;
            }
        }
        else{
            printk_ni("File create fail!\n");
            return;
        }
        if((startSegment % freq) == 0)
            printk_ni("Download : [%.1f%%]\n", 100.0 * currentSize / Local_IMG_SIZE);
    }
//    printk_ni("\n");
    /* Close the file so another file can be created. */
    ff_fclose( pxFile );
    /*Creat text file*/
    char buffer[77];
    writeSize = 77;
    memset(&buffer[0], 0, 77);
    FreeRTOS_gmtime_r((const time_t*)&image_info_table[i].timestamp, &tempTimeBuff);
    sprintf(&buffer[0], " - Index: %u,  Mode = %u, Flag = %u, Size = %u\n"
                        "Time: %4d-%02d-%02d %02d:%02d:%02d \n"
                          , i, image_info_table[i].mode,
                          image_info_table[i].flag,
                          image_info_table[i].size,
                          tempTimeBuff.tm_year+1900,
                          tempTimeBuff.tm_mon+1,
                          tempTimeBuff.tm_mday,
                          tempTimeBuff.tm_hour+8,
                          tempTimeBuff.tm_min,
                          tempTimeBuff.tm_sec);
//    FreeRTOS_gmtime_r((const time_t*)&image_info_table[i].timestamp, &tempTimeBuff);
//    sprintf(&buffer[100], " Time: %4d-%02d-%02d %02d:%02d:%02d \n", tempTimeBuff.tm_year+1900, tempTimeBuff.tm_mon+1, tempTimeBuff.tm_mday, tempTimeBuff.tm_hour+8, tempTimeBuff.tm_min, tempTimeBuff.tm_sec);

    snprintf( pcFileName, ffconfigMAX_FILENAME, "%04d%02d%02d_%02d%02d%02d_%d_%d.txt", tempTimeBuff.tm_year+1900, tempTimeBuff.tm_mon+1, tempTimeBuff.tm_mday, tempTimeBuff.tm_hour+8, tempTimeBuff.tm_min, tempTimeBuff.tm_sec, image_info_table[i].mode, image_info_table[i].type);

    ff_getcwd( pcRAMBuffer, fsRAM_BUFFER_SIZE );

    pxFile = ff_fopen( pcFileName, "w" );
    if( pxFile != NULL ){
        while (writeSize) {
            bytes2send = writeSize > pattern_len ? pattern_len :writeSize;
            remain = bytes2send;
            while(remain) {
                remain -= ff_fwrite( &buffer[0], 1, remain, pxFile );
            }
            writeSize -= bytes2send;
        }
    }
    else{
        printk_ni("File create fail!\n");
        return;
    }
    ff_fclose( pxFile );

    vPortFree(pcRAMBuffer);
    vPortFree(pcFileName);

    printk_ni("\n# === Image Download Completed === \n");
}



