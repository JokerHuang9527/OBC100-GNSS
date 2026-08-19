/*
 * task_tk2_sicd.h
 *
 *  Created on: 2021¦~12¤ë20¤é
 *      Author: User
 */

#ifndef INC_TASK_TK2_SICD_H_
#define INC_TASK_TK2_SICD_H_

#define I2C_SAND_SKIP       0
#define I2C_SAND_6_BYTE     6
#define I2C_SAND_64_BYTE    64

#define SICD_WAIT_TIME 5000
#define SICD_JPEG_WAIT_TIME 15000
#define SICD_TIMEOUT 10000

typedef enum{
    SICD_SOFTWARE_VERSION = 0x00,
    SICD_ECHO_REGISTER,
    SICD_TIMESET_REGISTER,
    SICD_ATTITUDE_KNOWLEDGE,
    SICD_RESET_REGISTER,
    SICD_CAMERA_MODULE_POWER_CONTROL_REGISTER,
    SICD_CAMERA_MODULE_SUSPEND_CONTROL_REGISTER,
    SICD_CAPTURE_CONTROL_REGISTER,
    SICD_MODE_0_SETTING,
    SICD_MODE_1_SETTING,
    SICD_MODE_2_SETTING,
    SICD_MODE_3_SETTING,
    SICD_MODE_4_SETTING,
    SICD_MODE_5_SETTING,
    SICD_CLEAR_HOUSE_KEEPING_DATA = 0x10,
    SICD_STORAGE_SYSTEM_TEST,
    SICD_READ_STORAGE_SYSTEM_TEST,
    SICD_CONTROL_STORAGE_SYSTEM,
    SICD_STORAGE_PICTURE_INFO,
    SICD_STORAGE_PICTURE_DOWNLOAD,
    SICD_STORAGE_TRANSFER_CONFIG,
    SICD_MOVE_PICTURE_TO_DOWNLOAD,
    SICD_SET_CAPTURE_PHOTO_NUMBER,
    SICD_CAPTURE_TRIGGER,
    SICD_BUSY_REGISTER = 0x20,
    SICD_HOUSE_KEEPING_DATA_1,
    SICD_HOUSE_KEEPING_DATA_2,
    SICD_HOUSE_KEEPING_DATA_3,
    SICD_HOUSE_KEEPING_DATA_4,
    SICD_HOUSE_KEEPING_DATA_5,
    SICD_MODE_4_MUL_EXP = 0x30,
    SICD_STATUS_MONITORS_DATA_S = 0x40,
    SICD_STATUS_MONITORS_DATA_E = 0x67
}SICDCommand;

extern SemaphoreHandle_t xSPIxFinshSemaphore;

void sicd_command_parser(uint8_t* pRX_Data_Slave,uint8_t* pi2c_buffer,uint8_t* pi2c_sand_byte);
void vTask_tk2_sicd(void *param);
void vTask_empty_test(void *param);
uint32_t get_current_time();
void update_table_info(uint8_t photo_number);
void clear_single_photo_table_info(uint8_t photo_number);

void UpdateInfoSequence();
uint8_t GetPhotoNumber();
uint8_t GetFreeSpaceNumber();
void WaitSF2Status(uint32_t status, uint32_t wait, uint32_t timeout);
void ImageDownloadFromSF2(uint8_t photo_number);

#endif /* INC_TASK_TK2_SICD_H_ */
