/*
 * main2.c
 *
 *  Created on: 20嚙踝蕭
 *      Author: kusoyao
 */
#include <SD_Card/sd_card.h>
#include <task_esm.h>
#include <time.h>
#include <stdio.h>
#include <string.h> // 提供 strncmp, strtok
#include <stdlib.h> // 提供 atof (字串轉浮點數)

#include "FreeRTOS.h"
#include "os_task.h"
#include "os_queue.h"
#include "os_timer.h"
#include "os_event_groups.h"

/* TCPIP related headers */
#include "FreeRTOSIPConfig.h"
#include "FreeRTOSTIMEConfig.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "FreeRTOS_TCP_server.h"

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_ramdisk.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_emif.h"
#include "HL_gio.h"
#include "HL_mibspi.h"
#include "HL_spi.h"
#include "HL_adc.h"
#include "HL_sys_core.h"
#include "HL_sys_dma.h"
#include "HL_sci.h"
#include "HL_can.h"
#include "HL_emac.h"
#include "HL_i2c.h"
#include "minmea.h"   // 引入mnea的解析庫

#include "global.h"
#include "task_spi.h"
#include "task_jpeg.h"
#include "task_console.h"
#include "task_adc.h"
#include "task_can.h"
#include "task_gio.h"
#include "task_esm.h"
#include "task_watchdog.h"
#include "task_thermal_vacuum.h"
#include "F021_flash.h"
#include "utils.h"
#include "UART_API.h"
#include "task_tk2_sicd.h"
#include "obc_io.h"  // For IO port debug purpose
#include "task_uart_protocol.h"
#include "task_uart_protocol_PC.h"
#include "xioctl.h"
#include "rtc.h"

/* List of TaskHandles */
TaskHandle_t xTaskHandle_FIRST;
TaskHandle_t xTaskHandle_CONSOLE;
TaskHandle_t xTaskHandle_ADC;
TaskHandle_t xTaskHandle_CAN;
TaskHandle_t xTaskHandle_SPI;
TaskHandle_t xTaskHandle_JPEG;
TaskHandle_t xTaskHandle_TCPServer;
TaskHandle_t xTaskHandle_ESM;
TaskHandle_t xTaskHandle_WATCHDOG;
TaskHandle_t xTaskHandle_GIO;
TaskHandle_t xTaskHandle_THERMAL_VACUUM;
TaskHandle_t xTaskHandle_I2C;
TaskHandle_t xTaskHandle_RTC;
TaskHandle_t xTaskHandle_I2C_SICD;
TaskHandle_t xTaskHandle_EMPTY_TEST;
TaskHandle_t xTaskHandle_UART;
TaskHandle_t xTaskHandle_SD;

/* List of Tasks */
#define TASK_PRIORITY_FIRST ((tskIDLE_PRIORITY + 4) | portPRIVILEGE_BIT)
#define TASK_PRIORITY_CONSOLE ((tskIDLE_PRIORITY + 3) | portPRIVILEGE_BIT)
#define TASK_PRIORITY_ADC (tskIDLE_PRIORITY + 1)
#define TASK_PRIORITY_CAN (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_SPI (tskIDLE_PRIORITY + 2| portPRIVILEGE_BIT)
#define TASK_PRIORITY_JPEG (tskIDLE_PRIORITY + 2)
#define TASK_TCP_SERVER_PRIORITY ( (tskIDLE_PRIORITY + 3) | portPRIVILEGE_BIT)
#define TASK_PRIORITY_ESM ( (tskIDLE_PRIORITY + 2) | portPRIVILEGE_BIT)
#define TASK_PRIORITY_WATCHDOG ( tskIDLE_PRIORITY + 4)
#define TASK_PRIORITY_GIO ( tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_THERMAL_VACUUM ( tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_I2C (tskIDLE_PRIORITY + 4)
#define TASK_PRIORITY_RTC (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_UART (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_SD (tskIDLE_PRIORITY + 3)

void Init_IO(void);

void vTask_first(void *param);

extern uint8   emacAddress[6U];
extern uint32  emacPhyAddress;

const uint8_t ucIPAddress[4] = {configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3};
const uint8_t ucNetMask[4] = {configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3};
const uint8_t ucGatewayAddress[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
const uint8_t ucDNSServerAddress[4] = {configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3};

/* FTP and HTTP servers execute in the TCP server work task. */
void vServerWorkTask(void *pvParameters);

/* system time */
volatile time_t xSysTimeSeconds = configTIME_START_EPOCH_TIME;
volatile unsigned int xSysTimeMsec;
volatile unsigned int xHighPrecisionTimerUsecMSB = 0;

/* RAM disk parameters */
#define mainRAM_DISK_SECTOR_SIZE    512UL
#define mainRAM_DISK_SECTORS        ((2048 * 1024UL) / mainRAM_DISK_SECTOR_SIZE)
#define mainIO_MANAGER_CACHE_SIZE   (15UL * mainRAM_DISK_SECTOR_SIZE)

/* RAM disk mount point */
#define mainRAM_DISK_NAME           "/ram"
//static uint8_t ucRAMDisk[ mainRAM_DISK_SECTORS * mainRAM_DISK_SECTOR_SIZE ];
uint8_t *ucRAMDisk = SRAM2_ADDR;

#if( configSUPPORT_STATIC_ALLOCATION == 1 )
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t IdleTaskTCB;
    static StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &IdleTaskTCB;
    *ppxIdleTaskStackBuffer = IdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#if( configUSE_TIMERS == 1)
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
    static StaticTask_t TimerTaskTCB;
    static StackType_t TimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &TimerTaskTCB;
    *ppxTimerTaskStackBuffer = TimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif

#endif

#ifdef configGENERATE_RUN_TIME_STATS
void vConfigureTimerForRunTimeStats()
{
    //REUSE RTI COUNTER 0
}

unsigned long vGetRunTimeCounterValue()
{
    //Read Free Running Counter.  ( configCPU_CLOCK_HZ / 2 ) is too fast, will quickly overflow. calculate it to 100000/second
    volatile uint32_t *portRTI_CNT0_FRC0_REG = 0xFFFFFC10;
    uint32_t freq = ( configCPU_CLOCK_HZ / 2 ) / 100000;
    return (*portRTI_CNT0_FRC0_REG) / freq;
}
#endif

/** ***************************************************************************************************
 * @fn      const char *pcApplicationHostnameHook(void)
 * @brief   DHCP hostname register hook function.
 * @details
 * Assign the name defined with "mainDEVICE_NICK_NAME" to this network node during DHCP.
 */
BaseType_t xApplicationDNSQueryHook(const char *pcName)
{
BaseType_t xReturn;

    /* Determine if a name lookup is for this node.  Two names are given
    to this node: that returned by pcApplicationHostnameHook() and that set
    by mainDEVICE_NICK_NAME. */
    if( strcmp( pcName, pcApplicationHostnameHook() ) == 0 )
    {
        xReturn = pdPASS;
    }
    else if( strcmp( pcName, mainDEVICE_NICK_NAME ) == 0 )
    {
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent)
{
    static BaseType_t xTasksAlreadyCreated = pdFALSE;
    BaseType_t res = pdFALSE;

    if( eNetworkEvent == eNetworkUp )
    {
        if( xTasksAlreadyCreated == pdFALSE )
        {
            //print ip info
//            cmd_ip_handle(0, 0);

            /* Start the UDP command line on port 5001 */
            //vStartUDPCommandInterpreterTask( mainUDP_CLI_TASK_STACK_SIZE, mainUDP_CLI_PORT_NUMBER, mainUDP_CLI_TASK_PRIORITY );

            /* Start TCP server task (HTTP, FTP) - move to command line "ip a"*/
            //res = xTaskCreate(vServerWorkTask, "TCPSrv", 4096 / sizeof( StackType_t ), NULL, TASK_TCP_SERVER_PRIORITY, &xTaskHandle_TCPServer);
            //if(res != pdTRUE )
            //{
            //    printk("OOM, Error when create TCP Servers.\n");
            //}

            xTasksAlreadyCreated = pdTRUE;
        }

    }
}

/** ***************************************************************************************************
 * @fn      const char *pcApplicationHostnameHook(void)
 * @brief   DHCP hostname register hook function.
 * @details
 * Assign the name defined with "mainDEVICE_NICK_NAME" to this network node during DHCP.
 */
const char *pcApplicationHostnameHook(void)
{
    return mainDEVICE_NICK_NAME;
}

/** ***************************************************************************************************
 * @fn      void vApplicationTickHook(void)
 * @brief   TICK hook function.
 */
void vApplicationTickHook(void)
{
    xSysTimeMsec++;
    if(xSysTimeMsec >= 1000)
    {
        xSysTimeMsec = 0;
        xSysTimeSeconds++;
    }
}

/** ***************************************************************************************************
 * @fn      void vApplicationIdleHook(void)
 * @brief   IDLE hook function.
 */
void vApplicationIdleHook(void)
{
}

/** ***************************************************************************************************
 * @fn      void vApplicationMallocFailedHook(void)
 * @brief   Malloc() fail hook function.
 */
void vApplicationMallocFailedHook(void)
{
    volatile uint32_t ulMallocFailures = 0;
    ulMallocFailures++;
}

/** ***************************************************************************************************
 * @fn      void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
 * @brief   Stack overflow hook function.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    ( void ) pcTaskName;
    configASSERT(0);
}

#if(ipconfigSUPPORT_OUTGOING_PINGS == 1)
/** ***************************************************************************************************
 * @fn      void vApplicationPingReplyHook(ePingReplyStatus_t eStatus, uint16_t usIdentifier)
 * @brief   Ping reply hook function.
 */
void vApplicationPingReplyHook(ePingReplyStatus_t eStatus, uint16_t usIdentifier)
{
static const char *pcSuccess = "Ping reply received - identifier %d\r\n";
static const char *pcInvalidChecksum = "Ping reply received with invalid checksum - identifier %d\r\n";
static const char *pcInvalidData = "Ping reply received with invalid data - identifier %d\r\n";

    switch(eStatus)
    {
        case eSuccess   :
            FreeRTOS_printf((pcSuccess, (int)usIdentifier));
            break;
        case eInvalidChecksum :
            FreeRTOS_printf((pcInvalidChecksum, (int)usIdentifier));
            break;
        case eInvalidData :
            FreeRTOS_printf((pcInvalidData, (int)usIdentifier));
            break;
        default :
            /* It is not possible to get here as all enums have their own case. */
            break;
    }
    /* Prevent compiler warnings in case FreeRTOS_debug_printf() is not defined. */
    (void) usIdentifier;
}
#endif

/*----------------------------------------------------------------------*/
// MAX-M10S flying mode command
const uint8_t setAirborne[] = {
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x21, 0x00, 0x11, 0x20, 0x08,
  0xF4, 0x51
};
int main2()
{
    /* Reference: http://www.loszi.hu/ */
    /* HCG Setting
     * FreeRTOS:
     * |-- Driver Enable: enable GIO SCI1 SCI3 SCI4 SPI3 SPI4 MIBSPI5 ADC1 EMAC EMIF driver, disable other
     * |-- R5-MPU-PMU:
     * | The processor嚙編 L1 cache does not cache shared normal regions. This means that a region marked as shared is always a non-cached region(this device does not support L2 cache).
     * | Change Region 5 Configuration: 0x60000000 Type: NORMAL_OIWBWA_SHARED  Size: 16MB  Disable-Sub-Region: un-check all
     * | Change Region 6 Configuration: 0x64000000 Type: NORMAL_OIWBWA_NONSHARED  Size: 16MB  Disable-Sub-Region: un-check all
     * |-- VIM Channel: enable ESM High & Low, LIN1 High & Low, SCI3 High & Low, CAN1 High & Low, DMA FTCA LFSA HBCA BTCA, EMAC C0_MISC_PULSE C0_TX_PULSE C_THRSH_PULSE C0_RX_PULSE
     * |-- RAM: User, Supervisor, FIQ, IRZ, Abort, Undefined Stack set to 0x00001000
     * |-- GCM: HCLK Divider => 2, VCLK1 VCLK2 VCLK3 => 0, VCLKA4 Post Src => PLL2_ODCLK_8, RTI1 Pre Src => OSC, RTI1 Divider => 3, RTI1 Post Src => PRE1
     * |-- PLL: PLL 1 Configuration Multiplier 165
     * OS:
     * |-- General: Total Heap Size: 131072
     * |-- Use Mutexes, Idle Hook, Recursive Mutexes, Tick Hook, Counting Semaphores, Malloc Failed Hook, Stack Overflow Hook
     * PINMUX:
     * |-- PIN Muxing: SCI3 SCI4 MIBSPI1 MIBSPI2 MIBSPI3 MIBSPI4 MIBSPI5 EMIF, No need select MII
     * |-- PIN Conflict select: MIBSPI2NENA MIBSPI1NCS[0] MIBSPI3NENA SCI4RX(A13), SCI4TX(B13)
     * |-- Input Pin Muxing: All Default Terminal
     * |-- Special Pin Muxing: enable Temp Sensor 1 2 3, Enable EMIF-CLK output, EMIF Output enable
     * EMIF: PINMUX conflict select EMIF_nWE. disable EMIF_RNW at D17.
     * |-- fill EMIF SDRAM timing parameter, see http://e2e.ti.com/support/microcontrollers/hercules/f/312/t/756750?TMS570LS3137-Unable-to-access-HDK-on-board-SDRAM-through-16-bit-buffered-EMIF
     * |-- Enable EMIF SDRAM(no hardware but enable it) ASYNC1 ASYNC2
     * |-- EMIF ASYNC1 & ASYNC2: W_SETUP: 0, W_STROBE: 0, W_HOLD: 0, R_SETUP: 0, R_STROBE: 1, R_HOLD: 0, TA: 0, ASIZE: 16_bit
     * GIO:
     * |-- PORT A: check ALL DIR Box(Output mode)
     * |-- PORT B: clear ALL DIR Box(Input mode), Bit 0 Interrupt Enable, Bit 1 Interrupt Enable, Bit 2 Interrupt Enable
     * SCI1: UART DEBUG PORT
     * |-- SCI Data Format: 982143-8-N-1
     * |-- Enable RXINT
     * SCI3: RS485 to outside
     * |-- SCI Data Format: 115200-8-N-1
     * |-- Enable RXINT
     * SCI4: UART to StarTracker
     * |-- SCI Data Format: 115200-8-N-1
     * MIBSPI1 CS1 - StarTracker
     * |-- Unused
     * MIBSPI2 CS0 - StarTracker
     * |-- Unused
     * SPI3 CS0 - StarTracker, CS1 - NAND Flash
     * |--
     * SPI4 CS0 - StarTracker, CS1 - NAND Flash
     * |--
     * MIBSPI 5 as slave: CS0 - StarTracker
     * |-- Global: uncheck Master Mode , Internal Clock
     * |-- MISSPIx Data Formats: default
     * |-- MISSPIx Delay: default
     * |-- MISSPIx Transfer Groups:
     * |--  |-- TG0: Buffer Mode 6, enable Chip Select Hold, Chip Select Hold fc...
     * |-- MISSPIx Port: SCS[0] Pin Mode change to GIO
     * ADC1:
     * |-- General: Cycle Time 290.91ns
     * |-- Group1: Fifo Size 7, Enable Channel Id in Conversion Results, Channel Selection Pin 0 ~ 6
     * CAN:
     * |-- CAN1 Msg1: Activate TX ID=1
     * |-- CAN1 Msg2: Activate RX ID=1 Enable Int
     * EMAC:
     * |-- EMAC ADDRESS: 00:08:EE:03:A6:6C
     * |-- Number of Receive Packet 20
     *
     * PHY ID: 0x2000 A240
     *
     * */

    // GPIO default Input, pull low
    gioInit();
    gioSetDirection(gioPORTA, 0xFF); // All output
    gioSetDirection(gioPORTB, 0x0); // All input

    //emif_SDRAMInit(); //NO SDRAM, BUT ENABLE SDRAM in HCG will increase SRAM speed about 5%.......
    emif_ASYNC1Init();
    emif_ASYNC2Init();
    //emif_ASYNC3Init();

    //memset(SDRAM_ADDR, 0, SDRAM_SIZE); //NO SDRAM
    memset(SRAM1_ADDR, 0, SRAM1_SIZE); //33 ms, 60MB/s
    memset(SRAM2_ADDR, 0, SRAM2_SIZE); //33 ms, 60MB/s
    //memset(SRAM3_ADDR, 0, SRAM3_SIZE);

    Init_IO();  // Initialize IO ports

    uint8_t FailMessage[] = "FirstTask Create fail.";
    if(xTaskCreate( vTask_first, "first", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_FIRST, &xTaskHandle_FIRST ) != pdTRUE)
    {
        uart_write(CONSOLE_PORT, sizeof(FailMessage), FailMessage);
    }
    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* Should not reach here */
    while(1);

    return 0;
}


const char build[] = "BOOT " __DATE__ " " __TIME__ "\n";
const char version_info[] = "SoftWare Version :" STR_SOFTWARE_VERSION "." STR_SOFTWARE_BUILD "\n";
const char liscotech[] =
"  _      _____  _____  _____ ____ _______ ______ _____ _    _\n"
" | |    |_   _|/ ____|/ ____/ __ \\__   __|  ____/ ____| |  | |\n"
" | |      | | | (___ | |   | |  | | | |  | |__ | |    | |__| |\n"
" | |      | |  \\___ \\| |   | |  | | | |  |  __|| |    |  __  |\n"
" | |____ _| |_ ____) | |___| |__| | | |  | |___| |____| |  | |\n"
" |______|_____|_____/ \\_____\\____/  |_|  |______\\_____|_|  |_|\n";

// 獨立的 GNSS 處理任務
#define GNSS_PORT 3 //UART 4是port 3
#define CONSOLE_PORT 0
TaskHandle_t xTaskHandle_GNSS;
#define LINE_BUFFER_SIZE 85
QueueHandle_t gnss_q = NULL;
extern uint8_t SCI4RXBUF;    // 從底層拉這個變數過來用
extern uint8_t index;

//GNSS 二進位
typedef enum {
    STATE_SYNC_1,   // 0xA0
    STATE_SYNC_2,   // 0xA1
    STATE_LEN_H,    // 長度高位元組
    STATE_LEN_L,    // 長度低位元組
    STATE_PAYLOAD,  // 有效載荷
    STATE_CHECKSUM, // 接收CS
    STATE_END_1,    // 0x0D
    STATE_END_2     // 0x0A
} ParserState;

ParserState gnss_state = STATE_SYNC_1;
uint16_t gnss_payload_len = 0;
uint16_t gnss_payload_idx = 0;
uint8_t gnss_payload_buf[1024];
uint8_t gnss_checksum = 0;
uint8_t gnss_expected_checksum = 0;

void process_gnss_packet(uint8_t *buf, uint16_t len);

//這串是NMEA的文字輸出，只能測量經緯度和高度等等數值。
//void vTask_gnss(void *param)
//{
//    //建立信箱
//    gnss_q = xQueueCreate(256, sizeof(uint8_t));
//    // 初始化 GNSS UART 介面
//    uart_init((void*)GNSS_PORT);
//    // 設定 GNSS 的通訊速率為 115200，和設置成nonblock
//    uart_ioctl((void*)GNSS_PORT, (void*)UART_CTL_BAUDRATE, (void*)IO_UART_MAX_SPEED, 0);
//    uart_ioctl((void*)GNSS_PORT, (void*)UART_CTL_BLOCK_MODE, (void*)NONE_BLOCKING, (void*)IO_SCI_RX_INT);
//    // 等待 GNSS 模組開機就緒
//    vTaskDelay(pdMS_TO_TICKS(2000));
//    // 發送飛行模式設定指令
//    uart_write((void*)GNSS_PORT, sizeof(setAirborne), (uint8_t*)setAirborne);
//    vTaskDelay(pdMS_TO_TICKS(500));
//
//    uint8_t rx_byte;
//
//    // 進入 GNSS
//    char line_buffer[LINE_BUFFER_SIZE];
//    int line_pos = 0;
//    char print_buf[128];
//    uart_read((void*)GNSS_PORT, 1, &SCI4RXBUF);
//    for(;;)
//    {
////        printf("%d", index);
//        // 檢查 GNSS 是否有吐資料出來
//
//        if (xQueueReceive(gnss_q, &rx_byte, portMAX_DELAY) == pdTRUE)
////        if(uart_rx_Ready((void*)GNSS_PORT) == 1)
//        {
//                uart_write((void*)CONSOLE_PORT, 1, &rx_byte);
//
//                // 判斷是不是遇到換行
//                if (rx_byte == '\n' || rx_byte == '\r')
//                {
//                    if (line_pos > 0)
//                    {
//                        line_buffer[line_pos] = '\0';
//                        // 檢查開頭是不是 $GNGGA 或 $GPGGA
//                        if (strncmp(line_buffer, "$GNGGA", 6) == 0 || strncmp(line_buffer, "$GPGGA", 6) == 0)
//                        {
//                            // strtok 複製一份來切
//                            char temp_buf[LINE_BUFFER_SIZE];
//                            strcpy(temp_buf, line_buffer);
//
//                            // 準備用來接資料的指標
//                            char *lat_str = "";
//                            char *lon_str = "";
//                            char *alt_str = "";
//                            char *sats_str = "";
//
//                            // 專屬 NMEA 的安全切割法
//                            int current_comma = 0;
//                            char *current_ptr = temp_buf;
//                            char *field_start = temp_buf;
//
//                            // 走訪整個字串
//                            while (*current_ptr != '\0' && *current_ptr != '*')
//                            {
//                                if (*current_ptr == ',')
//                                {
//                                    *current_ptr = '\0'; // 把逗號換成字串結尾符號 '\0'，這樣就切開了
//
//                                    // 根據剛切出來的這段，把指標存起來
//                                    if (current_comma == 2) lat_str = field_start;
//                                    else if (current_comma == 4) lon_str = field_start;
//                                    else if (current_comma == 7) sats_str = field_start;
//                                    else if (current_comma == 9) alt_str = field_start;
//
//                                    current_comma++;
//                                    field_start = current_ptr + 1; // 下一個欄位的起點在逗號的下一格
//                                }
//                                current_ptr++;
//                            }
//
//                            // 處理最後一個欄位 (或是遇到 '*' 號停止)
//                            if (*current_ptr == '*') {
//                                *current_ptr = '\0'; // 去掉 '*' 後面的 Checksum
//                                if (current_comma == 9) alt_str = field_start;
//                            }
//
//                            // 逗號切字串
////                            char *token = strtok(temp_buf, ",");
////                            int comma_count = 0;
////
////                            // 準備用來接資料的指標
////                            char *lat_str = "";
////                            char *lon_str = "";
////                            char *alt_str = "";
////                            char *sats_str = "";
////
////                            // 一直切，直到沒有逗號為止
////                            while (token != NULL) {
////                                if (comma_count == 2) lat_str = token;      // 第 2 個逗號後是緯度
////                                else if (comma_count == 4) lon_str = token; // 第 4 個逗號後是經度
////                                else if (comma_count == 7) sats_str = token;// 第 7 個逗號後是衛星數
////                                else if (comma_count == 9) alt_str = token; // 第 9 個逗號後是高度
////
////                                token = strtok(NULL, ","); // 繼續切
////                                comma_count++;
////                            }
//
////                            int len = sprintf(print_buf, "latitude:%s, longitude:%s, height:%s m, Satellite:%s\r\n",
////                                              lat_str, lon_str, alt_str, sats_str);
////                            uart_write((void*)CONSOLE_PORT, len, (uint8_t*)print_buf);
//                        }
//                        // 解析處理完畢清空，準備接下一句話
//                        line_pos = 0;
//                    }
//                }
//                else
//                {
//                    // 如果還沒遇到換行，就把收到的 Byte 塞進水桶裡
//                    if (line_pos < LINE_BUFFER_SIZE - 1)
//                        line_buffer[line_pos++] = (char)rx_byte;
//                }
//
//        }
//    }
//}

//下面的是主要的 binary 解析，需要對 GNSS 發送 0x1E 才能傳輸解析二進位封包
void vTask_gnss(void *param)
{
    // 建立信箱
    gnss_q = xQueueCreate(2048, sizeof(uint8_t));

    // 初始化 UART
    uart_init((void*)GNSS_PORT);

    // 設定通訊速率:115200
    uart_ioctl((void*)GNSS_PORT, (void*)UART_CTL_BAUDRATE, (void*)IO_UART_MAX_SPEED, 0);
    uart_ioctl((void*)GNSS_PORT, (void*)UART_CTL_BLOCK_MODE, (void*)NONE_BLOCKING, (void*)IO_SCI_RX_INT);

    // 等待 GNSS 模組開機就緒
//    vTaskDelay(pdMS_TO_TICKS(5000));

    // 發送飛行模式設定指令
//    uart_write((void*)GNSS_PORT, sizeof(setAirborne), (uint8_t*)setAirborne);
//    vTaskDelay(pdMS_TO_TICKS(500));

    // 中斷接收
    uart_read((void*)GNSS_PORT, 1, &SCI4RXBUF);
    //發送 Binary 輸出的0x1E
    const uint8_t enable_binary_cmd[] = {
         0xA0, 0xA1, 0x00, 0x09, 0x1E, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x1F, 0x0D, 0x0A};
    for (int i = 0; i < 4; i++) {
            uart_write((void*)GNSS_PORT, sizeof(enable_binary_cmd), (uint8_t*)enable_binary_cmd);
            vTaskDelay(pdMS_TO_TICKS(500)); // 每隔 0.5 s發一次，發三次出去給 GNSS
        }

    uart_write((void*)GNSS_PORT, sizeof(enable_binary_cmd), (uint8_t*)enable_binary_cmd);
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t rx_byte;

    for(;;)
    {
        if (xQueueReceive(gnss_q, &rx_byte, portMAX_DELAY) == pdTRUE)
        {
            // FSM
//            uart_write((void*)CONSOLE_PORT, 1, &rx_byte);
            switch (gnss_state) {
                case STATE_SYNC_1:
                    if (rx_byte == 0xA0) gnss_state = STATE_SYNC_2;
                    break;

                case STATE_SYNC_2:
                    if (rx_byte == 0xA1) gnss_state = STATE_LEN_H;
                    else if (rx_byte != 0xA0) gnss_state = STATE_SYNC_1;
                    break;

                case STATE_LEN_H:
                    gnss_payload_len = rx_byte << 8;
                    gnss_state = STATE_LEN_L;
                    break;

                case STATE_LEN_L:
                    gnss_payload_len |= rx_byte;
                    if (gnss_payload_len > 1024 || gnss_payload_len == 0) {
                        gnss_state = STATE_SYNC_1;
                    } else {
                        gnss_payload_idx = 0;
                        gnss_checksum = 0;
                        gnss_state = STATE_PAYLOAD;
                    }
                    break;

                case STATE_PAYLOAD:
                    gnss_payload_buf[gnss_payload_idx++] = rx_byte;
                    gnss_checksum ^= rx_byte;

                    if (gnss_payload_idx == gnss_payload_len) {
                        gnss_state = STATE_CHECKSUM;
                    }
                    break;

                case STATE_CHECKSUM:
                    gnss_expected_checksum = rx_byte;
                    gnss_state = STATE_END_1;
                    break;

                case STATE_END_1:
                    if (rx_byte == 0x0D) gnss_state = STATE_END_2;
                    else gnss_state = STATE_SYNC_1;
                    break;

                case STATE_END_2:
                    if (rx_byte == 0x0A)
                    {
                        if (gnss_checksum == gnss_expected_checksum)
                        {
                            process_gnss_packet(gnss_payload_buf, gnss_payload_len);
                        }
                        else
                        {
                            // 印出錯誤提示
                             printk("GNSS Checksum Error!\r\n");
                        }
                    }
                    gnss_state = STATE_SYNC_1;
                    break;
            }
        }
    }
}
void process_gnss_packet(uint8_t *buf, uint16_t len)
{
    uint8_t msg_id = buf[0];
    switch(msg_id)
    {
        case 0xDF: // 接收機導航狀態 (RCV_STATE)
            if (len == 81) {
                uint8_t nav_state = buf[2]; // Navigation State

                if (nav_state == 3 || nav_state == 2) {
                    double ecef_x, ecef_y, ecef_z;
                    memcpy(&ecef_x, &buf[13], 8);
                    memcpy(&ecef_y, &buf[21], 8);
                    memcpy(&ecef_z, &buf[29], 8);

                    printk("[PVT 0xDF] 3D located! X: %d, Y: %d\r\n", (int)ecef_x, (int)ecef_y);
                } else {
                    printk("[PVT 0xDF] still locating...(State: %d)\r\n", nav_state);
                }
            }
            break;

        case 0xDD: // 原始觀測量 (RAW_MEAS)
            if (len > 3) {
                uint8_t num_sats = buf[2]; // NMEAS
                printk("[RAW 0xDD] RAW_MEAS, amount of sats: %d \r\n", num_sats);
            }
            break;

        case 0xE5: // 擴展原始觀測量 (EXT_RAW_MEAS)
            if (len > 14) {
                uint8_t num_sats = buf[13]; // NMEAS 在第 13 個 byte
                printk("[RAW 0xE5] Extended RAW_MEAS, amount of sats: %d \r\n", num_sats);
            }
            break;

        case 0xB1: // GPS 星曆 (Ephemeris)
            printk("[EPH 0xB1] GPS calendar received!\r\n");
            break;

        case 0x1E:// input 訊號，OBC TXRX對接情況下進入該 case
            printk("the message is 0x%2X, tx and rx from OBC are connecting each other.\r\n", msg_id);
            break;

        default:
            printk("Unknown ID: 0x%02X\r\n", msg_id);
            break;
    }
}


void vTask_first(void *param)
{
    int res = 0;

    printk_str(build, sizeof(build) - 1);
    printk_str(liscotech, sizeof(liscotech) - 1);
    printk_str(version_info, sizeof(version_info) - 1);
    res += xTaskCreate( vTask_console, "console", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_CONSOLE, &xTaskHandle_CONSOLE );
//    res += xTaskCreate( vTask_uart_protocol_2, "uart", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_UART, &xTaskHandle_UART );
    res += xTaskCreate( vTask_adc, "adc", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_ADC, &xTaskHandle_ADC );  // read ADC from TK2
    //res += xTaskCreate( vTask_can, "can", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_CAN, &xTaskHandle_CAN );
//    res += xTaskCreate( vTask_spi, "spi", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_SPI, &xTaskHandle_SPI );
    res += xTaskCreate( vTask_esm, "esm", 1024 / sizeof( StackType_t ), 0, TASK_PRIORITY_ESM, &xTaskHandle_ESM );
//    res += xTaskCreate( vTask_gio, "gio", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_GIO, &xTaskHandle_GIO );
    res += xTaskCreate( vTask_initRTC, "rtc", 2048 / sizeof( StackType_t ), 0, TASK_PRIORITY_RTC, &xTaskHandle_RTC );
//   被註解調 res += xTaskCreate( vTask_uart_protocol, "spi", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_SPI, &xTaskHandle_SPI );
    //res += xTaskCreate( vTask_gio, "gio", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_GIO, &xTaskHandle_GIO );
//    res += xTaskCreate( vTask_watchdog, "watchdog", 1024 / sizeof( StackType_t ), 0, TASK_PRIORITY_WATCHDOG, &xTaskHandle_WATCHDOG );
    //res += xTaskCreate( vTask_thermal_vacuum, "vacuum", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_THERMAL_VACUUM, &xTaskHandle_THERMAL_VACUUM ); // for thermal_vacuum test
    //res += xTaskCreate( vTask_esm, "esm", 1024 / sizeof( StackType_t ), 0, TASK_PRIORITY_ESM, &xTaskHandle_ESM );
    //res += xTaskCreate( vTask_i2c, "i2c", 2048 / sizeof( StackType_t ), 0, TASK_PRIORITY_I2C, &xTaskHandle_I2C );
    //res += xTaskCreate( vTask_tk2_sicd, "i2c_sicd", 2048 / sizeof( StackType_t ), 0, TASK_PRIORITY_GIO, &xTaskHandle_I2C_SICD );
    //res += xTaskCreate( vTask_jpeg, "jpeg", 3072 / sizeof( StackType_t ), 0, TASK_PRIORITY_JPEG, &xTaskHandle_JPEG );
    //res += xTaskCreate( vTask_uart, "uart", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_UART, &xTaskHandle_UART );
    //res += xTaskCreate( vTask_sd_test, "SD", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_SD, &xTaskHandle_SD );
    //initialize ramdisk - move to command line
    //disk_init();

    //initialize network - move to command line "ip s"
    //reset_ethernet_phy();
    //FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, emacAddress);

    res += xTaskCreate( vTask_gnss, "gnss", 4096 / sizeof( StackType_t ), 0, TASK_PRIORITY_UART, &xTaskHandle_GNSS );

    //釋放資源
    vTaskDelete(0);
}

void disk_init()
{
    FF_Disk_t *pxDisk;

    /* Create the RAM disk. */
    pxDisk = FF_RAMDiskInit(mainRAM_DISK_NAME, ucRAMDisk, mainRAM_DISK_SECTORS, mainIO_MANAGER_CACHE_SIZE);
    configASSERT(pxDisk);

    /* Print out information on the disk. */
    FF_RAMDiskShowPartition(pxDisk);

    /* Create example files and web pages on the disk */
    //vCreateAndVerifyExampleFiles(mainRAM_DISK_NAME);
}

void vServerWorkTask(void *pvParameters)
{
    TCPServer_t *pxTCPServer = NULL;
    const TickType_t xInitialBlockTime = pdMS_TO_TICKS(200UL);

    static const struct xSERVER_CONFIG xServerConfiguration[] =
    {
        /* Server type,     port number,    backlog,    root dir. */
        { eSERVER_HTTP,     80,             10,         configHTTP_ROOT },
        { eSERVER_FTP,      21,             10,          "" }
    };

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    /* Configuring RTI timer for serving system time. - move to command line "ip n"*/
    //vConfigureTimerForSysTime();
    //vStartNTPTask(configMINIMAL_STACK_SIZE * 2, 4);

    /* Create the servers defined by the xServerConfiguration array above. */
    pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
    configASSERT( pxTCPServer );

    while(1)
    {
        FreeRTOS_TCPServerWork(pxTCPServer, xInitialBlockTime);
    }
}
