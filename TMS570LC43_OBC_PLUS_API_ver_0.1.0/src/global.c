/*
 * global.c
 *
 *  Created on: 2019¦~8¤ë23¤é
 *      Author: kusoyao
 */
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "user_sci.h"
#include "HL_gio.h"
#include "ff_time.h"
#include "uart.h"
#include "tk2_storage.h"
//#include "HL_mibspi.h"

uint8_t tk2_project_debug_flag = DEFAULT_DEBUG;

//volatile uint16_t spi_rx_buffer[IMAGE_LINE_COUNT/2][IMAGE_LINE_WIDTH/2] __SRAM1_SECTION__;
uint8_t spi_rx_buffer[65280] __SRAM2_SECTION__;
uint8_t spi_tx_buffer[65280] __SRAM1_SECTION__;
uint8_t imageinfo_buffer[17] __SRAM2_SECTION__;
uint8_t api_tx_buffer[512]   __SRAM1_SECTION__;
uint8_t api_rx_buffer[512]   __SRAM1_SECTION__;
uint8_t api_tx_buffer2[512]   __SRAM1_SECTION__;
uint8_t api_rx_buffer2[512]   __SRAM1_SECTION__;
//const uint8_t image_buffer_jpeg_dma[JPG_DMA_BUFFER_SIZE] __SRAM1_SECTION__;
//const image_buffer_t image_buffers __SRAM2_SECTION__;
//const uint8_t image_buffer_jpeg[JPG_BUFFER_SIZE] __SRAM2_SECTION__;
//const uint8_t image_buffer_thumbnail[THUMBNAIL_BUFFER_SIZE] __SRAM2_SECTION__;
//const uint8_t image_buffer_thumbnail_jpeg[JPG_BUFFER_SIZE/5] __SRAM2_SECTION__;
//const uint8_t image_buffer_test[200000] __SRAM2_SECTION__;
const  FF_TimeStruct_t TimeBuff[1] __SRAM2_SECTION__;
//const uint8_t image_buffer_thumbnail_jpeg[THUMBNAIL_BUFFER_SIZE] __SRAM2_SECTION__;
uint8_t sci_debug_buffer[CONSOLE_BUFFER_SIZE];// __SRAM1_SECTION__;

/* for testing SRAM3 */
//volatile uint16_t spi_rx_buffer_sram3[IMAGE_LINE_COUNT + add_image_id_data][IMAGE_LINE_SIZE / 2] __SRAM3_SECTION__;

//volatile HouseKeepingDataFormat houseKeepingData = {0} ;
volatile HouseKeepingDataFormat houseKeepingData __SRAM2_SECTION__ ;
volatile uint8_t testbuf[512] __SRAM2_SECTION__;

QueueHandle_t qin;
uint8_t SCI1RXBUF;
uint8_t SCI3RXBUF;
uint8_t SCI4RXBUF;
struct scidma_handle *sciconsole;
struct scidma_handle *scidata;
const int printk_len_limit = 256;
static int debug_buffer_offset = 0;
static int debug_buffer_offset_ni = 0;

volatile int gmode = 2;
volatile int goutmode = 4;
volatile int gframe_size = 0;
volatile int gframe_id = 0;
volatile uint8_t radiation_test = 0;
volatile uint8_t adc_enable = 0;
uint8_t transfer_time=0;
uint8_t save_photo_number=0xff;
uint8_t save_photo_number_mode3_Photo2=0xff;
uint8_t save_photo_number_mode3_Photo3=0xff;
uint32_t u32_test_read=0;
uint32_t jpeg_state_flag=0;
/* default value is disabled */
volatile uint8_t change_OBC_mode_from_SF2_switch = 0;

SemaphoreHandle_t sem_image_ready;
SemaphoreHandle_t sem_image_processed;
SemaphoreHandle_t sem_image_number;

QueueHandle_t queue_esm;
QueueHandle_t queue_i2c_sicd;
QueueHandle_t queue_gio;
QueueHandle_t queue_can1;
QueueHandle_t queue_SF2_reset;
QueueHandle_t queue_i2c;

int global_init()
{
    //console
    qin = xQueueCreate(1024 , sizeof( uint8_t ) ); // user may copy and paste a lot of command
    ASSERT(qin != NULL);
//    sciReceive(CONSOLE_PORT, 1, &SCI1RXBUF); //start next char receive, co-work with ti's driver, a stupid way...
//    sciReceive(RSI2000_PORT, 1, &SCI3RXBUF);
//    sciReceive(PC_PORT, 1, &SCI4RXBUF);
    uart_read(CONSOLE_PORT, 1, &SCI1RXBUF);
    uart_read(RSI2000_PORT, 1, &SCI3RXBUF);
//    uart_read(PC_PORT, 1, &SCI4RXBUF);
//    mibspiInit();
    /* sciReceive(STARTRACKER_PORT, 1, &SCI1RXBUF); */

    memset((uint8_t *)&houseKeepingData,0, sizeof(houseKeepingData));
    houseKeepingData.noTimeError = 1;
    houseKeepingData.powerStatus = (gioGetBit(gioPORTB, 0) << 2U) | (gioGetBit(gioPORTA, 1) << 1U ) | (gioGetBit(gioPORTA, 0) << 0U);

    sem_image_ready = xSemaphoreCreateBinary();
    ASSERT(sem_image_ready != NULL );
    sem_image_processed = xSemaphoreCreateBinary();
    ASSERT(sem_image_processed != NULL );
    xSemaphoreGive( sem_image_processed );
	
    sciconsole = sciDmaTxInit(sciREG1, true);
    //if(STARTRACKER_PORT == CONSOLE_PORT)
    //    scidata = sciconsole;
    //else
    //    scidata = sciDmaTxInit(STARTRACKER_PORT, true);
    //scidata = sciDmaTxInit(STARTRACKER_PORT, true);

    queue_esm = xQueueCreate( 128, sizeof( uint8_t ) );
    ASSERT( queue_esm != 0 );

    queue_gio = xQueueCreate( 128, sizeof( uint8_t ) );
    ASSERT( queue_gio != 0 );

    queue_i2c_sicd = xQueueCreate( 128, sizeof( uint8_t ) );
    ASSERT( queue_i2c_sicd != 0 );

    queue_can1 = xQueueCreate( 64, sizeof( uint8_t ) );
    ASSERT( queue_can1 != 0 );
	
	queue_SF2_reset = xQueueCreate( 16, sizeof( uint8_t ) );
    ASSERT( queue_SF2_reset != 0 );

    queue_i2c = xQueueCreate( 128, sizeof( uint8_t ) );
    ASSERT( queue_i2c != 0 );

  	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint8_t msg = 0;

    return 0;
}

//dont use printk in interrupt or callback. can only transfer printk_len_limit byte.
//call printk_str if you want to transfer raw string.
//call sciDmaTx if you want to transfer a lot of data.
int printk(const char *format, ...)
{

    char *buffer;
    int len;
    va_list args;

    taskENTER_CRITICAL();
    buffer = sci_debug_buffer + debug_buffer_offset;

    debug_buffer_offset += printk_len_limit;
    if(debug_buffer_offset >= CONSOLE_BUFFER_SIZE)
        debug_buffer_offset = 0;
    taskEXIT_CRITICAL();

    va_start( args, format);
    len = vsnprintf( buffer, printk_len_limit, format, args );
    va_end( args );

    sciDmaTx(sciconsole, (const uint8_t *)buffer, len);
    //sciPollTx(CONSOLE_PORT, buffer, len);
//    uart_write(CONSOLE_PORT, len, buffer);
    return len;
}

int printk_str(const char *string, int len)
{
    char *buffer;
    int bsend;

    while(len > 0)
    {
        taskENTER_CRITICAL();
        buffer = sci_debug_buffer + debug_buffer_offset;

        debug_buffer_offset += printk_len_limit;
        if(debug_buffer_offset >= CONSOLE_BUFFER_SIZE)
            debug_buffer_offset = 0;
        taskEXIT_CRITICAL();

        if(len > printk_len_limit)
            bsend = printk_len_limit;
        else
            bsend = len;

        memcpy(buffer, string, bsend);

        sciDmaTx(sciconsole, (const uint8_t *)buffer, bsend);
        //sciPollTx(CONSOLE_PORT, buffer, bsend);
//        uart_write(CONSOLE_PORT, len, buffer);
        len -= bsend;
        string += bsend;
    }

    return len;
}

int printk_ni(const char *format, ...)
{

    char *buffer;
    int len;
    va_list args;
    uint8_t sci_debug_buffer_ni[CONSOLE_BUFFER_SIZE];
    buffer = sci_debug_buffer_ni + debug_buffer_offset_ni;

    debug_buffer_offset_ni += printk_len_limit;
    if(debug_buffer_offset_ni >= CONSOLE_BUFFER_SIZE)
        debug_buffer_offset_ni = 0;

    va_start( args, format);
    len = vsnprintf( buffer, printk_len_limit, format, args );
    va_end( args );
//    sciDmaTx(sciconsole, (const uint8_t *)buffer, len);
    uart_write(CONSOLE_PORT, len, buffer);
    return len;
}
