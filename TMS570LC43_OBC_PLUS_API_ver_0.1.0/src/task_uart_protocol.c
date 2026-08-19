/*
 * task_uart_protocol.c
 *
 *  Created on: 2023/03/16
 *      Author: smithKao
 */
#include <stdbool.h>
#include "task_uart_protocol.h"
#include "global.h"
#include "ff_time.h"
#include "tk2_storage.h"
#include "xioctl.h"
#include <math.h>
#include <stdio.h>


///* FreeRTOS+FAT includes. */
//#include "ff_headers.h"
//#include "ff_stdio.h"
//#include "ff_time.h"

/*nand_flash*/
#include "user_nand_flash.h"

#define configTIME_TIME_ZONE 8
#define PAYLOAD_MAX_LENGTH 65280

///* The number of bytes read/written to the example files at a time. */
//#define fsRAM_BUFFER_SIZE               1000

/* For transfer error Temperature from airs */
#define C_THERMAL_FACTOR   10000.0f
#define B_COEFFICIENT      3435.0f
#define T0_INV             (1.0f / 298.15f)
#define VREF_CORRECT       2.56f
#define VCC                3.3f

float convert_Tx_to_Treal(float Tx)
{
    float Kelvin_wrong = Tx + 273.15f;
    float y = (1.0f / Kelvin_wrong - 0.003354f) * B_COEFFICIENT;
    float ADC = 4096.0f / (1.0f + expf(-y));

    float Vraw = VREF_CORRECT * (ADC / 4096.0f);

    float Rt = (Vraw * C_THERMAL_FACTOR) / (VCC - Vraw);

    float Treal = 1.0f / (logf(Rt / 10000.0f) / B_COEFFICIENT + T0_INV) - 273.15f;

    return Treal;
}


TCTM_ProtocolStruct tctm_protocol __SRAM1_SECTION__;
CMD_DownloadData cmd_downloaddata;
GloablReceiveResult gloabl_receive_result_rsi;

ImageInfo imageinfo __SRAM2_SECTION__;

TCTM_CommandStruct tctm_cmd_table[] = {
    {TCTM_CMD_ECHO, TCTM_Echo, 0},
    {TCTM_CMD_READ, TCTM_ReadData, 0},
    {TCTM_CMD_WRITE, TCTM_WriteData, 0},
    {TCTM_CMD_IMAGE_INFO, TCTM_ImageInfo, 0},
    {TCTM_CMD_IMAGE_CAPTURE, TCTM_ImageCapture, 0},
    {TCTM_CMD_IMAGE_DOWNLOAD, TCTM_ImageDownload, 0},
    {TCTM_CMD_IMAGE_PREPARE, TCTM_ImagePrepare, 0},
    {TCTM_CMD_IMAGE_DELETE, TCTM_ImageDelete, 0},
    {TCTM_CMD_BAD_IMAGE_SETTING, TCTM_BadImageSetting, 0},
    {TCTM_CMD_RESET, TCTM_SystemReset, 0},
    {TCTM_CMD_COMPRESS, TCTM_Compress, 0},
};

const TCTM_DataListStruct accessDataList[] = {
    {"Version",0},
    {"Device State",0},
    {"SF2 ADC CH0",0},
    {"SF2 ADC CH1",0},
    {"SF2 ADC CH2",0},
    {"SF2 ADC CH3",0},
    {"SF2 ADC CH4",0},
    {"SF2 ADC CH5",0},
    {"SF2 ADC CH6",0},
    {"SF2 ADC CH7",0},
    {"CMOS ADC CH0",0},
    {"CMOS ADC CH1",0},
    {"CMOS ADC CH2",0},
    {"CMOS ADC CH3",0},
    {"CMOS ADC CH4",0},
    {"CMOS ADC CH5",0},
    {"CMOS ADC CH6",0},
    {"CMOS ADC CH7",0},
    {"SF2 System Voltage 5.0V",1},
    {"SF2 System Current",1},
    {"SF2 CMOS Current",1},
    {"SF2 System Voltage 3.3V",1},
    {"SF2 System Voltage 2.5V",1},
    {"SF2 System Voltage 1.8V",1},
    {"SF2 System Voltage 1.2V",1},
    {"SF2 Temperature",1},
    {"CMOS System Voltage 5.0V",1},
    {"CMOS System Current",1},
    {"CMOS System Voltage 3.3V",1},
    {"CMOS LDO Voltage 3.3V",1},
    {"CMOS CS Voltage 1.8V",1},
    {"CMOS Temperature",1},
    {"Image Amount",0},
    {"Unix Timestamp (sec)",0},
    {"Segment Size",0},
    {"Test Pattern Enabled",0},
    {"Auto Exposure Enabled",0},
    {"Auto White Balance Enabled",0},
    {"AI Enabled",0},
    {"JPEG Enabled",0},
    {"Debayer Enabled",0},
    {"Image Mode",0},
    {"Indensity",0},
    {"Auto Exposure Timeout",0},
    {"Multiplier",0},
    {"Exposure",0},
    {"Gain",0},
    {"AWB Bound Percent",1},
    {"CMOS Enabled",0},
    {"Average",0},
    {"Is Package Echo",0},
    {"Is Debug Enable",0},
};


struct GloablResult {
    TCTM_HandleResult g_result;
    CP_HandleResult state;
    uint8_t command;
    uint16_t dataLength;
    uint8_t data[MAX_BUFFER_LENGTH];
};
struct GloablResult gloablresult;

union ReadData {
    float f;
    unsigned int u;
};
union ReadData RD;

const TCTM_SendPackageOptions defaultOptions = {0, 0};

CP_ElementConfig tctm_protocol_configs[] = {
    // Leading Code
    {
        .size = 2,
        .magic = LEADING_CODE<<16,
        .type = CP_ELEMENT_TYPE_MAGIC,
        .flag = CP_FLAG_NONE,
    },
    // Command
    {
        .size = 1,
        .type = CP_ELEMENT_TYPE_COMMAND,
        .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_ARG,
    },
    // Data Length
    {
        .size = 2,
        .type = CP_ELEMENT_TYPE_LENGTH,
        .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_ARG,
    },
    // Data
    {
        .size = MAX_BUFFER_LENGTH,
        .type = CP_ELEMENT_TYPE_DATA,
        .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_LENGTH_BINDING | CP_FLAG_ARG,
    },
    // Checksum
    {
        .size = 2,
        .type = CP_ELEMENT_TYPE_CHECKSUM,
        .flag = CP_FLAG_NONE,
    },
    // Trailer Code
    {
        .size = 2,
        .magic = TRAILER_CODE<<16,
        .type = CP_ELEMENT_TYPE_MAGIC,
        .flag = CP_FLAG_NONE,
    },
};

int TCTM_Echo(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    //printk_ni("echo\n");
    if(data[0]!=CP_SUCCESS){
        printk_ni("Get TCTM Package Result: %d, %s\n", data[0], CP_ERROR_CODE[data[0]]);
    }
    return TCTM_SUCCESS;
}
//

int TCTM_ReadData(uint8_t command, uint16_t dataLength, uint8_t *data)
{
   int i ,index;
   while (dataLength >= 6){
        RD.u = 0;
        index =(data[1] & 0xFF)/4;
        for (i = 0; i < 4; i++) {
            RD.u = (RD.u<<8)|data[(i+2)];
        }
        printk_ni("%s : ",accessDataList[index].name);
        if(index == 32){
            RD.u++;
        }
        if(index == 25 || index == 31){
            float correct_temp = convert_Tx_to_Treal(RD.f);
            printk_ni("%f\n",correct_temp);
        }
        else if(accessDataList[index].type == 0){//int
            printk_ni("%d\n",RD.u);
        }
        else if(accessDataList[index].type == 1){//float
            printk_ni("%f\n",RD.f);
        }
        data += 6;
        dataLength -= 6;
    }
    return TCTM_SUCCESS;
}

int TCTM_ImageInfo(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    FF_TimeStruct_t TimeBuf;
    if(dataLength == 0){
        printk_ni("IMAGE NOT EXIST\n");
        return TCTM_IMAGE_NOT_EXIST;
    }
    while (dataLength >= 15) {
        imageinfo.index = (data[0] << 8) | data[1];
        imageinfo.sequence = (data[2] << 8) | data[3];
        imageinfo.type = data[4];
        imageinfo.mode = data[5];
        imageinfo.flag = data[6];
        imageinfo.timestamp = (data[7] << 24) | (data[8] << 16) | (data[9] << 8) | data[10];
        imageinfo.size = (data[11] << 24) | (data[12] << 16) | (data[13] << 8) | data[14];

        printk_ni(" - Index: %u, Sequence = %u, Mode = %u, Type: %u (%u), Flag = %u\n",
                  imageinfo.index, imageinfo.sequence, imageinfo.mode, imageinfo.type, imageinfo.type, imageinfo.flag);
        FreeRTOS_gmtime_r((const time_t*)&imageinfo.timestamp, &TimeBuf);
        printk_ni("Time : %4d-%02d-%02d %02d:%02d:%02d  GMT+%d\n", TimeBuf.tm_year+1900, TimeBuf.tm_mon+1, TimeBuf.tm_mday, TimeBuf.tm_hour+8, TimeBuf.tm_min, TimeBuf.tm_sec, configTIME_TIME_ZONE);
        printk_ni("Size = %u\n\n", imageinfo.size);
        data += 15;
        dataLength -= 15;
    }

    return TCTM_SUCCESS;
}

int TCTM_WriteData(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    return data[0];
}

int TCTM_ImageDelete(uint8_t command, uint16_t dataLength, uint8_t* data)
{
    if(data[0]== TCTM_SUCCESS)
        printk_ni("SUCCESS Delete\n");
    else
        printk_ni("Failed Delete\n");

    return data[0];
}

int TCTM_ImageCapture(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    uint16_t buffer = 0x80; //Image Amount
    if(data[0]==0){
        printk_ni("# Image Capture Finished\n");
//        SendPackage(TCTM_CMD_READ, 2, &buffer,defaultOptions);
    }
    else
        printk_ni("# Image Capture Error\n");
    return data[0];
}
int TCTM_ImageDownload(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    int i;
    int result = data[0];
    uint32_t checksum = 0;

    uint32_t value = 0;

    for(i=1; i<5; i++)
        checksum = (checksum << 8) | data[i];
    if(result == TCTM_SUCCESS){
        /*SPI Transfer*/
        spi_transfer(SPI_DEVICE1, 29696, spi_tx_buffer, spi_rx_buffer);
        while(is_spi_ready(SPI_DEVICE1));
        for(i=0; i<SEGMENTSIZE; i++){
            value = (value + spi_rx_buffer[i]) & 0xFFFFFFFF;
        }
        if (checksum != value){
            printk_ni("value = %08x, check value = %08x\n" , value, checksum);
            spi_transfer(SPI_DEVICE1, 29696, spi_tx_buffer, spi_rx_buffer); //To clear buffer
            while(is_spi_ready(SPI_DEVICE1));
            return DOWNLOAD_CHECKSUM_FAIL;
        }
    }
    else
        return result;
    return TCTM_SUCCESS;
}

int TCTM_ImagePrepare(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    return data[0];
}

int TCTM_BadImageSetting(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    if(data[0]==TCTM_SUCCESS)
        printk_ni("SUCCESS BadImageSetting\n");
    else
        printk_ni("Failed BadImageSetting\n");

    return data[0];
}
int TCTM_Compress(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    if(data[0]==TCTM_SUCCESS)
        printk_ni("Compression completed\n");
    else if(data[0]==TCTM_DOWNLOAD_DATA_PREPAR)
        printk_ni("Compression starts\n");
    else
        printk_ni("Compression failed\n");

    return data[0];
}
int TCTM_SystemReset(uint8_t command, uint16_t dataLength, uint8_t *data)
{
    if(data[0]==TCTM_SUCCESS)
        printk_ni("SUCCESS SystemReset\n");
    else
        printk_ni("Failed SystemReset\n");

    return data[0];
}

void SendPackage(uint8_t command, uint16_t length, uint8_t *data,TCTM_SendPackageOptions options)
{
    if(gloablresult.state == CP_HANDLE_RESULT_WAIT){
        printk_ni("RSI in busy, please wait\n");
        return;
    }
    /* Put all data in side buffer, so you don't need to define static array to avoid data lost */
    CP_PackerInitial(&tctm_protocol.transmitPacker, tctm_protocol_configs, sizeof(tctm_protocol_configs) / sizeof(CP_ElementConfig));
    CP_GeneratePackage(&tctm_protocol.transmitPacker, &command, &length, data);

    if(options.awaitResponse!=0){
        gloablresult.state = CP_HANDLE_RESULT_WAIT;
        if(options.timeout==0){
            options.timeout = 3000;
        }

        if (tctm_protocol.transmitPacker.isValid)
            uart_write(RSI2000_PORT, tctm_protocol.transmitPacker.bufferIndex, tctm_protocol.transmitPacker.buffer);
        while(options.timeout > 0){
            vTaskDelay(10);
            options.timeout -= 10;
            if(gloablresult.state != CP_HANDLE_RESULT_WAIT)
                break;
        }
        if(gloablresult.state == CP_HANDLE_RESULT_WAIT)
            gloablresult.state = CP_FAIL;
    }
    else{
        if (tctm_protocol.transmitPacker.isValid)
            uart_write(RSI2000_PORT, tctm_protocol.transmitPacker.bufferIndex, tctm_protocol.transmitPacker.buffer);
    }
    CP_ClearPackage(&tctm_protocol.transmitPacker);
    CP_PackerRelease(&tctm_protocol.transmitPacker);
}

void CommandHandle(CP_PackagePacker *packer)
{
    uint32_t i;
    TCTM_HandleResult result;
    CP_GetElementValue(packer, 1, &gloablresult.command);
    CP_GetElementValue(packer, 2, &gloablresult.dataLength);
    CP_GetElementValue(packer, 3, &gloablresult.data);

    for (i = 0; i < sizeof(tctm_cmd_table) / sizeof(TCTM_CommandStruct); i++)
    {
        if (tctm_cmd_table[i].command == gloablresult.command)
        {
            tctm_cmd_table[i].commandstate = 1;
            break;
        }
    }
    CP_PackerDataClear(packer);
    CP_GetNextElement(packer);
}

void GetPackageCallback(CP_PackagePacker *packer, CP_HandleResult result)
{
    if (result == CP_SUCCESS)
        {
            CommandHandle(packer);
        }
    else{
        printk_ni("Get TCTM Package Result: %d, %s\n", result, CP_ERROR_CODE[result]);
        CP_PackerDataClear(packer);
        CP_GetNextElement(packer);
    }
}
void PutByteToReceivePackage(){
    CP_PackerPutByte(&tctm_protocol.receivePacker, SCI3RXBUF);
}

int GetOperateResult(){
    return gloablresult.g_result;
}

int GetOperateResponse(){
    return gloablresult.state;
}

void TCTM_CMD_Download(uint8_t flag, uint32_t value){
    /*get Image index*/
    uint16_t i;
    uint16_t index;
    uint16_t datalen = 5;
    uint8_t *buffer_index = (uint8_t*)pvPortMalloc(sizeof(uint8_t)*datalen);
    int segmentSize = SEGMENTSIZE;
    memset(buffer_index, 0, sizeof(uint8_t)*datalen);
    if (flag == 0)
       index = value;
    else if (flag == 1){
       buffer_index[0] = 1;
       for(i=1;i<5;i++){
           buffer_index[i] = value >> (32-8*i);
       }
       SendPackage(TCTM_CMD_IMAGE_INFO, datalen, buffer_index,(TCTM_SendPackageOptions){1,30});
       index = imageinfo.index;
    }
    else if (flag == 2){
        buffer_index[0] = 2;
        for(i=1;i<5;i++){
            buffer_index[i] = value >> (32-8*i);
        }
        SendPackage(TCTM_CMD_IMAGE_INFO, datalen, buffer_index,(TCTM_SendPackageOptions){1,30});
        index = imageinfo.index;
    }

    /*Set segment size*/
    if(segmentSize > PAYLOAD_MAX_LENGTH)
        segmentSize = PAYLOAD_MAX_LENGTH;
    unsigned short *buffer_write = (unsigned short*)pvPortMalloc(sizeof(unsigned short)*6);
    memset(buffer_write, 0, sizeof(unsigned short)*6);
    buffer_write[0] = 136;
    buffer_write[2] = segmentSize;
    SendPackage(TCTM_CMD_WRITE, 6, (uint8_t*)buffer_write,(TCTM_SendPackageOptions){1,30});
    buffer_write = (unsigned short*)pvPortMalloc(sizeof(unsigned short)*2);
    memset(buffer_write, 0, sizeof(unsigned short)*2);
    buffer_write[0] = 0x88;
    SendPackage(TCTM_CMD_READ, 2, (uint8_t*)buffer_write,(TCTM_SendPackageOptions){1,30});
    vPortFree(buffer_write);
    if(RD.u!=segmentSize){
        printk_ni("Segment Size RW Error,RD.u = %d\n",RD.u);
        return;
    }



    /*Get ImageInfo*/
    uint32_t size;
    datalen = 5;
    memset(buffer_index, 0, sizeof(uint8_t)*datalen);
    buffer_index[0] = 0;
    for(i=1;i<5;i++)
        buffer_index[i] = index >> (32-8*i);
    SendPackage(TCTM_CMD_IMAGE_INFO, datalen, buffer_index,(TCTM_SendPackageOptions){1,300});
    index = imageinfo.index;
    size = imageinfo.size;
    vPortFree(buffer_index);
//    printk_ni("Image Size = %d\n",size);

    /*Prepare Image File*/
    uint16_t *buffer_Prepare = (uint16_t*)pvPortMalloc(sizeof(uint16_t));
    memset(buffer_Prepare, 0, sizeof(uint16_t));
    buffer_Prepare[0] = index;
    printk("Prepare Image File\n");
    while(1){
        SendPackage(TCTM_CMD_IMAGE_PREPARE, 2, (uint8_t*)buffer_Prepare,(TCTM_SendPackageOptions){1,30});
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

    /*DOWNLOAD*/

    int obc_index = find_storage_index();
    imageinfo.index = obc_index;
    image_info_to_buffer();
    printk_ni("\n # === Image Start downloading === \n",index);
    if(save_image(index, obc_index, size) == 1)
    {
        save_label(obc_index);
        printk_ni("\n The index in OBC = [%d] \n",obc_index);
        printk_ni("\n # === Image Download completed === \n");
    }
    else{
        printk_ni("\n # === Download Fail! === \n");
    }
}

void vTask_uart_protocol(void *pvParameters)
{
    CP_PackerInitial(&tctm_protocol.receivePacker, tctm_protocol_configs, sizeof(tctm_protocol_configs) / sizeof(CP_ElementConfig));
    tctm_protocol.receivePacker.callback = GetPackageCallback;
    TCTM_HandleResult g_result;
    uint8_t i;
    while (1)
    {
        for (i = 0; i < sizeof(tctm_cmd_table) / sizeof(TCTM_CommandStruct); i++)
        {
            if (tctm_cmd_table[i].commandstate == 1)
            {
                g_result = tctm_cmd_table[i].handle(gloablresult.command, gloablresult.dataLength, gloablresult.data);
                tctm_cmd_table[i].commandstate = 0;
                if(tctm_cmd_table[i].command != TCTM_CMD_ECHO){
                    gloablresult.state = CP_SUCCESS;
                    gloablresult.g_result = g_result;
                }
                break;
            }
        }
        vTaskDelay(1);
    }
    vTaskDelete(0);
}

void image_info_to_buffer(){
    imageinfo_buffer[0] = imageinfo.index >> 8;
    imageinfo_buffer[1] = imageinfo.index;
    imageinfo_buffer[2] = imageinfo.sequence >> 8;
    imageinfo_buffer[3] = imageinfo.sequence;
    imageinfo_buffer[4] = imageinfo.type;
    imageinfo_buffer[5] = imageinfo.mode;
    imageinfo_buffer[6] = imageinfo.flag;
    imageinfo_buffer[7] = imageinfo.timestamp >> 24;
    imageinfo_buffer[8] = imageinfo.timestamp >> 16;
    imageinfo_buffer[9] = imageinfo.timestamp >> 8;
    imageinfo_buffer[10] = imageinfo.timestamp;
    imageinfo_buffer[11] = imageinfo.size >> 24;
    imageinfo_buffer[12] = imageinfo.size >> 16;
    imageinfo_buffer[13] = imageinfo.size >> 8;
    imageinfo_buffer[14] = imageinfo.size;
    imageinfo_buffer[15] = 0;
    imageinfo_buffer[16] = 1;
}

