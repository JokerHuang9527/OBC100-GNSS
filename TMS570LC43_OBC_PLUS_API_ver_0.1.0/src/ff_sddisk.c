/*
 * FreeRTOS+FAT V2.3.3
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
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

#define FF_PRINTF   printk

/*
 * Mutex for partition.
 */
static SemaphoreHandle_t xPlusFATMutex = NULL;
nand_device_t *sdcard;

static int32_t prvReadSD( uint8_t * pucBuffer,
                          uint32_t ulSectorNumber,
                          uint32_t ulSectorCount,
                          FF_Disk_t * pxDisk )
{
    int32_t lReturnCode;
    if( ( pxDisk != NULL ) &&
        ( pxDisk->ulSignature == sdSIGNATURE ) &&
        ( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
        ( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
        ( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
    {
        if( disk_read(0, pucBuffer, ulSectorNumber, ulSectorCount) == 0)
            lReturnCode = ulSectorCount * 512;
    }
    else
    {
        /* Make sure no random data is in the returned buffer. */
        memset( ( void * ) pucBuffer, '\0', ulSectorCount * 512UL );

        if( pxDisk->xStatus.bIsInitialised != pdFALSE )
        {
            FF_PRINTF( "prvFFRead: warning: %u + %u > %u\n", ( unsigned int ) ulSectorNumber, ( unsigned int ) ulSectorCount, ( unsigned int ) pxDisk->ulNumberOfSectors );
        }
    }

    return lReturnCode;
}
/*-----------------------------------------------------------*/

static int32_t prvWriteSD( uint8_t * pucBuffer,
                           uint32_t ulSectorNumber,
                           uint32_t ulSectorCount,
                           FF_Disk_t * pxDisk )
{
    int32_t lReturnCode = FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_ERRFLAG;

    if( ( pxDisk != NULL ) &&
        ( pxDisk->ulSignature == sdSIGNATURE ) &&
        ( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
        ( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
        ( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
    {
        if( disk_write(0, pucBuffer, ulSectorNumber, ulSectorCount) == 0)
            lReturnCode = ulSectorCount * 512;
    }
    else
    {
        if( pxDisk->xStatus.bIsInitialised != pdFALSE )
        {
            FF_PRINTF( "prvFFWrite: warning: %u + %u > %u\n", ( unsigned ) ulSectorNumber, ( unsigned ) ulSectorCount, ( unsigned ) pxDisk->ulNumberOfSectors );
        }
    }

    return lReturnCode;
}
/*-----------------------------------------------------------*/

void FF_SDDiskFlush( FF_Disk_t * pxDisk )
{
    if( ( pxDisk != NULL ) &&
        ( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
        ( pxDisk->pxIOManager != NULL ) )
    {
        FF_FlushCache( pxDisk->pxIOManager );
    }
}
/*-----------------------------------------------------------*/

FF_Disk_t * FF_SDDiskInit( const char * pcName, int b_mount )
{
    FF_Error_t xFFError;
    BaseType_t xPartitionNumber = 0;
    FF_CreationParameters_t xParameters;
    FF_Disk_t * pxDisk;

    sdcard = SD_CARD_FLASH_Init();

    if( sdcard->is_initialized != 1 )
    {
        FF_PRINTF( "FF_SDDiskInit: prvSDMMCInit failed\n" );
        pxDisk = NULL;
    }
    else
    {
        pxDisk = ( FF_Disk_t * ) ffconfigMALLOC( sizeof( *pxDisk ) );

        if( pxDisk == NULL )
        {
            FF_PRINTF( "FF_SDDiskInit: Malloc failed\n" );
        }
        else
        {
            /* Initialise the created disk structure. */
            memset( pxDisk, '\0', sizeof( *pxDisk ) );

            pxDisk->ulNumberOfSectors = sd_get_sector_count();

            if( xPlusFATMutex == NULL )
            {
                xPlusFATMutex = xSemaphoreCreateRecursiveMutex();
            }

            pxDisk->ulSignature = sdSIGNATURE;

            if( xPlusFATMutex != NULL )
            {
                memset( &xParameters, '\0', sizeof( xParameters ) );
                xParameters.ulMemorySize = sdIOMAN_MEM_SIZE;
                xParameters.ulSectorSize = 512;
                xParameters.fnWriteBlocks = prvWriteSD;
                xParameters.fnReadBlocks = prvReadSD;
                xParameters.pxDisk = pxDisk;

                /* prvFFRead()/prvFFWrite() are not re-entrant and must be
                 * protected with the use of a semaphore. */
                xParameters.xBlockDeviceIsReentrant = pdFALSE;

                /* The semaphore will be used to protect critical sections in
                 * the +FAT driver, and also to avoid concurrent calls to
                 * prvFFRead()/prvFFWrite() from different tasks. */
                xParameters.pvSemaphore = ( void * ) xPlusFATMutex;

                pxDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xFFError );
                if( pxDisk->pxIOManager == NULL )
                {
                    FF_PRINTF( "FF_SDDiskInit: FF_CreateIOManager: %s\n", ( const char * ) FF_GetErrMessage( xFFError ) );
                    FF_SDDiskDelete( pxDisk );
                    pxDisk = NULL;
                }
                else
                {
                    pxDisk->xStatus.bIsInitialised = pdTRUE;
                    pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
                    if(b_mount)
                    {
                        if( FF_SDDiskMount( pxDisk ) == 0 )
                        {
                            FF_SDDiskDelete( pxDisk );
                            pxDisk = NULL;
                        }
                        else
                        {
                            if( pcName == NULL )
                            {
                                pcName = "/";
                            }

                            FF_FS_Add( pcName, pxDisk );
                            FF_PRINTF( "FF_SDDiskInit: Mounted SD-card as root \"%s\"\n", pcName );
                            FF_SDDiskShowPartition( pxDisk );
                        }
                    }
                } /* if( pxDisk->pxIOManager != NULL ) */
            }     /* if( xPlusFATMutex != NULL) */
        }         /* if( pxDisk != NULL ) */
    }             /* if( xSDCardStatus == pdPASS ) */

    return pxDisk;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskFormat( FF_Disk_t * pxDisk )
{
    FF_Error_t xError;
    BaseType_t xReturn = pdFAIL;
    FF_PartitionParameters_t xPartition;

    xError = FF_Unmount( pxDisk );

    if( FF_isERR( xError ) != pdFALSE )
    {
        FF_PRINTF( "FF_SDDiskFormat: unmount fails: %08x\n", ( unsigned ) xError );
    }
    else
    {
        /* Create a single partition that fills all available space on the disk. */
        memset( &xPartition, '\0', sizeof( xPartition ) );
        xPartition.ulSectorCount = pxDisk->ulNumberOfSectors;
        xPartition.ulHiddenSectors = 8;
        xPartition.xPrimaryCount = 1;
        xPartition.eSizeType = eSizeIsQuota;

        /* Partition the disk */
        xError = FF_Partition( pxDisk, &xPartition );
        FF_PRINTF( "FF_Partition: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

        /* Format the drive - try FAT32 with large clusters. */
        xError = FF_Format( pxDisk, 0, pdFALSE, pdFALSE );

        if( FF_isERR( xError ) )
        {
            FF_PRINTF( "FF_SDDiskFormat: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
        }
        else
        {
            FF_PRINTF( "FF_SDDiskFormat: OK, now remounting\n" );
            //pxDisk->xStatus.bPartitionNumber = 0;
            xError = FF_SDDiskMount( pxDisk );
            FF_PRINTF( "FF_SDDiskFormat: rc %08x\n", ( unsigned ) xError );

            if( FF_isERR( xError ) == pdFALSE )
            {
                xReturn = pdPASS;
                FF_SDDiskShowPartition( pxDisk );
            }
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskUnmount( FF_Disk_t * pxDisk )
{
    FF_Error_t xFFError;
    BaseType_t xReturn = pdPASS;

    if( ( pxDisk != NULL ) && ( pxDisk->xStatus.bIsMounted != pdFALSE ) )
    {
        pxDisk->xStatus.bIsMounted = pdFALSE;
        xFFError = FF_Unmount( pxDisk );

        if( FF_isERR( xFFError ) )
        {
            FF_PRINTF( "FF_SDDiskUnmount: rc %08x\n", ( unsigned ) xFFError );
            xReturn = pdFAIL;
        }
        else
        {
            FF_PRINTF( "Drive unmounted\n" );
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskReinit( FF_Disk_t * pxDisk )
{
    BaseType_t xStatus = disk_initialize( 0 ); /* Hard coded index. */

    /*_RB_ parameter not used. */
    ( void ) pxDisk;

    FF_PRINTF( "FF_SDDiskReinit: rc %08x\n", ( unsigned ) xStatus );
    return xStatus;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskMount( FF_Disk_t * pxDisk )
{
    FF_Error_t xFFError;
    BaseType_t xReturn;

    /* Mount the partition */
    xFFError = FF_Mount( pxDisk, pxDisk->xStatus.bPartitionNumber );

    if( FF_isERR( xFFError ) )
    {
        FF_PRINTF( "FF_SDDiskMount: %08lX\n", xFFError );
        xReturn = pdFAIL;
    }
    else
    {
        pxDisk->xStatus.bIsMounted = pdTRUE;
        FF_PRINTF( "****** FreeRTOS+FAT initialized %u sectors\n", ( unsigned ) pxDisk->pxIOManager->xPartition.ulTotalSectors );
        xReturn = pdPASS;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

FF_IOManager_t * sddisk_ioman( FF_Disk_t * pxDisk )
{
    FF_IOManager_t * pxReturn;

    if( ( pxDisk != NULL ) && ( pxDisk->xStatus.bIsInitialised != pdFALSE ) )
    {
        pxReturn = pxDisk->pxIOManager;
    }
    else
    {
        pxReturn = NULL;
    }

    return pxReturn;
}
/*-----------------------------------------------------------*/

/* Release all resources */
BaseType_t FF_SDDiskDelete( FF_Disk_t * pxDisk )
{
    if( pxDisk != NULL )
    {
        pxDisk->ulSignature = 0;
        pxDisk->xStatus.bIsInitialised = 0;

        if( pxDisk->pxIOManager != NULL )
        {
            if( FF_Mounted( pxDisk->pxIOManager ) != pdFALSE )
            {
                FF_Unmount( pxDisk );
            }

            FF_DeleteIOManager( pxDisk->pxIOManager );
        }

        vPortFree( pxDisk );
    }

    return 1;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskShowPartition( FF_Disk_t * pxDisk )
{
    FF_Error_t xError;
    uint64_t ullFreeSectors;
    uint32_t ulTotalSizeMB, ulFreeSizeMB;
    int iPercentageFree;
    FF_IOManager_t * pxIOManager;
    const char * pcTypeName = "unknown type";
    BaseType_t xReturn = pdPASS;

    if( pxDisk == NULL )
    {
        xReturn = pdFAIL;
    }
    else
    {
        pxIOManager = pxDisk->pxIOManager;

        FF_PRINTF( "Reading FAT and calculating Free Space\n" );

        switch( pxIOManager->xPartition.ucType )
        {
            case FF_T_FAT12:
                pcTypeName = "FAT12";
                break;

            case FF_T_FAT16:
                pcTypeName = "FAT16";
                break;

            case FF_T_FAT32:
                pcTypeName = "FAT32";
                break;

            default:
                pcTypeName = "UNKOWN";
                break;
        }

        FF_GetFreeSize( pxIOManager, &xError );

        ullFreeSectors = pxIOManager->xPartition.ulFreeClusterCount * pxIOManager->xPartition.ulSectorsPerCluster;
        iPercentageFree = ( int ) ( ( pxIOManager->xPartition.ulDataSectors / 2 ) /
                                    ( ( uint64_t ) pxIOManager->xPartition.ulDataSectors ) );

        ulTotalSizeMB = pxIOManager->xPartition.ulDataSectors / sdSECTORS_PER_MB;
        ulFreeSizeMB = ( uint32_t ) ( ullFreeSectors / sdSECTORS_PER_MB );

        /* It is better not to use the 64-bit format such as %Lu because it
         * might not be implemented. */
        FF_PRINTF( "Partition Nr   %8u\n", pxDisk->xStatus.bPartitionNumber );
        FF_PRINTF( "Type           %8u (%s)\n", pxIOManager->xPartition.ucType, pcTypeName );
        FF_PRINTF( "VolLabel       '%8s' \n", pxIOManager->xPartition.pcVolumeLabel );
        FF_PRINTF( "TotalSectors   %8u\n", ( unsigned ) pxIOManager->xPartition.ulTotalSectors );
        FF_PRINTF( "SecsPerCluster %8u\n", ( unsigned ) pxIOManager->xPartition.ulSectorsPerCluster );
        FF_PRINTF( "Size           %8u MB\n", ( unsigned ) ulTotalSizeMB );
        FF_PRINTF( "FreeSize       %8u MB ( %d perc free )\n", ( unsigned ) ulFreeSizeMB, iPercentageFree );
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

/* This routine returns true if the SD-card is inserted.  After insertion, it
 * will wait for sdCARD_DETECT_DEBOUNCE_TIME_MS before returning pdTRUE. */
BaseType_t FF_SDDiskDetect( FF_Disk_t * pxDisk )
{
    int xReturn;

    return xReturn;
}
/*-----------------------------------------------------------*/
