/*
 * task_console.c
 *
 *  Created on: 2019嚙羯10嚙踝蕭18嚙踝蕭
 *      Author: kusoyao
 */


#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_time.h"

#include "FreeRTOSTIMEConfig.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "NTPDemo.h"
#include "os_portmacro.h"

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_sci.h"
#include "HL_gio.h"
#include "HL_emac.h"
#include "HL_hw_reg_access.h"
#include "HL_esm.h"
#include "HL_adc.h"
#include "HL_sys_vim.h"
#include "HL_rti.h"
#include "HL_i2c.h"
#include "HL_reg_i2c.h"


#include "utils.h"
#include "user_sci.h"
#include "cmd.h"
#include "global.h"
#include "F021_flash.h"
#include "nand_flash.h"
#include "ftl.h"
#include "task_adc.h"
#include "UART_API.h"
#include "rtc.h"
#include "task_uart_protocol.h"
#include "task_uart_protocol_PC.h"
#include "tk2_storage.h"
#include "xioctl.h"
#include "obc_io.h"

extern QueueHandle_t queue_SF2_reset;
uint8_t SF2_RESET;
char cmd_buffer[MAX_CMD_LEN];
cmd_t *first_cmd = 0;
int RxOverflow = 0;
FF_Disk_t *pxSDDisk = NULL;

void prvCreateFileInfoString( char *pcBuffer, FF_FindData_t *pxFindStruct );
BaseType_t prvPerformCopy( const char *pcSourceFile, int32_t lSourceFileLength, const char *pcDestinationFile);
//
int cmd_gpio_handle(int argc, char **argv);
int cmd_cmp_handle(int argc, char **argv);
int cmd_mcp_handle(int argc, char **argv);
int cmd_mc_handle(int argc, char **argv);
int cmd_md_handle(int argc, char **argv);
int cmd_mm_handle(int argc, char **argv);
int cmd_mw_handle(int argc, char **argv);
int cmd_memtest_handle(int argc, char **argv);
int cmd_watchdog_handle(int argc, char **argv);
int cmd_reset_handle(int argc, char **argv);
int cmd_mode_handle(int argc, char **argv);
int cmd_debug_handle(int argc, char **argv);
int cmd_phy_handle(int argc, char **argv);
int cmd_ip_handle(int argc, char **argv);
int cmd_top_handle(int argc, char **argv);
int cmd_ping_handle(int argc, char **argv);
int cmd_f021_handle(int argc, char **argv);
int cmd_sddisk_handle(int argc, char **argv);
int cmd_nanddisk_handle(int argc, char **argv);
int cmd_nand_handle(int argc, char **argv);
int cmd_storage_handle(int argc, char **argv);
int cmd_date_handle(int argc, char **argv);
int cmd_esm_handle(int argc, char **argv);
int cmd_mrc_handle(int argc, char **argv);
int cmd_adc_handle(int argc, char **argv);
int cmd_st_handle(int argc, char **argv);
int cmd_info_handle(int argc, char **argv);
int cmd_help_handle(int argc, char **argv);
//VFS command
int cmd_ls_handle(int argc, char **argv);
int cmd_cd_handle(int argc, char **argv);
int cmd_cat_handle(int argc, char **argv);
int cmd_rm_handle(int argc, char **argv);
int cmd_rmdir_handle(int argc, char **argv);
int cmd_mkdir_handle(int argc, char **argv);
int cmd_cp_handle(int argc, char **argv);
int cmd_pwd_handle(int argc, char **argv);
int cmd_console_handle(int argc, char **argv);

/* int cmd_radiation_test_handle(int argc, char **argv); */

int cmd_SD_write_handle(int argc, char **argv);
int cmd_SD_read_handle(int argc, char **argv);

/*------RTC Command------*/
int cmd_RTC_timeset_handle(int argc, char **argv);
int cmd_RTCdate_handle(int argc, char **argv);
int cmd_RTCtest_handle(int argc, char **argv);
/*------RSI2000 Command------*/
int cmd_RSI_Read_handle(int argc, char **argv);
int cmd_RSI_Write_handle(int argc, char **argv);
int cmd_RSI_Info_handle(int argc, char **argv);
int cmd_RSI_Delete_handle(int argc, char **argv);
int cmd_RSI_Capture_handle(int argc, char **argv);
int cmd_RSI_Bad_handle(int argc, char **argv);
int cmd_RSI_Reset_handle(int argc, char **argv);
int cmd_RSI_Prepare_handle(int argc, char **argv);
int cmd_RSI_Downlode_handle(int argc, char **argv);
/*------OBC100 API Command------*/
int cmd_buffer_write_handle(int argc, char **argv);
int cmd_buffer_read_handle(int argc, char **argv);

int cmd_uart_block_handle(int argc, char **argv);
int cmd_uart_speed_handle(int argc, char **argv);
int cmd_uart_read_handle(int argc, char **argv);
int cmd_uart_write_handle(int argc, char **argv);

int cmd_i2c_speed_handle(int argc, char **argv);
int cmd_i2c_read_handle(int argc, char **argv);
int cmd_i2c_write_handle(int argc, char **argv);
int cmd_i2c_transfer_handle(int argc, char **argv);
int cmd_i2c_md_handle(int argc, char **argv);
int cmd_i2c_mm_handle(int argc, char **argv);
int cmd_i2c_mw_handle(int argc, char **argv);

int cmd_spi_info_handle(int argc, char **argv);
int cmd_spi_block_handle(int argc, char **argv);
int cmd_spi_speed_handle(int argc, char **argv);
int cmd_spi_read_handle(int argc, char **argv);
int cmd_spi_write_handle(int argc, char **argv);

int cmd_gpio_info_handle(int argc, char **argv);
int cmd_gpio_output_drain_handle(int argc, char **argv);
int cmd_gpio_input_pull_handle(int argc, char **argv);
int cmd_gpio_pull_dir_handle(int argc, char **argv);
int cmd_gpio_read_handle(int argc, char **argv);
int cmd_gpio_write_handle(int argc, char **argv);

int cmd_can_info_handle(int argc, char **argv);
int cmd_can_id_handle(int argc, char **argv);
int cmd_can_mask_handle(int argc, char **argv);
int cmd_can_speed_handle(int argc, char **argv);
int cmd_can_read_handle(int argc, char **argv);
int cmd_can_write_handle(int argc, char **argv);

#define SF2_command "SF2 <trg | sg (0~5)| uarttest | mode (0~5)>\n"

#define EXT_SIGNAL_command "<EXT gio_num length(ms)> for testing: only GIOA[5 or 6 or 7] creates a pulse(make sure GIOA[5 or 6 or 7] has been connectd to test pin)\n"

#define RAD_command "RAD <ESMW sram3_area(1~4) write_data | ESMC sram3_area(1~4) compare_data | ISMW w data(internal SRAM) | ISMC c data | ENDW external_nand1_area(1~128) SRAM3(1~4) | ENDR EN1A (1~128) SRAM3(1~4) | ENDC EN1A(1~128) SRAM3(1~4) c data >\n"

cmd_t cmd_cmp = { 0, "cmp", "compare memory: cmp addr1 addr2 length [c|w|l]\n", cmd_cmp_handle};
cmd_t cmd_mcp = { &cmd_cmp, "mcp", "memory copy: mcp dest_addr source_addr length\n", cmd_mcp_handle};
//cmd_t cmd_mc = { &cmd_mcp, "mc", "memory check: mc address length value [c|w|l]\n", cmd_mc_handle};
cmd_t cmd_md = { &cmd_mcp, "md", "memory display: md address length [c|w|l]\n", cmd_md_handle};
cmd_t cmd_mm = { &cmd_md, "mm", "memory modify: mm address value [c|w|l]\n", cmd_mm_handle};
cmd_t cmd_mw = { &cmd_mm, "mw", "memory fill: mw address length value [c|w|l]\n", cmd_mw_handle};
//cmd_t cmd_memtest = { &cmd_mw, "memtest", "memtest: <1|2|3> <a|w|r>\n", cmd_memtest_handle};
cmd_t cmd_watchdog = { &cmd_mw, "watchdog", "watchdog: [i|r] \n", cmd_watchdog_handle};
cmd_t cmd_reset = { &cmd_watchdog, "reset", "software reset: reset \n", cmd_reset_handle};
cmd_t cmd_top = { &cmd_reset , "top", "cpu utilization: top\n", cmd_top_handle};
cmd_t cmd_sddisk = { &cmd_top, "sddisk", "sddisk <s|m|f|t|c> \n", cmd_sddisk_handle};
cmd_t cmd_nand1 = { &cmd_sddisk, "nand1", "nand <e|r|w|E|R|W> <dest address> <sector | buffer address> [length] \n", cmd_nand_handle};
cmd_t cmd_nand0 = { &cmd_nand1, "nand0", "nand <e|r|w|E|R|W> <dest address> <sector | buffer address> [length] \n", cmd_nand_handle};
cmd_t cmd_nand = { &cmd_nand0, "nand", "Use command nand0 or nand1 \n", cmd_nand_handle};
cmd_t cmd_date = { &cmd_nand, "date", "print date time\n", cmd_date_handle};
cmd_t cmd_esm = { &cmd_date, "esm", "Error Signaling Module: esm [normal | reset| force | enable(group1) | disable(group1) | Enable(group2) | Disable(group2)] [channel]\n", cmd_esm_handle};
cmd_t cmd_adc = { &cmd_esm, "adc", "control adc convert: adc [enable | disable | print]\n", cmd_adc_handle};
cmd_t cmd_info = { &cmd_adc, "information", "Hardware Information \n", cmd_info_handle};
cmd_t cmd_help = { &cmd_info, "help", "print all command and help message: help [cmd]\n", cmd_help_handle};

//VFS command
cmd_t cmd_dir = { 0, "dir", "same as ls\n", cmd_ls_handle};
cmd_t cmd_ls = { &cmd_dir, "ls", "Lists the files in the current directory\n", cmd_ls_handle};
cmd_t cmd_cd = { &cmd_ls, "cd", "Changes the working directory: cd <dir name>\n" , cmd_cd_handle};
cmd_t cmd_type = { &cmd_cd, "type", "same as cat\n" , cmd_cat_handle};
cmd_t cmd_cat = { &cmd_type, "cat", "Prints file contents to the terminal: cat <filename>\n" , cmd_cat_handle};
cmd_t cmd_del = { &cmd_cat, "del", "same as rm\n" , cmd_rm_handle};
cmd_t cmd_rm = { &cmd_del, "rm", "remove a file: rm <filename|directory>:\n" , cmd_rm_handle};
cmd_t cmd_rmdir = { &cmd_rm, "rmdir", "remove a directory: rmdir <directory name>\n" , cmd_rmdir_handle};
cmd_t cmd_mkdir = { &cmd_rmdir, "mkdir", "create a directory: mkdir <directory name>\n" , cmd_mkdir_handle};
cmd_t cmd_copy = { &cmd_mkdir, "copy", "same as cp\n", cmd_cp_handle};
cmd_t cmd_cp = { &cmd_copy, "cp", "copy a file: cp <source file> <dest file>\n", cmd_cp_handle};
cmd_t cmd_pwd = { &cmd_cp, "pwd", "Print Working Directory\n" , cmd_pwd_handle};
//
cmd_t cmd_SD_write = { &cmd_pwd, "SD_write", "SD Card Write: SD_write <buff_addr> <sector_number> <sector_countr> each sector 512 byte\n", cmd_SD_write_handle};
cmd_t cmd_SD_read = { &cmd_SD_write, "SD_read", "SD Card Read: SD_read <buff_addr> <sector_number> <sector_countr> each sector 512 byte\n", cmd_SD_read_handle};
/* for RTC command */
cmd_t cmd_RTC_timeset = { &cmd_SD_read, "RTC_SET", "RTC_SET: <Year> <Month> <Day> <Hour (24 hour)> <Minutes> <Seconds>\n", cmd_RTC_timeset_handle};
cmd_t cmd_RTC_date = { &cmd_RTC_timeset, "RTC_DATE", "RTC_DATE: print RTC_date time\n", cmd_RTCdate_handle};
//cmd_t cmd_RTCtest = { &cmd_RTC_date, "RTC_TEST", "test RTC <uint time>\n", cmd_RTCtest_handle};
/* for RSI2000 command */
cmd_t cmd_RSI_Read = { &cmd_RTC_date, "RSI_READ", "RSI_READ [index] : Read RSI Data\n", cmd_RSI_Read_handle};
cmd_t cmd_RSI_Write = { &cmd_RSI_Read, "RSI_WRITE", "RSI_WRITE [index] [value] : Write RSI Data\n", cmd_RSI_Write_handle};
cmd_t cmd_RSI_Info = { &cmd_RSI_Write, "RSI_INFO", "RSI_INFO [index/new/old/all] [value/amount]: Dump RSI Image Info\n", cmd_RSI_Info_handle};
cmd_t cmd_RSI_Delete = { &cmd_RSI_Info , "RSI_DELETE", "RSI_DELETE [index/new/old/all] [value]: Delete RSI Image\n", cmd_RSI_Delete_handle};
cmd_t cmd_RSI_Capture = { &cmd_RSI_Delete, "RSI_CAP", "RSI_CAP: Send Image Capture Command\n", cmd_RSI_Capture_handle};
cmd_t cmd_RSI_Bad = { &cmd_RSI_Capture, "RSI_BAD", "RSI_BAD [index] [value]: Tag RSI Bad Image Index\n", cmd_RSI_Bad_handle};
cmd_t cmd_RSI_Reset = { &cmd_RSI_Bad, "RSI_RESET", "RSI_RESET : Reset RSI\n", cmd_RSI_Reset_handle};
cmd_t cmd_RSI_Prepare = { &cmd_RSI_Reset, "RSI_PRE", "RSI_PRE [index] : Prepare Image\n", cmd_RSI_Prepare_handle};
cmd_t cmd_RSI_Download = { &cmd_RSI_Prepare, "RSI_DOW", "RSI_DOW [index/new/old] (image index) : Download Image from RSI\n", cmd_RSI_Downlode_handle};
cmd_t cmd_storage = { &cmd_RSI_Download, "storage", "storage <s|i|l|d|c> <index>\n", cmd_storage_handle};
/* for OBC100 API command */
cmd_t cmd_buffer_write_HANDLE = { &cmd_storage, "buffer_write", "buffer_write <m/r> <value> \n", cmd_buffer_write_handle};
cmd_t cmd_buffer_read_HANDLE = { &cmd_buffer_write_HANDLE, "buffer_read", "buffer_read <m/r> <length> \n", cmd_buffer_read_handle};

cmd_t cmd_uart_block = { &cmd_buffer_read_HANDLE, "uart_block", "uart_block <device> <mode> <flag>\n", cmd_uart_block_handle};
cmd_t cmd_uart_speed = { &cmd_uart_block, "uart_speed", "uart_speed <device> <value>\n", cmd_uart_speed_handle};
cmd_t cmd_uart_read = { &cmd_uart_speed, "uart_read", "uart_read <device> <length>\n", cmd_uart_read_handle};
cmd_t cmd_uart_write = { &cmd_uart_read, "uart_write", "uart_write <device> <length>\n", cmd_uart_write_handle};

cmd_t cmd_i2c_speed = { &cmd_uart_write, "i2c_speed", "i2c_speed <value>\n", cmd_i2c_speed_handle};
cmd_t cmd_i2c_read = { &cmd_i2c_speed, "i2c_read", "i2c_read <slave_add> <read_add> <length>\n", cmd_i2c_read_handle};
cmd_t cmd_i2c_write = { &cmd_i2c_read, "i2c_write", "i2c_write <slave_add> <write_add> <length>\n", cmd_i2c_write_handle};
cmd_t cmd_i2c_transfer = {&cmd_i2c_write, "i2c_transfer", "i2c_transfer <slave_add> <tx_datalen> <rx_datalen>\n", cmd_i2c_transfer_handle};
cmd_t cmd_i2c_md = { &cmd_i2c_transfer, "i2c_md", " i2c_md <slave_addr> <mem_addr> <length>\n", cmd_i2c_md_handle};
cmd_t cmd_i2c_mm = { &cmd_i2c_md, "i2c_mm", "i2c_mm <slave_addr> <mem_addr> <value>\n", cmd_i2c_mm_handle};
cmd_t cmd_i2c_mw = { &cmd_i2c_mm, "i2c_mw", "i2c_mw <slave_addr> <mem_addr> <length> <val0> [<val1> ...]\n", cmd_i2c_mw_handle};

cmd_t cmd_spi_info = { &cmd_i2c_mw, "spi_info", "spi_info : get spi infomation\n", cmd_spi_info_handle};
cmd_t cmd_spi_block = { &cmd_spi_info, "spi_block", "spi_block <device> <mode> <flag>\n", cmd_spi_block_handle};
cmd_t cmd_spi_speed = { &cmd_spi_block, "spi_speed", "spi_speed <device> <value>\n", cmd_spi_speed_handle};
cmd_t cmd_spi_read = { &cmd_spi_speed, "spi_read", "spi_read <device> <length>\n", cmd_spi_read_handle};
cmd_t cmd_spi_write = { &cmd_spi_read, "spi_write", "spi_write <device> <length>\n", cmd_spi_write_handle};

cmd_t cmd_gpio_info = { &cmd_spi_write, "gpio_info", "gpio_info : get gpio infomation\n", cmd_gpio_info_handle};
cmd_t cmd_gpio_outputdrain = { &cmd_gpio_info, "gpio_drain", "gpio_drain <device> <open/close>\n", cmd_gpio_output_drain_handle};
cmd_t cmd_gpio_inputpull = { &cmd_gpio_outputdrain, "gpio_pull", "gpio_pull <device> <open/close>\n", cmd_gpio_input_pull_handle};
cmd_t cmd_gpio_pulldir = { &cmd_gpio_inputpull, "gpio_pulldir", "gpio_pulldir <device> <up/down>\n", cmd_gpio_pull_dir_handle};
cmd_t cmd_gpio_read = { &cmd_gpio_pulldir, "gpio_read", "gpio_read <device>\n", cmd_gpio_read_handle};
cmd_t cmd_gpio_write = { &cmd_gpio_read, "gpio_write", "gpio_write <device> <high/low>\n", cmd_gpio_write_handle};

cmd_t cmd_can_info = { & cmd_gpio_write, "can_info", "can_info : get can infomation\n", cmd_can_info_handle};
cmd_t cmd_can_id = {&cmd_can_info, "can_id", "can_id <device> <tx/rx> <enable/disable>(Extended identifier) <id>\n", cmd_can_id_handle};
cmd_t cmd_can_mask = {&cmd_can_id, "can_mask", "can_mask <device> <tx/rx> <mask>\n", cmd_can_mask_handle};
cmd_t cmd_can_speed = {&cmd_can_mask, "can_speed", "can_speed <device> <bitrate (kbit/s)>\n", cmd_can_speed_handle};
cmd_t cmd_can_read = {&cmd_can_speed, "can_read", "can_read <device> <datalen>\n", cmd_can_read_handle};
cmd_t cmd_can_write = {&cmd_can_read, "can_write", "can_write <device> <datalen>\n", cmd_can_write_handle};

/* This command unblocks the debug mode */
cmd_t cmd_console = { & cmd_can_write, "console", "Console out\n" , cmd_console_handle};

extern QueueHandle_t gnss_q;

void sciNotification(sciBASE_t *sci, uint32 flags)
{
    int index = uart_get_index(sci);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if((index == CONSOLE_PORT) && (flags == SCI_RX_INT))
    {
        if( xQueueSendFromISR( qin, &SCI1RXBUF, &xHigherPriorityTaskWoken ) != pdTRUE)
            RxOverflow++;

        uart_read(CONSOLE_PORT, 1, &SCI1RXBUF);
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//接收GNSS flag(用UART4)
    else if((index == 3) && (flags == SCI_RX_INT))
    {
        // 1. 如果信箱已經建好，就把剛收到的 SCI4RXBUF 塞進信箱
        if(gnss_q != NULL)
        {
            xQueueSendFromISR( gnss_q, &SCI4RXBUF, &xHigherPriorityTaskWoken );
        }
        // 2. 再次呼叫 uart_read，請硬體去抓「下一個」字元放到 SCI4RXBUF
        uart_read((void*)3, 1, &SCI4RXBUF);
        // 3. 觸發 RTOS 排程切換
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//    else if((index == RSI2000_PORT) && (flags == SCI_RX_INT))
//    {
//        PutByteToReceivePackage();
//        uart_read(RSI2000_PORT, 1, &SCI3RXBUF);
//    }

    //傳輸確認提示，先註解掉
//    else if(flags == SCI_TX_INT){
//        printk_ni("UART COM[%d] transmit complete\n",index + 1);
//    }
//    else if(flags == SCI_RX_INT){
//        printk_ni("UART COM[%d] receive complete\n",index + 1);
//    }

//    if((index == PC_PORT) && (flags == SCI_RX_INT))
//    {
//        PutByteToReceivePackage2();
//        uart_read(PC_PORT, 1, &SCI4RXBUF);
//    }

}

int init_cmd()
{
    first_cmd = &cmd_help;
    register_cmd(&cmd_console);
    return 0;
}

int register_cmd(cmd_t *c)
{
    if(first_cmd == 0)
    {
        first_cmd = c;
        return 0;
    }

    cmd_t *cmd = first_cmd;
    while(cmd->next != 0)
    {
        cmd = cmd->next;
    }

    cmd->next = c;
    return 0;
}

cmd_t * find_cmd(char *name)
{
    cmd_t *cmd = first_cmd;
    while(cmd != 0)
    {
        if(cmd->cmd)
            if(strcmp(name, cmd->cmd) == 0)
                break;
        cmd = cmd->next;
    }

    return cmd;
}

int execute_cmd(int argc, char **argv)
{
    cmd_t *cmd;
    cmd = find_cmd(argv[0]);
    if(cmd == 0)
    {
        return CMD_NOT_FOUND;
    }
    if(cmd->handle)
        return cmd->handle(argc, argv);
    else
        return CMD_NOT_IMPLEMENT;
}


void delay_us(int val) {
    /* this function can delay at least 1us */
    uint32_t td = 0;
    rti_t t1 = {0}, t2 = {0};
    save_rti_time(&t1);
    while(td < val) {
        save_rti_time(&t2);
        td = diff_rti(&t2, &t1);
        td = rti_to_microsecond(td);
    }
}

int cmd_cmp_handle(int argc, char **argv)
{
    uint32_t addr1, addr2, length, diff;
    char unit;

    if(argc < 4)
    {
        printk(cmd_cmp.helpmsg);
        return CMD_FAIL;
    }

    addr1 = strtoul(argv[1], 0, 16);
    addr2 = strtoul(argv[2], 0, 16);
    length = strtoul(argv[3], 0, 16);
    unit = 'l';
    if(argc > 4)
        unit = argv[4][0];

    switch(unit)
    {
    case 'c':
        diff = compare_memory_uint8((uint8_t *)addr1, (uint8_t *)addr2, length / sizeof(uint8_t));
        break;

    case 'w':
        diff = compare_memory_uint16((uint16_t *)addr1, (uint16_t *)addr2, length / sizeof(uint16_t));
        break;

    case 'l':
    default:
        diff = compare_memory_uint32((uint32_t *)addr1, (uint32_t *)addr2, length / sizeof(uint32_t));
        break;
    }
    printk("%u diff\n", diff);
    return CMD_SUCCESS;
}

int cmd_mcp_handle(int argc, char **argv)
{
    uint32_t source, dest, length;

    if(argc < 3)
    {
        printk(cmd_mcp.helpmsg);
        return CMD_FAIL;
    }

    dest = strtoul(argv[1], 0, 16);
    source = strtoul(argv[2], 0, 16);
    length = strtoul(argv[3], 0, 16);

    memcpy((void *)dest, (void *)source, length);
    return CMD_SUCCESS;
}

int cmd_mc_handle(int argc, char **argv)
{
    uint32_t start, length, value;
    char unit;

    if(argc < 4)
    {
//        printk(cmd_mc.helpmsg);
        return CMD_FAIL;
    }

    start = strtoul(argv[1], 0, 16);
    length = strtoul(argv[2], 0, 16);
    value = strtoul(argv[3], 0, 16);
    unit = 'l';
    if(argc > 4)
        unit = argv[4][0];

    switch(unit)
    {
    case 'c':
        compare_memory_pattern_uint8((uint8_t *)start, length / sizeof(uint8_t), value);
        break;

    case 'w':
        compare_memory_pattern_uint16((uint16_t *)start, length / sizeof(uint16_t), value);
        break;

    case 'l':
    default:
        compare_memory_pattern_uint32((uint32_t *)start, length / sizeof(uint32_t), value);
        break;
    }
    return CMD_SUCCESS;
}

int cmd_md_handle(int argc, char **argv)
{
    uint32_t start, length;
    char unit;

    if(argc < 3)
    {
        printk(cmd_md.helpmsg);
        return CMD_FAIL;
    }

    start = strtoul(argv[1], 0, 16);
    length = strtoul(argv[2], 0, 16);
    unit = 'l';
    if(argc > 3)
        unit = argv[3][0];

    switch(unit)
    {
    case 'c':
        dump_memory_uint8((uint8_t *)start, length / sizeof(uint8_t));
        break;

    case 'w':
        dump_memory_uint16((uint16_t *)start, length / sizeof(uint16_t));
        break;

    case 'l':
    default:
        dump_memory_uint32((uint32_t *)start, length / sizeof(uint32_t));
        break;
    }
    return CMD_SUCCESS;
}

int cmd_mm_handle(int argc, char **argv)
{
    uint32_t address, value;
    char unit;

    if(argc < 3)
    {
        printk(cmd_mm.helpmsg);
        return CMD_FAIL;
    }

    address = strtoul(argv[1], 0, 16);
    value = strtoul(argv[2], 0, 16);
    unit = 'l';
    if(argc > 3)
        unit = argv[3][0];

    switch(unit)
    {
    case 'c':
        *(volatile uint8_t *)address = value;
        break;

    case 'w':
        *(volatile uint16_t *)address = value;
        break;

    case 'l':
    default:
        *(volatile uint32_t *)address = value;
        break;
    }
    return CMD_SUCCESS;
}

int cmd_mw_handle(int argc, char **argv)
{
    uint32_t start, length, value;
    char unit;

    if(argc < 4)
    {
        printk(cmd_mw.helpmsg);
        return CMD_FAIL;
    }

    start = strtoul(argv[1], 0, 16);
    length = strtoul(argv[2], 0, 16);
    value = strtoul(argv[3], 0, 16);
    //printk("length:%x(%d)\n", length, length);
    //printk("length/sizeof:%x(%d)\n", length/sizeof(uint32_t), length/sizeof(uint32_t));
    unit = 'l';
    if(argc > 4)
        unit = argv[4][0];

    switch(unit)
    {
    case 'c':
        fill_memory_uint8((uint8_t *)start, length / sizeof(uint8_t), value);
        break;

    case 'w':
        fill_memory_uint16((uint16_t *)start, length / sizeof(uint16_t), value);
        break;

    case 'l':
    default:
        fill_memory_uint32((uint32_t *)start, length / sizeof(uint32_t), value);
        break;
    }
    return CMD_SUCCESS;
}
int  cmd_memtest_handle(int argc, char **argv)
{
    char unit;
    char number;
    uint32_t int_count;
    uint32_t memory_address;
    uint32_t memory_write_data;
    uint32_t sram_address;

     if(argc < 2)
     {
//         printk(cmd_memtest.helpmsg);
         return CMD_FAIL;
     }
     if(argc > 1)
         number = argv[1][0];
     switch(number)
        {
        case '1':
            sram_address=SRAM1_ADDR;
            break;

        case '2':
            sram_address=SRAM2_ADDR;
            break;

        case '3':
        default:
            sram_address=SRAM3_ADDR;
            break;
        }


     if(argc > 2)
         unit = argv[2][0];
    printk("unit = %c \n",unit);
    switch(unit)
       {
       case 'r':
           for(int_count=0;int_count<SRAM1_SIZE;int_count+=1000)
             {
                 if((*(volatile uint32_t *)(sram_address+int_count))==(int_count<<16)+0xaaaa)
                     printk("[%08x] ",int_count);
                 else
                     printk("[%08x]======>FAIL\n",int_count);
             }
           break;

       case 'w':
           for(int_count=0;int_count<SRAM1_SIZE;int_count+=1000)
              {
                  memory_address=sram_address+int_count;
                  memory_write_data=(int_count<<16)+0xaaaa;
                  *(volatile uint32_t *)memory_address=memory_write_data;
                  if(*(volatile uint32_t *)memory_address!=memory_write_data)
                      printk("W[%08x]==>FAIL\n",memory_address);
              }
           break;

       case 'a':
       default:
           for(int_count=0;int_count<SRAM1_SIZE;int_count+=1000)
            {
                memory_address=sram_address+int_count;
                memory_write_data=(int_count<<16)+0xaaaa;
                *(volatile uint32_t *)memory_address=memory_write_data;
                if(*(volatile uint32_t *)memory_address!=memory_write_data)
                    printk("W[%08x]==>FAIL\n",memory_address);
            }

            for(int_count=0;int_count<SRAM1_SIZE;int_count+=1000)
            {
                if((*(volatile uint32_t *)(sram_address+int_count))==(int_count<<16)+0xaaaa)
                    printk("[%08x] ",int_count);
                else
                    printk("[%08x]======>FAIL\n",int_count);
            }
           break;
       }




    return CMD_SUCCESS;
}
int cmd_watchdog_handle(int argc, char **argv)
{
    char op = 'g'; // get
    uint32_t counter = 0;
    uint32_t RTICLK = ( configCPU_CLOCK_HZ );

    if(argc > 1)
    {
        op = argv[1][0];
    }

    switch(op)
    {
    case 'i': //Watchdog init
        if(argc > 2)
        {
            counter = strtoul(argv[2], 0, 10); // in second
        }
        else
        {
            counter = 16;
        }
        counter = (((counter * RTICLK) >> 13) - 1);
        if(counter > 4095)
        {
            printk("preload counter value (%d) > 4095\n", counter);
            return CMD_FAIL;
        }
        dwdInit(rtiREG1, counter); // WATCHDOG_PRELOAD_VALUE
        dwdCounterEnable(rtiREG1);
        printk("wd Init counter 0x%x\n", counter);
        break;

    case 'r': //reset watchdog counter
        dwdReset(rtiREG1);
        break;

    case 'g': //get watchdog counter
        counter = dwwdGetCurrentDownCounter(rtiREG1);
        printk("counter 0x%x\n", counter);
        break;

    default:
        break;
    }

    return CMD_SUCCESS;
}

int cmd_reset_handle(int argc, char **argv)
{
    systemREG1->SYSECR |= 0x00008000;
    return CMD_FAIL;
}

int cmd_mode_handle(int argc, char **argv)
{
    if(argc < 3)
    {
//        printk(cmd_mode.helpmsg);
        return CMD_FAIL;
    }

    gmode = strtoul(argv[1], 0, 10);
    goutmode = strtoul(argv[2], 0, 10);

    printk("change mode to %d output mode %d\n", gmode, goutmode);

    return CMD_SUCCESS;
}

int cmd_debug_handle(int argc, char **argv)
{
    ftl_init();
    return CMD_FAIL;
}

int cmd_top_handle(int argc, char **argv)
{
    const char * tstate[] = { "Running", "Ready", "Blocked", "Suspended", "Deleted", "Invalid"};
    TaskStatus_t *pxTaskStatusArray[2] = {0};
    UBaseType_t NumTasks, uxArraySize[2], i, j;
    uint32_t ulTotalTime[2], ulTotalTimeDiff, ulRunTimeCounterDiff, ulStatsAsPercentage;
    TickType_t delay = configTICK_RATE_HZ;

    BaseType_t xRunningPrivileged = prvRaisePrivilege();

    TickType_t tick = xTaskGetTickCount();
    size_t freemem = xPortGetFreeHeapSize();
    size_t minmem = xPortGetMinimumEverFreeHeapSize();
    printk("Tick %d\n"
           "Memory Total: %d Free: %d (%2.0f%%) Min: %d\n", tick, configTOTAL_HEAP_SIZE, freemem, (float)freemem*100/(float)configTOTAL_HEAP_SIZE, minmem);

    NumTasks = uxTaskGetNumberOfTasks();
    pxTaskStatusArray[0] = pvPortMalloc( NumTasks * sizeof( TaskStatus_t ) * 2);
    pxTaskStatusArray[1] = pxTaskStatusArray[0] + NumTasks;
    portRESET_PRIVILEGE( xRunningPrivileged );

    if( pxTaskStatusArray[0] == NULL)
        goto out;

    uxArraySize[0] = uxTaskGetSystemState( pxTaskStatusArray[0], NumTasks, &ulTotalTime[0] );
    vTaskDelay(delay);
    uxArraySize[1] = uxTaskGetSystemState( pxTaskStatusArray[1], NumTasks, &ulTotalTime[1] );

    if((NumTasks != uxArraySize[0]) || (NumTasks != uxArraySize[1]))
        goto out;

    ulTotalTimeDiff = (ulTotalTime[1] - ulTotalTime[0]) / 100;

    printk("TaskID TaskName RTCounter Percentage stackBase stackWaterMark CurrentState BasePrio CurrPrio\n");
    /* Create a human readable table from the binary data. */
    for( i = 0; i < NumTasks; i++ )
    {
        //find the same task
        for(j = 0; j < NumTasks; j++)
        {
            if(pxTaskStatusArray[0][i].xTaskNumber == pxTaskStatusArray[1][j].xTaskNumber)
                break;
        }
        if(j >= NumTasks)
            goto out;

        ulRunTimeCounterDiff = pxTaskStatusArray[1][j].ulRunTimeCounter - pxTaskStatusArray[0][i].ulRunTimeCounter;
        ulStatsAsPercentage = ulRunTimeCounterDiff / ulTotalTimeDiff;


        printk("%2d %12s %9u %9u%% %9x %14u %12s %8u %8u\n", pxTaskStatusArray[1][j].xTaskNumber, pxTaskStatusArray[0][i].pcTaskName, ulRunTimeCounterDiff, ulStatsAsPercentage,
               pxTaskStatusArray[1][j].pxStackBase, pxTaskStatusArray[1][j].usStackHighWaterMark, tstate[pxTaskStatusArray[1][j].eCurrentState],
               pxTaskStatusArray[1][j].uxBasePriority, pxTaskStatusArray[1][j].uxCurrentPriority );
    }

out:
    xRunningPrivileged = prvRaisePrivilege();
    if(pxTaskStatusArray[0])
        vPortFree( pxTaskStatusArray[0] );
    portRESET_PRIVILEGE( xRunningPrivileged );
    return CMD_SUCCESS;
}

int cmd_nand_handle(int argc, char **argv)
{
    //printk("cmd_nand_handle");
    uint8_t status = 0;
    uint32_t data_address, flash_address, length;
    nand_device_t *dev;
    char op;
    int i;
    rti_t t1 = {0}, t2 = {0};
    uint32_t td;


    if(strcmp(argv[0], "nand") == 0)
    {
        printk(cmd_nand.helpmsg);
        return CMD_FAIL;
    }

    if(argc < 2)
    {
        printk(cmd_nand0.helpmsg);
        return CMD_FAIL;
    }

    if(argv[0][4] == '0')
        dev = NANDFLASH_Init(0);
    else if(argv[0][4] == '1')
        dev = NANDFLASH_Init(1);
    else
    {
        printk(cmd_nand0.helpmsg);
        return CMD_FAIL;
    }

    op = argv[1][0];

    switch(op)
    {
    case 'E': // erase whole chip
        for(i = 0; i < dev->num_blocks;++i)
        {
            status = NANDFLASH_Erase(dev, dev->block_size * i , dev->block_size);
            if(status == 0)
                ;//printk("erase address 0x%x success.", i);
            else
                printk("erase address 0x%x fail. 0x%x", i, status);
        }
        break;

    case 'e': // erase
        flash_address = strtoul(argv[2], 0, 16);
        length = strtoul(argv[3], 0, 16);
        status = NANDFLASH_Erase(dev, flash_address, length);
        break;

    case 'r': // read
        flash_address = strtoul(argv[2], 0, 16);
        data_address = strtoul(argv[3], 0, 16);
        length = strtoul(argv[4], 0, 16);
        save_rti_time(&t1);
        status = NANDFLASH_Read(dev, flash_address, (uint8_t *)data_address, length);
        save_rti_time(&t2);
        td = diff_rti(&t2, &t1);
        printk("read take %uus\n", rti_to_microsecond(td));
        break;

    case 'w': // programing
        flash_address = strtoul(argv[2], 0, 16);
        data_address = strtoul(argv[3], 0, 16);
        length = strtoul(argv[4], 0, 16);
        save_rti_time(&t1);
        status = NANDFLASH_Write(dev, flash_address, (uint8_t *)data_address, length);
        save_rti_time(&t2);
        td = diff_rti(&t2, &t1);
        printk("write take %uus\n", rti_to_microsecond(td));
        break;

    case 'R': // read spare
        flash_address = strtoul(argv[2], 0, 16);
        data_address = strtoul(argv[3], 0, 16);
        length = strtoul(argv[4], 0, 16);
        status = NANDFLASH_ReadSpare(dev, flash_address, (uint8_t *)data_address, length);
        break;

    case 'W': // write spare
        flash_address = strtoul(argv[2], 0, 16);
        data_address = strtoul(argv[3], 0, 16);
        length = strtoul(argv[4], 0, 16);
        status = NANDFLASH_WriteSpare(dev, flash_address, (uint8_t *)data_address, length);
        break;

    default:
        return CMD_FAIL;
        break;
    }
    if(status != 0)
        printk("fail. 0x%x\n", status);

    return CMD_SUCCESS;
}

int cmd_sddisk_handle(int argc, char **argv)
{
    uint8_t status = 0;
    uint32_t buffer_address, sector;
//    ftl_handle_t * ftl = ftl_init();
//    nand_device_t *dev = ftl->dev;
    char op;
//    FF_Disk_t *pxSDDisk = NULL;
    int i;

    if(argc < 2)
    {
//        printk(cmd_nanddisk.helpmsg);
        return CMD_FAIL;
    }

    op = argv[1][0];

    switch(op)
    {
    case 's'://initialize
        pxSDDisk = FF_SDDiskInit("/sd", 1);
        break;

    case 'm': // nand disk mount
        SD_CARD_FLASH_Init(NULL);
        nand_disk_mount(pxSDDisk);
        FF_RAMDiskShowPartition(pxSDDisk);
        break;

    case 'f': // format nand disk
        pxSDDisk = FF_SDDiskInit(NULL, 0);
        if( pxSDDisk )
            FF_SDDiskFormat(pxSDDisk);
        break;

    case 't': // create test file
        //vCreateAndVerifyExampleFiles("/nand1");
        GenerateTestFile("/sd/test.txt", 8192);
        FF_SDDiskFlush(pxSDDisk);
        break;

    case 'c': // create test file
        //vCreateAndVerifyExampleFiles("/nand1");
        printk("S:%s D:%s\n", argv[2], argv[3]);
        //xCopyFile("/sd/2023_11_10_13-41-56_tk2_10b.bin", "/sd/10b.bin");
        xCopyFile(argv[2], argv[3]);
        FF_SDDiskFlush(pxSDDisk);
        break;

    default:
        return CMD_FAIL;
        break;
    }
    if(status != 0)
        printk("fail. 0x%x\n", status);

    return CMD_SUCCESS;
}

int cmd_date_handle(int argc, char **argv)
{
    FF_TimeStruct_t TimeBuf;
    time_t t;
    uint32_t time;
    t = FreeRTOS_time(0);
    t += 3600*configTIME_TIME_ZONE;
    FreeRTOS_gmtime_r(&t, &TimeBuf);
    printk("System Time get：%4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", TimeBuf.tm_year+1900, TimeBuf.tm_mon+1, TimeBuf.tm_mday, TimeBuf.tm_hour, TimeBuf.tm_min, TimeBuf.tm_sec, configTIME_TIME_ZONE);
    return CMD_SUCCESS;
}

int cmd_esm_handle(int argc, char **argv)
{
    // 0: ERROR STATE, PIN HIGH
    // 1: Normal STATE PIN LOW
    uint32 errorpin;
    errorpin = esmError();
    printk("ERROR PIN: %d (%s)\n", errorpin, errorpin ? "HIGH": "LOW");

    char op;
    uint32_t channel;
    uint64_t mask;

    if(argc < 2)
    {
        printk(cmd_esm.helpmsg);
        return CMD_FAIL;
    }

    op = argv[1][0];

    switch(op)
    {
    case 'n':
        esmActivateNormalOperation();
        break;

    case 'r':
        esmTriggerErrorPinReset();
        break;

    case 'f':
        //Force error state
        esmREG->EKR = 0xAU;
        break;

    case 'E':
        //group 2 only, enable from VIM module, test no effect
        vimEnableInterrupt(0, SYS_FIQ); //vim channel 0: ESM HIGH
        break;

    case 'D':
        //group 2 only, disable from VIM module, test no effect
        vimDisableInterrupt(0); //vim channel 0: ESM HIGH
        break;

    default:
        break;
    }

    if(argc < 3)
    {
        printk(cmd_esm.helpmsg);
        return CMD_FAIL;
    }
    channel = strtoul(argv[2], 0, 10);

    switch(op)
    {
    case 'o': //enable group 1 channel X control ERROR PIN
        // BUG shift more than 32 bit will fail
        if(channel < 32)
        {
            mask = 0x1 << channel;
            esmEnableError(mask);
        }
        else if(channel < 64)
        {
            mask = 0x100000000;
            mask = mask << (channel - 32);
            esmEnableError(mask);
        }
        else if(channel < 96)
        {
            mask = 0x1 << (channel - 64);
            esmEnableErrorUpper(mask);
        }
        else if(channel == 999)
        {
            esmEnableError(0xFFFFFFFFFFFFFFFF);
            esmEnableErrorUpper(0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            printk(cmd_esm.helpmsg);
            return CMD_FAIL;
        }
        break;

    case 'x': //disable group 1 channel X control ERROR PIN
        if(channel < 32)
        {
            mask = 0x1 << channel;
            esmDisableError(mask);
        }
        else if(channel < 64)
        {
            mask = 0x100000000;
            mask = mask << (channel - 32);
            esmDisableError(mask);
        }
        else if(channel < 96)
        {
            mask = 0x1 << (channel - 64);
            esmDisableErrorUpper(mask);
        }
        else if(channel == 999)
        {
            esmDisableError(0xFFFFFFFFFFFFFFFF);
            esmDisableErrorUpper(0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            printk(cmd_esm.helpmsg);
            return CMD_FAIL;
        }
        break;

    case 'e':
        //group 1 only
        if(channel < 32)
        {
            mask = 0x1 << channel;
            esmEnableInterrupt(mask);
        }
        else if(channel < 64)
        {
            mask = 0x100000000;
            mask = mask << (channel - 32);
            esmEnableInterrupt(mask);
        }
        else if(channel < 96)
        {
            mask = 0x1 << (channel - 64);
            esmEnableInterruptUpper(mask);
        }
        else if(channel == 999)
        {
            esmEnableInterrupt(0xFFFFFFFFFFFFFFFF);
            esmEnableInterruptUpper(0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            printk(cmd_esm.helpmsg);
            return CMD_FAIL;
        }
        break;

    case 'd':
        //group 1 only
        if(channel < 32)
        {
            mask = 0x1 << channel;
            esmDisableInterrupt(mask);
        }
        else if(channel < 64)
        {
            mask = 0x100000000;
            mask = mask << (channel - 32);
            esmDisableInterrupt(mask);
        }
        else if(channel < 96)
        {
            mask = 0x1 << (channel - 64);
            esmDisableInterruptUpper(mask);
        }
        else if(channel == 999)
        {
            esmDisableInterrupt(0xFFFFFFFFFFFFFFFF);
            esmDisableInterruptUpper(0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            printk(cmd_esm.helpmsg);
            return CMD_FAIL;
        }
        break;

    default:
        return CMD_FAIL;
        break;
    }

    return CMD_SUCCESS;
}

int cmd_mrc_handle(int argc, char **argv)
{
    const uint32_t Opcode_1 = 0, CRn = 0, CRm = 0, Opcode_2 = 0;
    uint32_t data = 0;
    data = __MRC(15, Opcode_1, CRn, CRm, Opcode_2);
    //__MCR(15, Opcode_1, data, CRn, CRm, Opcode_2);
    printk("%x\n", data);

    return CMD_SUCCESS;
}

int cmd_adc_handle(int argc, char **argv)
{
    char op = 'p'; // print
    TickType_t timeout = 1000;

    if(argc > 1)
    {
        op = argv[1][0];
    }

    switch(op)
    {
    case 'e'://enable
        adc_enable = 1;
        break;

    case 'd'://disable
        adc_enable = 0;
        break;

    case 'p': //print
    default:
        adc_print();
        break;
    }
    return CMD_SUCCESS;
}

int cmd_st_handle(int argc, char **argv)
{
    //send trigger to star tracker
    char c = 'T';
    char r;
    //sciPollTx(STARTRACKER_PORT, &c, 1);

    // check reply
    //sciPollRx(STARTRACKER_PORT, &r, 1);
    printk("got reply %c\n", r);

    return CMD_SUCCESS;
}

int cmd_help_handle(int argc, char **argv)
{
    cmd_t *c;
    if(argc < 2)
    {
        c = first_cmd;
        while(c != 0)
        {
            printk(c->cmd);
            printk("\n");
            printk(c->helpmsg);
            printk("\n");
            c = c->next;
        }
    }
    else
    {
        c = find_cmd(argv[1]);
        if(c != 0)
        {
            printk(c->cmd);
            printk("\n");
            printk(c->helpmsg);
            printk("\n");
        }
        else
        {
            printk("command %s not found\n", argv[1]);
        }
    }
    return CMD_SUCCESS;
}

//////////////////VFS command
int cmd_cat_handle(int argc, char **argv)
{
    static FF_FILE *pxFile = NULL;
    int iChar;
    size_t xByte;
    size_t bufferlen = 511;
    char pcWriteBuffer[512];

    if(argc < 2)
    {
        printk(cmd_cat.helpmsg);
        return CMD_FAIL;
    }

    pxFile = ff_fopen( argv[1], "r" );

    if( pxFile == NULL )
    {
        printk("File %s open fail.\n", argv[1]);
        return CMD_FAIL;
    }

    while(pxFile != NULL)
    {
        /* Read the next chunk of data from the file. */
        for( xByte = 0; xByte < bufferlen; xByte++ )
        {
            iChar = ff_fgetc( pxFile );

            if( iChar == -1 )
            {
                /* No more characters to return. */
                ff_fclose( pxFile );
                pxFile = NULL;
                break;
            }
            else
            {
                pcWriteBuffer[ xByte ] = ( char ) iChar;
            }
        }
        if(xByte > 0)
        {
            pcWriteBuffer[xByte] = 0;
            printk_str(pcWriteBuffer, xByte - 1);
        }
    }

    return CMD_SUCCESS;
}

int cmd_cd_handle(int argc, char **argv)
{
    int iReturned;

    if(argc < 2)
    {
        printk(cmd_cd.helpmsg);
        return CMD_FAIL;
    }

    /* Attempt to move to the requested directory. */
    iReturned = ff_chdir( argv[1] );

    if( iReturned != FF_ERR_NONE )
    {
        printk("change directory fail.\n");
        return CMD_FAIL;
    }

    return CMD_SUCCESS;
}

int cmd_ls_handle(int argc, char **argv)
{
    FF_FindData_t pxFindStruct;
    int iReturned = FF_ERR_NONE;
    char pcWriteBuffer[512];
    memset( &pxFindStruct, 0x00, sizeof( FF_FindData_t ) );

    if(argc > 1)
        iReturned = ff_findfirst( argv[1], &pxFindStruct );
    else
        iReturned = ff_findfirst( "", &pxFindStruct );

    while( iReturned == FF_ERR_NONE )
    {
        prvCreateFileInfoString( pcWriteBuffer, &pxFindStruct );
        printk(pcWriteBuffer);

        iReturned = ff_findnext( &pxFindStruct );
    }

    return CMD_SUCCESS;
}

int cmd_rmdir_handle(int argc, char **argv)
{
    int iReturned;

    if(argc < 2)
    {
        printk(cmd_rmdir.helpmsg);
        return CMD_FAIL;
    }

    /* Attempt to delete the directory. */
    iReturned = ff_rmdir( argv[1] );

    if( iReturned == FF_ERR_NONE )
    {
        printk("%s was deleted\n", argv[1] );
    }
    else
    {
        printk( "Error.  %s was not deleted\n", argv[1] );
        return CMD_FAIL;
    }

    return CMD_SUCCESS;
}

int cmd_mkdir_handle(int argc, char **argv)
{
    int iReturned;

    if(argc < 2)
    {
        printk(cmd_mkdir.helpmsg);
        return CMD_FAIL;
    }

    /* Attempt to delete the directory. */
    iReturned = ff_mkdir( argv[1] );

    if( iReturned == FF_ERR_NONE )
    {
        printk("%s was created\n", argv[1] );
    }
    else
    {
        printk( "Error.  %s was not created\n", argv[1] );
        return CMD_FAIL;
    }

    return CMD_SUCCESS;
}

int cmd_rm_handle(int argc, char **argv )
{
    int iReturned;

    if(argc < 2)
    {
        printk(cmd_rm.helpmsg);
        return CMD_FAIL;
    }

    /* Attempt to delete the file. */
    iReturned = ff_remove( argv[1] );

    if( iReturned == FF_ERR_NONE )
    {
        printk( "%s was deleted\n", argv[1] );
    }
    else
    {
        printk( "Error.  %s was not deleted\n", argv[1] );
        return CMD_FAIL;
    }

    return CMD_SUCCESS;
}

int cmd_cp_handle(int argc, char **argv)
{
    char *pcSourceFile;
    const char *pcDestinationFile;
    long lSourceLength, lDestinationLength = 0;
    FF_Stat_t xStat;

    if(argc < 3)
    {
        printk(cmd_cp.helpmsg);
        return CMD_FAIL;
    }

    pcSourceFile = argv[1];
    pcDestinationFile = argv[2];

    /* See if the source file exists, obtain its length if it does. */
    if( ff_stat( pcSourceFile, &xStat ) == FF_ERR_NONE )
    {
        lSourceLength = xStat.st_size;
    }
    else
    {
        lSourceLength = 0;
    }

    if( lSourceLength == 0 )
    {
        printk("Source file does not exist\n");
    }
    else
    {
        /* See if the destination file exists. */
        if( ff_stat( pcDestinationFile, &xStat ) == FF_ERR_NONE )
        {
            lDestinationLength = xStat.st_size;
        }
        else
        {
            lDestinationLength = 0;
        }

        if( xStat.st_mode == FF_IFDIR )
        {
            printk( "Error: Destination is a directory not a file" );

            /* Set lDestinationLength to a non-zero value just to prevent an attempt to copy the file. */
            lDestinationLength = 1;
        }
        else if( lDestinationLength != 0 )
        {

            printk( "Error: Destination file already exists" );
        }
    }

    /* Continue only if the source file exists and the destination file does not exist. */
    if( ( lSourceLength != 0 ) && ( lDestinationLength == 0 ) )
    {
        if( prvPerformCopy( pcSourceFile, lSourceLength, pcDestinationFile) == pdPASS )
        {
            printk("Copy made\n");
            return CMD_SUCCESS;
        }
        else
        {
            printk("Error during copy\n");
        }
    }
    return CMD_FAIL;
}

int cmd_pwd_handle(int argc, char **argv)
{
    /* Copy the current working directory into the output buffer. */
    char buffer[128];
    ff_getcwd( buffer, 128 );
    printk(buffer);
    printk("\n");
    return CMD_SUCCESS;
}
int cmd_console_handle(int argc, char **argv)
{
    printk("cmd_print_handle\n");
    return CMD_SUCCESS;
}


int cmd_SD_write_handle(int argc, char **argv)
{

    unsigned int *buf;
    unsigned long sector;
    unsigned int count;

    if(argc < 4)
    {
        printk(cmd_SD_write.helpmsg);
        return CMD_FAIL;
    }

    buf = (unsigned int *)strtoul(argv[1], 0, 16);
    sector = (unsigned long)strtoul(argv[2], 0, 32);
    count = (unsigned int)strtoul(argv[3], 0, 32);

    printk("cmd_SD_write_handle buf(0x%08x) sector(%d) count(%d)\n",buf,sector,count);
    return lisco_disk_write(buf,sector,count);

    //return lisco_disk_write (buf,sector,count);
}

int cmd_SD_read_handle(int argc, char **argv)
{
    unsigned int *buf;
    unsigned long sector;
    unsigned int count;

    if(argc < 4)
    {

        printk(cmd_SD_read.helpmsg);
        return CMD_FAIL;
    }

    buf = (unsigned int *)strtoul(argv[1], 0, 16);
    sector = (unsigned long)strtoul(argv[2], 0, 32);
    count = (unsigned int)strtoul(argv[3], 0, 32);

    printk("cmd_SD_read_handle buf(0x%08x) sector(%d) count(%d)\n",buf,sector,count);
    //return lisco_disk_read(0x6414419c,0,2);
    return lisco_disk_read (buf,sector,count);

}
/*--------RTC Command--------*/
int cmd_RTCdate_handle(int argc, char **argv)
{
    FF_TimeStruct_t TimeBuf;
    getRTC(&TimeBuf);
    printk("RTC Time get：%4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", TimeBuf.tm_year+1900, TimeBuf.tm_mon+1, TimeBuf.tm_mday, TimeBuf.tm_hour, TimeBuf.tm_min, TimeBuf.tm_sec, configTIME_TIME_ZONE);
    return CMD_SUCCESS;
}

int cmd_RTCtest_handle(int argc, char **argv)
{
    if(argc < 2)
    {
//        printk(cmd_RTCtest.helpmsg);
        return CMD_FAIL;
    }
    int tmp = atoi(argv[1]);
    FF_TimeStruct_t TimeBuf;
    //printk("tmp=%d",tmp);
    time_t t;
    while(1){
        getRTC(&TimeBuf);
        t = FreeRTOS_mktime(&TimeBuf);
        //printk("t:%d\n",t);
        if(t>=(tmp+3600*8-600))
            printk("RTC Time get：%4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", TimeBuf.tm_year+1900, TimeBuf.tm_mon+1, TimeBuf.tm_mday, TimeBuf.tm_hour, TimeBuf.tm_min, TimeBuf.tm_sec, configTIME_TIME_ZONE);
        if(t>=(tmp+3600*8)){
            break;
        }


        vTaskDelay(1000);
    }
    printk("complete\n");
    return CMD_SUCCESS;
}

int cmd_RTC_timeset_handle(int argc, char **argv)
{
    if(argc != 7)
    {
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    //check argv is digit
    int i,j;
    for(i=1;i<7;i++){
        int jj=strlen(argv[i]);
        for(j=0;j<jj;j++){
            if(argv[i][j]<'0'||argv[i][j]>'9'){
                printk(cmd_RTC_timeset.helpmsg);
                return CMD_FAIL;
            }
        }
    }
    FF_TimeStruct_t *pTimeBuf=TimeBuff;
    memset(pTimeBuf,0,1);
    int mon =  atoi(argv[2])-1,year = atoi(argv[1])-1900,mday= atoi(argv[3]),sec= atoi(argv[6]),min= atoi(argv[5]),hour= atoi(argv[4]);
//    printk("Time get：%04d-%02d-%02d %02d:%02d:%02d\n", atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    if(year<123||year>150){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_year = year;
    if(mon<0||mon>11){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_mon = mon;
    if(mday<1||mday>31){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_mday = mday;
    if(hour<0||hour>23){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_hour = hour;
    if(min<0||min>59){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_min = min;
    if(sec<0||sec>59){
        printk(cmd_RTC_timeset.helpmsg);
        return CMD_FAIL;
    }
    pTimeBuf->tm_sec = sec;
    setRTC(pTimeBuf);

    return CMD_SUCCESS;
}
/*---------------------------*/

/*------RSI2000 Command------*/
int cmd_RSI_Read_handle(int argc, char **argv){
    uint16_t datalen = argc-1;
    int i;
    uint16_t *buffer = (uint16_t*)pvPortMalloc(sizeof(uint16_t)*datalen);
    memset(buffer, 0, sizeof(uint16_t)*datalen);
    for(i=0;i<datalen;i++){
        buffer[i] = atoi(argv[i+1])*4;
    }
    SendPackage(TCTM_CMD_READ, datalen*2, (uint8_t*)buffer,defaultOptions);
    vPortFree(buffer);
    return CMD_SUCCESS;
}

int cmd_RSI_Write_handle(int argc, char **argv){
    uint16_t datalen = argc-1;
    int i;
    unsigned short *buffer = (unsigned short*)pvPortMalloc(sizeof(unsigned short)*datalen/2*6);
    memset(buffer, 0, sizeof(unsigned short)*datalen/2*6);
    for(i=0;i<(datalen/2);i++){
        if(accessDataList[atoi(argv[i*2+1])].type == 0){//int
            buffer[i*3] = atoi(argv[i*2+1])*4;
            buffer[i*3+2] = atoi(argv[i*2+2]);
        }
        if(accessDataList[atoi(argv[i*2+1])].type == 1){//float
            buffer[i*3] = atoi(argv[i*2+1])*4;
            float floatValue = atof(argv[i*2+2]);
            buffer[i*3+1] = *((uint32_t*)&floatValue)>>16;
            buffer[i*3+2] = *((uint32_t*)&floatValue);
        }
    }
    SendPackage(TCTM_CMD_WRITE, datalen/2*6, (uint8_t*)buffer,defaultOptions);
    vPortFree(buffer);
    return CMD_SUCCESS;
}

int cmd_RSI_Info_handle(int argc, char **argv){
    char *flag = argv[1];
    uint32_t value = 0;
    if(argc > 2)
        value = atoi(argv[2]);
    uint8_t imageOperateFlag = 0x00;
    if (strcmp(flag,"index")==0){
       imageOperateFlag = 0;
    }
    else if (strcmp(flag,"new")==0)
       imageOperateFlag = 1;
    else if (strcmp(flag,"old")==0)
       imageOperateFlag = 2;
    else if (strcmp(flag,"all")==0)
       imageOperateFlag = 255;

    uint8_t buffer[CMD_IMAGE_INFO_LEN];
    memset(buffer, 0, sizeof(buffer));

    buffer[0] = imageOperateFlag;
    buffer[1] = (value >> 24) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 8) & 0xFF;
    buffer[4] = value & 0xFF;

    SendPackage(TCTM_CMD_IMAGE_INFO, CMD_IMAGE_INFO_LEN, buffer,defaultOptions);

    return CMD_SUCCESS;
}

int cmd_RSI_Delete_handle(int argc, char **argv){
    char *flag = argv[1];
    uint32_t value = 0;
    if(argc > 2)
        value = atoi(argv[2]);
    uint8_t imageOperateFlag = 0x00;
    if (strcmp(flag,"index")==0){
       imageOperateFlag = 0;
    }
    else if (strcmp(flag,"new")==0)
       imageOperateFlag = 1;
    else if (strcmp(flag,"old")==0)
       imageOperateFlag = 2;
    else if (strcmp(flag,"all")==0)
       imageOperateFlag = 255;

    uint8_t buffer[CMD_IMAGE_INFO_LEN];
    memset(buffer, 0, sizeof(buffer));

    buffer[0] = imageOperateFlag;
    buffer[1] = (value >> 24) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 8) & 0xFF;
    buffer[4] = value & 0xFF;

    SendPackage(TCTM_CMD_IMAGE_DELETE, CMD_IMAGE_DELETE_LEN, buffer, defaultOptions);

    return CMD_SUCCESS;
}

int cmd_RSI_Capture_handle(int argc, char **argv){
    time_t timeNow = FreeRTOS_time(0);
    uint8_t buffer[CMD_CAPTURE_LEN];

    memset(buffer, 0, sizeof(buffer));
    buffer[0] = 0x00;  //Unix Timestamp Address
    buffer[1] = 0x84;  //Unix Timestamp Address
    buffer[2] = (timeNow >> 24) & 0xFF;
    buffer[3] = (timeNow >> 16) & 0xFF;
    buffer[4] = (timeNow >> 8) & 0xFF;
    buffer[5] = timeNow & 0xFF;

    SendPackage(TCTM_CMD_WRITE, CMD_CAPTURE_LEN, buffer, (TCTM_SendPackageOptions){1, 500});
    SendPackage(TCTM_CMD_IMAGE_CAPTURE, 0, NULL, (TCTM_SendPackageOptions){1, 10000});

    uint16_t imageAmountCode = 0x80;
    SendPackage(TCTM_CMD_READ, sizeof(imageAmountCode), (uint8_t*)&imageAmountCode, defaultOptions);

    return CMD_SUCCESS;
}

int cmd_RSI_Bad_handle(int argc, char **argv){

    uint16_t index = atoi(argv[1]);
    uint32_t value = atoi(argv[2]);

    uint8_t buffer[CMD_BAD_IMAGE_SETTING_LEN];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = (index >> 8) & 0xFF;
    buffer[1] = index & 0xFF;
    buffer[2] = (value >> 24) & 0xFF;
    buffer[3] = (value >> 16) & 0xFF;
    buffer[4] = (value >> 8) & 0xFF;
    buffer[5] = value & 0xFF;

    SendPackage(TCTM_CMD_BAD_IMAGE_SETTING, CMD_BAD_IMAGE_SETTING_LEN, buffer, defaultOptions);
    return CMD_SUCCESS;
}

int cmd_RSI_Reset_handle(int argc, char **argv){
    SendPackage(TCTM_CMD_RESET, 0, NULL,(TCTM_SendPackageOptions){1,30});
    return CMD_SUCCESS;
}

int cmd_RSI_Prepare_handle(int argc, char **argv){
    uint16_t index = atoi(argv[1]);
    uint8_t buffer[CMD_PREPARE_LEN];

    buffer[0] = (index >> 8) & 0xFF;
    buffer[1] = index & 0xFF;

    printk("Prepare Image File\n");
    while(1){
        SendPackage(TCTM_CMD_IMAGE_PREPARE, CMD_PREPARE_LEN, buffer,(TCTM_SendPackageOptions){1,30});
        if(GetOperateResult() == TCTM_SUCCESS){
            printk_ni("\nPrepare Finished\n");
            break;
        }
        else if( GetOperateResult() == TCTM_DOWNLOAD_DATA_PREPAR){
            printk_ni(".");
            vTaskDelay(500);
        }
        else
            printk_ni("\nPrepare Image Error:%d\n",GetOperateResult());
    }
    return CMD_SUCCESS;
}

int cmd_RSI_Downlode_handle(int argc, char **argv){
//    TCTM_CMD_Download(argv[1], atoi(argv[2]));
    init_local_storage();
    char *flag = argv[1];
    uint8_t imageOperateFlag;
    if (strcmp(flag,"index")==0){
       imageOperateFlag = 0;
    }
    else if (strcmp(flag,"new")==0)
       imageOperateFlag = 1;
    else if (strcmp(flag,"old")==0)
       imageOperateFlag = 2;
    cmd_downloaddata.type = imageOperateFlag;
    cmd_downloaddata.value = atoi(argv[2]);
    cmd_downloaddata.flag = 1;
    TCTM_CMD_Download(cmd_downloaddata.type, cmd_downloaddata.value);
    return CMD_SUCCESS;
}
/*---------------------------*/

/*------Storage Command------*/
int cmd_storage_handle(int argc, char **argv)
{
    int index = 0;
    char op;
    FF_TimeStruct_t tempTimeBuff;
    if(argc < 2)
    {
        printk(cmd_storage.helpmsg);
        return CMD_FAIL;
    }

    op = argv[1][0];
    init_local_storage();

    switch(op)
    {
    case 's'://initialize
        update_image_info_table();
        break;

    case 'i': // read image info
        index = atoi(argv[2]);
        transmit_partial_label(index);
        memcpy(&imageinfo, &imageinfo_buffer[0], 17);
        if(imageinfo.showflag == 0){
            printk_ni("Image[%02d] no exist \n", index);
            break;
        }
        printk_ni(" - Index: %u,  Mode = %u, Flag = %u, Size = %u,  Lock = %u, \n"
                              , index, imageinfo.mode,
                              imageinfo.flag,
                              imageinfo.size,
                              imageinfo.lockflag);
        FreeRTOS_gmtime_r((const time_t*)&imageinfo.timestamp, &tempTimeBuff);
        printk_ni("Time：%4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", tempTimeBuff.tm_year+1900, tempTimeBuff.tm_mon+1, tempTimeBuff.tm_mday, tempTimeBuff.tm_hour+8, tempTimeBuff.tm_min, tempTimeBuff.tm_sec, configTIME_TIME_ZONE);

        break;
    case 'l': // lock image
        index = atoi(argv[2]);
        lock_storage_image(index);
        break;

    case 'd': // delete image
        index = atoi(argv[2]);
        delete_storage_image(index);
        break;

    case 'c': // copy image to sd_card
        index = atoi(argv[2]);
        save_image_to_sd(index);
        break;

    default:
        return CMD_FAIL;
    }

    return CMD_SUCCESS;
}
/*---------------------------*/

/*------OBC100 API Command------*/
int cmd_buffer_write_handle(int argc, char **argv){
    uint8 *address;
    uint32 value,length,sum;
    char op;
    int i;
    if(argc < 3)
    {
        printk(cmd_buffer_write_HANDLE.helpmsg);
        return CMD_FAIL;
    }
    op = argv[1][0];
    address = &api_tx_buffer[0];
    if(op == 'm'){
        for(i = 2; i < argc; i++){
            value = strtoul(argv[i], 0, 16);
            memcpy(address, &value, 4);
            address = address + 4;
        }
        printk("E_SUCCESS\n");
    }
    else if(op == 'r'){
        length = atoi(argv[2]);
        sum = 0;
        for(i=0; i<length; i++){
            api_tx_buffer[i] = i % 256;
            sum += api_tx_buffer[i];
        }
        printk_ni("check sum = %x\n",sum);
    }
    else{
        printk(cmd_buffer_write_HANDLE.helpmsg);
        return CMD_FAIL;
    }
    return CMD_SUCCESS;
}

int cmd_buffer_read_handle(int argc, char **argv){
    uint8 *address;
    uint16 length,i,sum;
    uint32 value;
    char op;
    if(argc < 3)
    {
        printk(cmd_buffer_read_HANDLE.helpmsg);
        return CMD_FAIL;
    }
    op = argv[1][0];
    length = atoi(argv[2]);
    address = &api_rx_buffer[0];
    if(op == 'm'){
        for(i = 0; i < length/4; i++){
            memcpy(&value, address, 4);
            printk_ni("address[%08x] : [%08x]\n", address, value);
            address = address + 4;
        }
    }
    else if(op == 'r'){
        for(i = 0; i < length; i++)
            sum += api_rx_buffer[i];
        printk_ni("check sum = %x\n",sum);
    }
    else{
        printk(cmd_buffer_read_HANDLE.helpmsg);
        return CMD_FAIL;
    }
    return CMD_SUCCESS;
}

/*------UART------*/

int cmd_uart_block_handle(int argc, char **argv){
    char op,op2;
    int error,index;
    if(argc < 4)
    {
        printk(cmd_uart_block.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];
    op2 = argv[3][0];
    if(index == 0){
        printk("UART COM[1] is console, do not use.\n");
        return CMD_FAIL;
    }
    if(op == 'b'){
        if(op2 == 't'){
            error = uart_ioctl(index, UART_CTL_BLOCK_MODE, BLOCKING, IO_SCI_TX_INT);
        }
        else if(op2 == 'r'){
            error = uart_ioctl(index, UART_CTL_BLOCK_MODE, BLOCKING, IO_SCI_RX_INT);
        }
    }
    else{
        if(op2 == 't'){
            error = uart_ioctl(index, UART_CTL_BLOCK_MODE, NONE_BLOCKING, IO_SCI_TX_INT);
        }
        else if(op2 == 'r'){
            error = uart_ioctl(index, UART_CTL_BLOCK_MODE, NONE_BLOCKING, IO_SCI_RX_INT);
        }
    }
    if(error != E_SUCCESS){
        printk("invalid Input\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_uart_speed_handle(int argc, char **argv){
    uint32 speed;
    int error,index;
    if(argc < 3)
    {
        printk(cmd_uart_speed.helpmsg);
        return CMD_FAIL;
    }

    index = atoi(argv[1]) - 1;
    speed = atoi(argv[2]);

    if(index == 0){
        printk("UART COM[1] is console, do not use.\n");
        return CMD_FAIL;
    }
    error = uart_ioctl(index, UART_CTL_BAUDRATE, speed, NULL);

    if(error != E_SUCCESS){
        printk("invalid Input\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_uart_read_handle(int argc, char **argv){
    int error,index;
    if(argc < 3)
    {
        printk(cmd_uart_read.helpmsg);
        return CMD_FAIL;
    }
    uint16 length;
    index = atoi(argv[1]) - 1;
    length = atoi(argv[2]);
    if(index == 0){
        printk("UART COM[1] is console, do not use.\n");
        return CMD_FAIL;
    }

    if(uart_rx_Ready(index))
        error = uart_read(index, length, &api_rx_buffer[0]);
    else{
        printk_ni("UART in busy!\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    if(error == E_SUCCESS)
        return CMD_SUCCESS;

    return CMD_FAIL;
}
int cmd_uart_write_handle(int argc, char **argv){
    int error,index;
    if(argc < 3)
    {
        printk(cmd_uart_write.helpmsg);
        return CMD_FAIL;
    }
    uint16 length;
    index = atoi(argv[1]) - 1;
    length = atoi(argv[2]);
    if(index == 0){
        printk("UART COM[1] is console, do not use.\n");
        return CMD_FAIL;
    }

    if(uart_tx_complete(index))
        error = uart_write(index, length, &api_tx_buffer[0]);
    else{
        printk_ni("UART in busy!\n");
        return CMD_FAIL;
    }
//    xioresponse(error);
    if(error == E_SUCCESS)
        return CMD_SUCCESS;

    return CMD_FAIL;
}

/*------I2C Command------*/

int cmd_i2c_speed_handle(int argc, char **argv){
    uint32 speed;
    int error;
    if(argc < 2)
    {
        printk(cmd_i2c_speed.helpmsg);
        return CMD_FAIL;
    }
    speed = atoi(argv[1]);
    error = i2c_ioctl(I2C_COM1, I2C_CTL_BAUDRATE, speed, NULL);

    if(error != E_SUCCESS){
        printk("invalid Input\n");
        return CMD_FAIL;
    }

    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_i2c_read_handle(int argc, char **argv){
    int error;
    uint32 slave_add, read_add;
    if(argc < 4)
    {
        printk(cmd_i2c_read.helpmsg);
        return CMD_FAIL;
    }
    uint16 length;
    length = atoi(argv[3]) - 1;
    slave_add = strtoul(argv[1], 0, 16);
    read_add = strtoul(argv[2], 0, 16);

    error = i2c_read(I2C_COM1, slave_add, read_add, length, &api_rx_buffer[0]);

    if(error != E_SUCCESS)
        return CMD_FAIL;

    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_i2c_write_handle(int argc, char **argv){
    int error;
    uint32 slave_add, write_add;
    if(argc < 4)
    {
        printk(cmd_i2c_write.helpmsg);
        return CMD_FAIL;
    }
    uint16 length;
    length = atoi(argv[3]);
    slave_add = strtoul(argv[1], 0, 16);
    write_add = strtoul(argv[2], 0, 16);

    error = i2c_write(I2C_COM1, slave_add, write_add, length, &api_tx_buffer[0]);

    if(error != E_SUCCESS)
        return CMD_FAIL;

    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_i2c_transfer_handle(int argc, char **argv){
    int error;
    uint32 slave_add, write_add;
    if(argc < 4)
    {
        printk(cmd_i2c_transfer.helpmsg);
        return CMD_FAIL;
    }
    uint32 length_tx, length_rx;
    slave_add = strtoul(argv[1], 0, 16);
    length_tx = strtoul(argv[2], 0, 16);
    length_rx = strtoul(argv[2], 0, 16);


    error = i2c_transfer(I2C_COM1, slave_add, length_tx, length_rx, api_tx_buffer, api_rx_buffer);

    if(error != E_SUCCESS)
        return CMD_FAIL;

    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_i2c_md_handle(int argc, char **argv) {
    if (argc < 4) {
        printk(cmd_i2c_md.helpmsg);
        return CMD_FAIL;
    }
    int i;
    uint32_t slave_addr = strtoul(argv[1], NULL, 16);
    uint32_t mem_addr   = strtoul(argv[2], NULL, 16);
    uint16_t length     = strtoul(argv[3], NULL, 0);

    if (length > sizeof(api_rx_buffer)) {
        printk("Requested length too long\n");
        return CMD_FAIL;
    }

    int error = i2c_read(I2C_COM1, slave_addr, mem_addr, length, api_rx_buffer);
    if (error != E_SUCCESS) {
        printk("I2C read failed\n");
        return CMD_FAIL;
    }

    // display memory（16 bytes per line）
    printk("I2C Read: slave=0x%02X, mem_addr=0x%02X, len=%d\n", slave_addr, mem_addr, length);
    for (i = 0; i < length; i++) {
        if (i % 16 == 0)
            printk("\n0x%04X: ", mem_addr + i);
        printk("%02X ", api_rx_buffer[i]);
    }
    printk("\n");

    return CMD_SUCCESS;
}

int cmd_i2c_mm_handle(int argc, char **argv) {
    if (argc < 4) {
        printk(cmd_i2c_mm.helpmsg);
        return CMD_FAIL;
    }

    uint32_t slave_addr = strtoul(argv[1], NULL, 16);
    uint32_t mem_addr   = strtoul(argv[2], NULL, 16);
    uint8_t  value      = (uint8_t)strtoul(argv[3], NULL, 16);

    api_tx_buffer[0] = value;
    int error = i2c_write(I2C_COM1, slave_addr, mem_addr, 1, api_tx_buffer);
    if (error != E_SUCCESS) {
        printk("I2C write failed\n");
        return CMD_FAIL;
    }

    printk("Wrote 0x%02X to slave=0x%02X at mem_addr=0x%02X\n", value, slave_addr, mem_addr);
    return CMD_SUCCESS;
}

int cmd_i2c_mw_handle(int argc, char **argv) {
    if (argc < 5) {
        printk(cmd_i2c_mw.helpmsg);
        return CMD_FAIL;
    }
    int i;
    uint32_t slave_addr = strtoul(argv[1], NULL, 16);
    uint32_t mem_addr   = strtoul(argv[2], NULL, 16);
    uint16_t length     = strtoul(argv[3], NULL, 0);

    if (argc < 4 + length) {
        printk("Error: not enough data bytes provided\n");
        return CMD_FAIL;
    }

    if (length > sizeof(api_tx_buffer)) {
        printk("Write length too large\n");
        return CMD_FAIL;
    }

    for (i = 0; i < length; ++i) {
        api_tx_buffer[i] = (uint8_t)strtoul(argv[4 + i], NULL, 16);
    }

    int error = i2c_write(I2C_COM1, slave_addr, mem_addr, length, api_tx_buffer);
    if (error != E_SUCCESS) {
        printk("I2C write failed\n");
        return CMD_FAIL;
    }

    printk("Wrote %d bytes to slave=0x%02X at mem_addr=0x%02X\n", length, slave_addr, mem_addr);
    return CMD_SUCCESS;
}
/*------SPI Command------*/
int cmd_spi_info_handle(int argc, char **argv){
    spi_info();
    return CMD_SUCCESS;
}
int cmd_spi_block_handle(int argc, char **argv){
    char op,op2;
    int error,index;
    if(argc < 3)
    {
        printk(cmd_spi_block.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];

    if(op == 'b'){
        error = spi_ioctl(index, SPI_CTL_BLOCK_MODE, BLOCKING, IO_SPI_RX_INT);
    }
    else if(op == 'n'){
        error = spi_ioctl(index, SPI_CTL_BLOCK_MODE, NONE_BLOCKING, IO_SPI_RX_INT);
    }
    else{
        printk("invalid Input\n");
        return CMD_FAIL;
    }
    if(error != E_SUCCESS){
        printk("invalid Input\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_spi_speed_handle(int argc, char **argv){
    uint32 speed,index;
    int error;
    if(argc < 3)
    {
        printk(cmd_spi_speed.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    speed = atoi(argv[2]);
    error = spi_ioctl(index, SPI_CTL_BAUDRATE, speed, NULL);

    if(error != E_SUCCESS){
        printk("invalid Input\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_spi_write_handle(int argc, char **argv){
    int length, index, error;
    if(argc < 3){
        printk(cmd_spi_write.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    length = atoi(argv[2]);
    spi_transfer(index, length, api_tx_buffer, api_rx_buffer2);

    if(error == E_INVALID_INPUT){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    else if(error == E_BUSY){
        printk("E_BUSY\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_spi_read_handle(int argc, char **argv){
    int length, index, error;
//    spi_ioctl(SPI_DEVICE2, SPI_CTL_BLOCK_MODE, NONE_BLOCKING, IO_SPI_RX_INT);
    if(argc < 3){
        printk(cmd_spi_read.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    length = atoi(argv[2]);
    error = spi_transfer(index, length, api_tx_buffer2, api_rx_buffer);
    if(error == E_INVALID_INPUT){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    else if(error == E_BUSY){
        printk("E_BUSY\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}
/*------GPIO Command------*/
int cmd_gpio_info_handle(int argc, char **argv){
    gpio_info();
    return CMD_SUCCESS;
}
int cmd_gpio_output_drain_handle(int argc, char **argv){
    int index, error, mode;
    char op;
    if(argc < 3){
        printk(cmd_gpio_outputdrain.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];
    if(op == 'o')
        mode = IO_GPIO_OUTPUT_DRAIN_OPEN;
    else if(op == 'c')
        mode = IO_GPIO_OUTPUT_DRAIN_CLOSE;
    else{
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }

    error = gpio_ioctl(index, GPIO_CTL_OUTPUT_DRAIN, mode, NULL);
    if(error != E_SUCCESS){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_gpio_input_pull_handle(int argc, char **argv){
    int index, error, mode;
    char op;
    if(argc < 3){
        printk(cmd_gpio_inputpull.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];

    if(op == 'o')
        mode = IO_GPIO_INPUT_PULL_OPEN;
    else if(op == 'c')
        mode = IO_GPIO_INPUT_PULL_CLOSE;
    else{
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    error = gpio_ioctl(index, GPIO_CTL_INPUT_PULL, mode, NULL);
    if(error != E_SUCCESS){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_gpio_pull_dir_handle(int argc, char **argv){
    int index, error, mode;
    char op;
    if(argc < 3){
        printk(cmd_gpio_pulldir.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];

    if(op == 'u')
        mode = IO_GPIO_PULL_DIR_UP;
    else if(op == 'd')
        mode = IO_GPIO_PULL_DIR_DOWN;
    else{
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    error = gpio_ioctl(index, GPIO_CTL_PULL_MODE, mode, NULL);
    if(error == E_NOT_SUPPORT){
        printk("E_NOT_SUPPORT\n");
        return CMD_FAIL;
    }
    else if(error == E_INVALID_INPUT){
        printk(" E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    xioresponse(error);
    return CMD_SUCCESS;
}
int cmd_gpio_read_handle(int argc, char **argv){
    int index, result;
    if(argc < 2){
        printk(cmd_gpio_read.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    result = gpio_input(index);
    if(result == E_INVALID_INPUT){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    else if(result == E_NOT_SUPPORT){
        printk("E_NOT_SUPPORT\n");
        return CMD_FAIL;
    }
    if(result == 1)
        printk("GPIO[%d] get : [High]\n", index + 1);
    else
        printk("GPIO[%d] get : [Low]\n", index + 1);
    return CMD_SUCCESS;
}
int cmd_gpio_write_handle(int argc, char **argv){
    int index, error, mode;
    char op;
    if(argc < 3){
        printk(cmd_gpio_write.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    op = argv[2][0];

    if(op == 'h')
        mode = IO_GPIO_HIGH;
    else if(op == 'l')
        mode = IO_GPIO_LOW;
    else{
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    error = gpio_output(index, mode);
    if(error == E_INVALID_INPUT){
        printk("E_INVALID_INPUT\n");
        return CMD_FAIL;
    }
    else if(error == E_NOT_SUPPORT){
        printk("E_NOT_SUPPORT\n");
        return CMD_FAIL;
    }
    if(mode == IO_GPIO_HIGH)
        printk("GPIO[%d] output : [High]\n", index + 1);
    else
        printk("GPIO[%d] output : [Low]\n", index + 1);
    return CMD_SUCCESS;
}
/*-------------------------*/
/*-------CAN Command-------*/
int cmd_can_info_handle(int argc, char **argv){
    can_info();
    return CMD_SUCCESS;
}
int cmd_can_id_handle(int argc, char **argv){
    uint32 id;
    char dir,ide;
    int index, error;
    if(argc < 5){
        printk(cmd_can_id.helpmsg);
        return CMD_FAIL;
    }

    index = atoi(argv[1]) - 1;
    dir = argv[2][0];
    ide = argv[3][0];
    id  = strtoul(argv[4], 0 , 16);
    if(dir == 't'){
        if(ide == 'e')
            error = can_ioctl(index, CAN_CTL_ID, IO_CAN_TX, ENABLE, id);
        else
            error = can_ioctl(index, CAN_CTL_ID, IO_CAN_TX, DISABLE, id);
    }
    else{
        if(ide == 'e')
            error = can_ioctl(index, CAN_CTL_ID, IO_CAN_RX, ENABLE, id);
        else
            error = can_ioctl(index, CAN_CTL_ID, IO_CAN_RX, DISABLE, id);
    }
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_can_mask_handle(int argc, char **argv){
    uint32 mask;
    char dir;
    int index, error;
    if(argc < 4){
        printk(cmd_can_mask.helpmsg);
        return CMD_FAIL;
    }

    index = atoi(argv[1]) - 1;
    dir = argv[2][0];
    mask  = strtoul(argv[3], 0 , 16);
    if(dir == 't')
        error = can_ioctl(index, CAN_CTL_MASK, IO_CAN_TX, mask, NULL);
    else
        error = can_ioctl(index, CAN_CTL_MASK, IO_CAN_RX, mask, NULL);
    xioresponse(error);

    return CMD_SUCCESS;
}

int cmd_can_speed_handle(int argc, char **argv){
    uint16 speed;
    int error,index;
    if(argc < 3)
    {
        printk(cmd_can_speed.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    speed = atoi(argv[2]);
    error = can_ioctl(index, CAN_CTL_BAUDRATE, speed, NULL, NULL);

    xioresponse(error);

    return CMD_SUCCESS;
}

int cmd_can_read_handle(int argc, char **argv){
    int index, result, datalen, error;
    uint32 msgbox;
    if(argc < 3){
        printk(cmd_can_read.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    datalen = atoi(argv[2]);
    error = can_read(index, (uint32)2, api_rx_buffer, datalen);
    xioresponse(error);
    return CMD_SUCCESS;
}

int cmd_can_write_handle(int argc, char **argv){
    int index, result, datalen, error;
    uint32 msgbox;
    if(argc < 3){
        printk(cmd_can_read.helpmsg);
        return CMD_FAIL;
    }
    index = atoi(argv[1]) - 1;
    datalen = atoi(argv[2]);
    error = can_write(index, (uint32)1, api_tx_buffer, datalen);
    xioresponse(error);
    return CMD_SUCCESS;
}

BaseType_t prvPerformCopy( const char *pcSourceFile, int32_t lSourceFileLength, const char *pcDestinationFile)
{
    int32_t lBytesRead = 0, lBytesToRead, lBytesRemaining;
    FF_FILE *pxSourceFile, *pxDestinationFile;
    BaseType_t xReturn = pdPASS;
    char pxWriteBuffer[128];
    size_t xWriteBufferLen = 128;

    /* NOTE:  Error handling has been omitted for clarity. */

    pxSourceFile = ff_fopen( pcSourceFile, "r" );
    pxDestinationFile = ff_fopen( pcDestinationFile, "a" );

    if( ( pxSourceFile != NULL ) && ( pxDestinationFile != NULL ) )
    {
        while( lBytesRead < lSourceFileLength )
        {
            /* How many bytes are left? */
            lBytesRemaining = lSourceFileLength - lBytesRead;

            /* How many bytes should be read this time around the loop.  Can't
            read more bytes than will fit into the buffer. */
            if( lBytesRemaining > ( long ) xWriteBufferLen )
            {
                lBytesToRead = ( long ) xWriteBufferLen;
            }
            else
            {
                lBytesToRead = lBytesRemaining;
            }

            ff_fread( pxWriteBuffer, lBytesToRead, 1, pxSourceFile );
            ff_fwrite( pxWriteBuffer, lBytesToRead, 1, pxDestinationFile );

            lBytesRead += lBytesToRead;
        }
    }

    if( pxSourceFile != NULL )
    {
        ff_fclose( pxSourceFile );
    }

    if( pxSourceFile != NULL )
    {
        ff_fclose( pxDestinationFile );
    }

    if( lBytesRead == lSourceFileLength )
    {
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

void prvCreateFileInfoString( char *pcBuffer, FF_FindData_t *pxFindStruct )
{
    int i = 0;
    /* Point pcAttrib to a string that describes the file. */
    if( ( pxFindStruct->ucAttributes & FF_FAT_ATTR_DIR ) != 0 )
    {
        pcBuffer[i++] = 'd';
    }
    else
    {
        pcBuffer[i++] = '-';
    }

    if( pxFindStruct->ucAttributes & FF_FAT_ATTR_READONLY )
    {
        pcBuffer[i++] = 'r';
        pcBuffer[i++] = '-';
    }
    else
    {
        pcBuffer[i++] = '-';
        pcBuffer[i++] = 'w';
    }
    pcBuffer[i++] = '-';
    pcBuffer[i++] = ' ';

    /* Create a string that includes the file name, the file size and the
    attributes string. */
    sprintf( pcBuffer + i, "%10d %s\n", ( int ) pxFindStruct->ulFileSize, pxFindStruct->pcFileName);
}

void vTask_console(void *param)
{
    vPortTaskUsesFPU(); // some command will print float

//    sciBASE_t *uart = CONSOLE_PORT;
    int i;
    uint8_t c;
    int res;

    init_cmd();

    int sharp_on = 0;
    char toomanyargs[] = "\nTOO MANY ARGS\n";
    char cmdtoolong[] = "\nCMD TOO LONG\n";
    char cmdnotfound[] = "CMD NOT FOUND\n";
    char cmdnotimpl[] = "CMD NOT IMPLEMENT\n";
    char *prompt = "> ";
    int len = 0;
    int old_len = 0;
    int start = 0;
    int argc;
    char *argv[MAX_ARGS];

    ff_getcwd( cmd_buffer, sizeof(cmd_buffer) );
    printk(cmd_buffer);
    printk(prompt);

    while(1)
    {
        if( xQueueReceive( qin, &c, ( portTickType ) portMAX_DELAY ) == pdTRUE)
        {
            switch (c)
            {
            case 3: // Ctrl+C
                printk("\n");
                ff_getcwd( cmd_buffer, sizeof(cmd_buffer) );
                printk(cmd_buffer);
                printk(prompt);
                len = 0;
                break;

            case 8:
            case 127: //backspace
                if(len > 0)
                {
                    len--;
                    printk("%c", c); // echo
                }
                break;

            case '#':
                sharp_on = 1;
                break;

            case '\t': //TAB
                //TODO
                break;

            case '\r':
            case '\n':
                printk("\n");
                cmd_buffer[len] = 0;
                len++;

                i = 0;
                start = 0;
                argc = 0;
                while(cmd_buffer[i] != 0)
                {
                    if(cmd_buffer[i] == ' ')
                    {
                        if(start == 1)
                            cmd_buffer[i] = 0;
                        start = 0;
                        i++;
                        continue;
                    }

                    if(start == 1)
                    {
                        i++;
                        continue;
                    }

                    start = 1;
                    if(argc >= MAX_ARGS)
                    {
                        printk(toomanyargs);
                        argc = 0;
                        break;
                    }
                    argv[argc] = cmd_buffer + i;
                    argc++;
                    i++;
                }

                if(argc > 0)
                {
                    res = execute_cmd(argc, argv);
                    if(res != CMD_SUCCESS)
                    {
                        if(res > 0)
                        {
                            printk("return code: %d", res);
                        }
                        else if(res == CMD_NOT_FOUND)
                        {
                            printk(cmdnotfound);
                        }
                        else if(res == CMD_NOT_IMPLEMENT)
                        {
                            printk(cmdnotimpl);
                        }
                    }
                }
                old_len = len;
                len = 0;
                ff_getcwd( cmd_buffer, sizeof(cmd_buffer) );
                printk(cmd_buffer);
                printk(prompt);
                break;

            case 27: // previous command, only work when command buffer not overwrite yet
                if(len == 0)
                {
                    len = old_len;
                    printk(cmd_buffer);
                }
                break;

            default:
                if(len < MAX_CMD_LEN)
                {
                    printk("%c", c); // echo
                    if(len == 0 && (c == ' ' || c == 0))
                        continue; // remove leading space
                    cmd_buffer[len] = c;
                    len++;
                }
                else
                {
                    printk(cmdtoolong);
                    ff_getcwd( cmd_buffer, sizeof(cmd_buffer) );
                    printk(cmd_buffer);
                    printk(prompt);
                    len = 0;
                }
                break;
            }
        }
    }
    //vTaskDelay(1000);
    vTaskDelete(0);
}
int cmd_info_handle(int argc, char **argv)
{

    printk("\n\r");
    printk("  __         __     ______     ______     ______     ______    ______     ______     __  __\r\n");
    printk(" /\\ \\       /\\ \\   /\\  ___\\   /\\  ___\\   /\\  __ \\   /\\__  _\\  /\\  ___\\   /\\  ___\\   /\\ \\_\\ \\\r\n");
    printk(" \\ \\ \\____  \\ \\ \\  \\ \\___  \\  \\ \\ \\____  \\ \\ \\/\\ \\  \\/_/\\ \\/  \\ \\  __\\   \\ \\ \\____  \\ \\  __ \\\r\n");
    printk("  \\ \\_____\\  \\ \\_\\  \\/\\_____\\  \\ \\_____\\  \\ \\_____\\    \\ \\_\\   \\ \\_____\\  \\ \\_____\\  \\ \\_\\ \\_\\\r\n");
    printk("   \\/_____/   \\/_/   \\/_____/   \\/_____/   \\/_____/     \\/_/    \\/_____/   \\/_____/   \\/_/\\/_/\r\n");
    printk("\n\r");
    printk("\n\r");
    printk("   Liscotech OBC system\r\n");
    printk("   MCU : TI TMS570LC4357\r\n");
    printk("   RAM : 4MB ECC SRAM\r\n");
    printk("   Flash : 256MB NAND Flash\r\n");
    printk("   SoftWare Version : %d.%d(%s-%s)\r\n",SOFTWARE_VERSION,SOFTWARE_BUILD,__DATE__,__TIME__);
    printk("\n\r");

    return CMD_SUCCESS;
}
