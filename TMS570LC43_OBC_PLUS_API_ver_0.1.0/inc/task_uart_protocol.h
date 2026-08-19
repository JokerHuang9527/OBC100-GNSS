/*
 * task_uart_protocol.h
 *
 *  Created on: 2023/03/16
 *      Author: smithKao
 */
#ifndef TASK_UART_PROTOCOL_H_
#define TASK_UART_PROTOCOL_H_
#include "common_protocol.h"

#define LEADING_CODE 0x5AA5
#define TRAILER_CODE 0x33CC
#define MAX_BUFFER_LENGTH 256

#define CMD_CAPTURE_LEN 6
#define CMD_IMAGE_INFO_LEN 5
#define CMD_IMAGE_DELETE_LEN 5
#define CMD_BAD_IMAGE_SETTING_LEN 6
#define CMD_PREPARE_LEN 2

typedef struct
{
    uint8_t command;
    int (*handle)(uint8_t, uint16_t, uint8_t *);
    uint8_t commandstate;
} TCTM_CommandStruct;

typedef struct
{
    uint8_t command;
    uint16_t dataLength;
    uint8_t data[MAX_BUFFER_LENGTH];
    CP_HandleResult result;
} GloablReceiveResult;

typedef struct {
    uint16_t index;
    uint16_t sequence;
    uint8_t type;
    uint8_t mode;
    uint8_t flag;
    uint32_t timestamp;
    uint32_t size;
    uint8_t lockflag;
    uint8_t showflag;
}ImageInfo;
extern ImageInfo imageinfo;

typedef struct
{
    char *name;
    int  type;//0:int,1:float
} TCTM_DataListStruct;

typedef struct {
    int awaitResponse;
    int timeout;
} TCTM_SendPackageOptions;

struct TCTM_AccessDataStruct
{
    uint32_t data;
    uint16_t address;
}__attribute__((packed));
typedef struct TCTM_AccessDataStruct TCTM_AccessData;

struct TCTM_DownloadDataSturct
{
    uint16_t index;
    uint16_t segmentIndex;
}__attribute__((packed));
typedef struct TCTM_DownloadDataSturct TCTM_DownloadData;

struct TCTM_ImageOperateStruct
{
    uint8_t flag;
    uint32_t value;
}__attribute__((packed));
typedef struct TCTM_ImageOperateStruct TCTM_ImageOperate;

typedef union
{
    uint8_t buffer[MAX_BUFFER_LENGTH];
    TCTM_AccessData access;
    TCTM_ImageOperate select;
} TCTM_DataUnion;

typedef enum
{
    TCTM_SUCCESS = 0x00,
    TCTM_FAIL,
    TCTM_RW_ADDRESS_ERROR,
    // Info
    TCTM_IMAGE_NOT_EXIST,
    TCTM_IMAGE_BAD_BLOCK_DETECT,
    // Capture
    TCTM_CAPTURE_DMA_TIMEOUT,
    TCTM_IMAGE_SAVING_TIMEOUT,
    // Download
    TCTM_DOWNLOAD_DMA_TIMEOUT,
    TCTM_DOWNLOAD_DATA_PREPAR,
    TCTM_DOWNLOAD_MEMORY_FAIL,

    DOWNLOAD_CHECKSUM_FAIL,

} TCTM_HandleResult;

typedef struct
{
    CP_PackagePacker receivePacker;
    CP_PackagePacker transmitPacker;
} TCTM_ProtocolStruct;

struct CMD_DownloadDataSturct
{
    uint8_t type;
    uint32_t value;
    uint8_t flag;
}__attribute__((packed));
typedef struct CMD_DownloadDataSturct CMD_DownloadData;

typedef enum
{
    TCTM_CMD_ECHO = 0,
    TCTM_CMD_READ,
    TCTM_CMD_WRITE,
    TCTM_CMD_IMAGE_INFO,
    TCTM_CMD_IMAGE_CAPTURE,
    TCTM_CMD_IMAGE_DOWNLOAD,
    TCTM_CMD_IMAGE_PREPARE,
    TCTM_CMD_IMAGE_DELETE,
    TCTM_CMD_BAD_IMAGE_SETTING,
    TCTM_CMD_RESET,
    TCTM_CMD_COMPRESS,
} TCTM_COMMAND;

extern const TCTM_DataListStruct accessDataList[];
extern const TCTM_SendPackageOptions defaultOptions;
extern CMD_DownloadData cmd_downloaddata;
/* TCTM Command */
int TCTM_Echo(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_ReadData(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_WriteData(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_ImageInfo(uint8_t command, uint16_t dataLength, uint8_t* data);
int TCTM_ImageCapture(uint8_t command, uint16_t dataLength, uint8_t* data);
int TCTM_ImageDownload(uint8_t command, uint16_t dataLength, uint8_t* data);
int TCTM_ImagePrepare(uint8_t command, uint16_t dataLength, uint8_t* data);
int TCTM_ImageDelete(uint8_t command, uint16_t dataLength, uint8_t* data);
int TCTM_BadImageSetting(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_SystemReset(uint8_t command, uint16_t dataLength, uint8_t *data);
int TCTM_Compress(uint8_t command, uint16_t dataLength, uint8_t* data);

void TCTM_CMD_Download(uint8_t flag, uint32_t value);

void SendPackage(uint8_t command, uint16_t length, uint8_t *data,TCTM_SendPackageOptions);
void CommandHandle(CP_PackagePacker *packer);
void GetPackageCallback(CP_PackagePacker *packer, CP_HandleResult result);
void PutByteToReceivePackage();
int GetOperateResult();
int GetOperateResponse();
void vTask_uart_protocol(void *pvParameters);
void image_info_to_buffer();
#endif /* TASK_UART_PROTOCOL_H_ */
