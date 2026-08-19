/*
 * nand_disk.c
 *
 *  Created on: 2019¦~12¤ë4¤é
 *      Author: kusoyao
 */


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
//#include "portable.h"

#include "ff_headers.h"
#include "ff_devices.h"

#include "HL_sys_common.h"
#include "ftl.h"

FF_Disk_t *NandDisk = NULL;

int32_t prvWriteNAND( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
    FTL_STATUS status;
    ftl_handle_t * ftl = (ftl_handle_t *)pxDisk->pvTag;
    int32_t lReturn = FF_ERR_NONE;
    int i;
    for(i = 0; i < ulSectorCount; ++i)
    {
        status = ftl_write_page(ftl, ulSectorNumber + i, pucBuffer + i*512); //512B ONLY
        if(status != FTL_SUCCESS)
            return FF_ERR_DEVICE_DRIVER_FAILED;
    }
    return lReturn;
}

int32_t prvReadNAND( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
    FTL_STATUS status;
    ftl_handle_t * ftl = (ftl_handle_t *)pxDisk->pvTag;
    int32_t lReturn = FF_ERR_NONE;
    int i;
    for(i = 0; i < ulSectorCount; ++i)
    {
        status = ftl_read_page(ftl, ulSectorNumber + i, pucBuffer + i*512); //512B ONLY
        if(status != FTL_SUCCESS)
            return FF_ERR_DEVICE_DRIVER_FAILED;
    }
    return lReturn;
}

BaseType_t nand_disk_delete( FF_Disk_t *pxDisk )
{
    if( pxDisk != NULL )
    {
        pxDisk->ulSignature = 0;
        pxDisk->xStatus.bIsInitialised = 0;
        if( pxDisk->pxIOManager != NULL )
        {
            FF_DeleteIOManager( pxDisk->pxIOManager );
        }

        vPortFree( pxDisk );
    }

    return pdPASS;
}

FF_Error_t nand_disk_mount( FF_Disk_t *pxDisk)
{
    char *pcName = "/nand1";
    FF_Error_t xError;
    int PARTITION_NUMBER = 0;

    pxDisk->xStatus.bIsInitialised = pdTRUE;

    /* Record the partition number the FF_Disk_t structure is, then
    mount the partition. */
    pxDisk->xStatus.bPartitionNumber = PARTITION_NUMBER;

    /* Mount the partition. */
    xError = FF_Mount( pxDisk, PARTITION_NUMBER );
    FF_PRINTF( "FF_RAMDiskInit: FF_Mount: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

    if( FF_isERR( xError ) == pdFALSE )
    {
        /* The partition mounted successfully, add it to the virtual
        file system - where it will appear as a directory off the file
        system's root directory. */
        FF_FS_Add( pcName, pxDisk );
    }

    return xError;
}

FF_Error_t nand_disk_format( FF_Disk_t *pxDisk)
{
    //modify from FF_Error_t NandPartitionAndFormatDisk( FF_Disk_t *pxDisk )
    FF_PartitionParameters_t xPartition;
    FF_Error_t xError;

    if(pxDisk == NULL)
        return FF_ERR_NULL_POINTER;

    /* Record that the RAM disk has been initialised. */
    pxDisk->xStatus.bIsInitialised = pdTRUE;

    /* Create a partition on the NAND disk.
     * Do not perform this step if the media have already been partitioned. */

    /* Create a single partition that fills all available space on the disk. */
    memset( &xPartition, '\0', sizeof( xPartition ) );
    xPartition.ulSectorCount = pxDisk->ulNumberOfSectors;
    xPartition.ulHiddenSectors = 8;
    xPartition.xPrimaryCount = 1;
    xPartition.eSizeType = eSizeIsQuota;

    /* Partition the disk */
    xError = FF_Partition( pxDisk, &xPartition );
    FF_PRINTF( "FF_Partition: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

    if( FF_isERR( xError ) == pdFALSE )
    {
        /* Format the partition. */
        xError = FF_Format( pxDisk, 0, pdTRUE, pdTRUE );
        FF_PRINTF( "FF_RAMDiskInit: FF_Format: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
    }

    return xError;
}

FF_Disk_t * nand_disk_init()
{
    /*
    In this example:
     + pcName is the name to give the disk within FreeRTOS+FAT's virtual file system.
     + pucDataBuffer is the start of the RAM to use as the disk.
     + ulSectorCount is effectively the size of the disk, each sector is 512 bytes.
     + xIOManagerCacheSize is the size of the IO manager's cache, which must be a
       multiple of the sector size, and at least twice as big as the sector size.
     */
    ftl_handle_t * ftl = ftl_init();

    FF_Error_t xError;
    FF_CreationParameters_t xParameters;
    size_t xIOManagerCacheSize = 2 * ftl->dev->page_size;

    /* Attempt to allocated the FF_Disk_t structure. */
    if(NandDisk == NULL)
    {
        NandDisk = ( FF_Disk_t * ) pvPortMalloc( sizeof( FF_Disk_t ) );
    }
    else
        return NandDisk;

    if( NandDisk != NULL )
    {
        /* Start with every member of the structure set to zero. */
        memset( NandDisk, '\0', sizeof( FF_Disk_t ) );

        /* The pvTag member of the FF_Disk_t structure allows the structure to be
        extended to also include media specific parameters.  The only media
        specific data that needs to be stored in the FF_Disk_t structure for a
        RAM disk is the location of the RAM buffer itself - so this is stored
        directly in the FF_Disk_t's pvTag member. */
        NandDisk->pvTag = ( void * ) ftl;

        /* The signature is used by the disk read and disk write functions to
        ensure the disk being accessed is a RAM disk. */
        NandDisk->ulSignature = ftl->magic;

        /* The number of sectors is recorded for bounds checking in the read and
        write functions. */
        NandDisk->ulNumberOfSectors = ftl->dev->num_pages / 16 * 15;

        /* Create the IO manager that will be used to control the RAM disk. */
        memset( &xParameters, '\0', sizeof( xParameters ) );
        xParameters.pucCacheMemory = NULL;
        xParameters.ulMemorySize = xIOManagerCacheSize;
        //xParameters.ulSectorSize = 512;//ftl->dev->page_size;//must be 512 TODO
        xParameters.ulSectorSize = (ftl->dev->page_size / 512) * 512  ;//must be 512 TODO
        xParameters.fnWriteBlocks = prvWriteNAND;
        xParameters.fnReadBlocks = prvReadNAND;
        xParameters.pxDisk = NandDisk;

        /* Driver is reentrant so xBlockDeviceIsReentrant can be set to pdTRUE.
        In this case the semaphore is only used to protect FAT data
        structures. */
        xParameters.pvSemaphore = ( void * ) xSemaphoreCreateRecursiveMutex();
        xParameters.xBlockDeviceIsReentrant = pdFALSE;

        NandDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xError );
        if( ( NandDisk->pxIOManager == NULL ) || ( FF_isERR( xError ) != pdFALSE ) )
        {
            FF_PRINTF( "FF_RAMDiskInit: FF_CreateIOManger: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

            /* The disk structure was allocated, but the disk's IO manager could
            not be allocated, so free the disk again. */
            nand_disk_delete( NandDisk );
            NandDisk = NULL;
        }
    }
    else
    {
        printk("alloc pxDisk failed\n");
    }
    return NandDisk;
}


