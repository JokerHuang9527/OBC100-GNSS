/*
 * ff_example.c
 *
 *  Created on: 2023¦~11¤ë28¤é
 *      Author: Bruce lee
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"

/* FreeRTOS+FAT includes. */
#include "ff_sddisk.h"
#include "ff_sys.h"

/* ST HAL includes. */
#include "SD_Card/sd_card.h"

/* Misc definitions. */
#define sdSIGNATURE                          0x41404342UL
#define sdHUNDRED_64_BIT                     ( 100ull )
#define sdBYTES_PER_MB                       ( 1024ull * 1024ull )
#define sdSECTORS_PER_MB                     ( sdBYTES_PER_MB / 512ull )
#define sdIOMAN_MEM_SIZE                     4096

#define FF_PRINTF

void GenerateTestFile( char *pcDestinationFileName, int length )
{
    char buffer[512];
    int pattern_len = sizeof(buffer);
    FF_FILE *pxDestinationFile;
    int remain, bytes2send;
    int i;

    for(i=0; i<256; i++)
        sprintf(&buffer[i*2], "%02x", i);

    pxDestinationFile = ff_fopen( pcDestinationFileName, "w+" );
    if( pxDestinationFile != NULL )
    {
        while (length) {
            bytes2send = length > pattern_len ?pattern_len :length;
            remain = bytes2send;
            while(remain) {
                remain -= ff_fwrite( &buffer[bytes2send - remain], 1, remain, pxDestinationFile );
            }
            length -= bytes2send;
        }
    }

    ff_fclose( pxDestinationFile);
}

BaseType_t xCopyFile( char *pcSourceFileName, char *pcDestinationFileName )
{
    FF_FILE *pxSourceFile, *pxDestinationFile;
    size_t xCount;
    uint8_t ucBuffer[ 512 ];

    /* Open the source file in read only mode. */
    pxSourceFile = ff_fopen( pcSourceFileName, "r" );

    if( pxSourceFile != NULL )
    {
        /* Create or overwrite a writable file. */
        pxDestinationFile = ff_fopen( pcDestinationFileName, "w+" );

        if( pxDestinationFile != NULL )
        {
            for( ;; )
            {
                /* Read sizeof( ucBuffer ) bytes from the source file into a buffer. */
                xCount = ff_fread( ucBuffer, 1, sizeof( ucBuffer ), pxSourceFile );

                /* Write however many bytes were read from the source file into the
                destination file. */
                ff_fwrite( ucBuffer, 1, xCount, pxDestinationFile );
                printk(".");
                if( xCount < sizeof( ucBuffer ) )
                {
                    /* The end of the flie was reached. */
                    break;
                }
            }

            /* Close the destination file. */
            ff_fclose( pxDestinationFile );
        }

        /* Close the source file. */
        ff_fclose( pxSourceFile );
    }
}

