/*
 * task_can.h
 *
 *  Created on: 2019¦~10¤ë30¤é
 *  Author: kusoyao
 *  modified on:2010/05/14
 *  Author: Ivan Yu
 *  if user using pcan-view to test canbus functionality, user can
 *  load script from below project path on the pcan-view
 *  project_path/doc/verification_doc/OBC_CANBUS_SCRIPT.xmt
 */

#ifndef INC_TASK_CAN_H_
#define INC_TASK_CAN_H_
#define CMD_CANBUS_TEST 0x00
/* CMD_TEST_CANBUS command format
cammand:
data0 data1 data2 ....data7
CMD_TEST_CANBUS 00 00 00, 00 00 00 00(data0=CMD_TEST_CANBUS, data1~data7 can type any value)
*/

#define CMD_SF2 0x01
#define PARAM1_PHOTO 0x00
#define PARAM2_PHOTO_PAUSE 0x00
#define PARAM2_PHOTO_CONTIONUE 0x01
#define PARAM1_NORFLASH 0x01
#define PARAM2_NORFLASH_SAVE_CURRENT_IMG 0x00
#define PARAM2_NORFLASH_SAVE_SPECIFY_IMG 0x01
#define PARAM2_NORFLASH_READ_IMG 0x02
#define PARAM2_NORFLASH_INFO 0x03

/* PARAM3_NORFLASH_INFO_NUM1 = 1
* read area1 saved image number and has been masked or not
...
* PARAM3_NORFLASH_INFO_NUM39 = 39
* read area39 saved image number and has been masked or not
*/


#define PARAM2_NORFLASH_MASK_AREA 0x04
#define PARAM2_NORFLASH_CANCLE_MASK_AREA 0x05
#define PARAM1_SEND_ONE_IMG 0X02
#define PARAM1_TMP 0X03
#define PARAM1_CMOS 0X04
#define PARAM2_CMOS_READ 0x00
#define PARAM2_CMOS_WRITE 0x01
#define PARAM1_TR 0X05
#define PARAM1_SF2_RESET 0X06

/* CMD_SF2 command format
pause photograph:
CMD_SF2 PARAM1_PHOTO PRAM2_PHOTO_PAUSE 00, 00 00 00 00

contionue photograph:
CMD_SF2 PARAM1_PHOTO PRAM2_PHOTO_CONTIONUE 00, 00 00 00 00

save current image:
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_SAVE_CURRENT_IMG 00, 00 00 00 00
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_SAVE_SPECIFY_IMG 00, 00 00 00 00 , ex: CMD_SF2 PARAM1_NORFLASH PRAM2_NORFLASH_SAVE_SPECIFY_IMG  00 00 , 00 00 a0 00 
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_READ_CURRENT_IMG 00, 00 00 00 IMAGE_NUMBER(dec)

only send one image from SF2
CMD_SF2 PARAM1_SEND_ONE_IMG 00 00, 00 00 00 00

read SF2 CMOS temperature
CMD_SF2 PARAM1_TMP 00 00, 00 00 00 00

read SF2 NORFLASH INFO
ex:read N area which saves image number(1~39) 
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_INFO PRARM3_NORFLASH_NUMX(dec), 00 00 00 00
ex:read N area which mask or not 
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_INFO PRARM3_NORFLASH_NUMX(dec), 00 00 00 00

set norflash mask area
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_MASK_AREA 00, 00 00 00 num(dec:0~39)

cancle norflash mask area
CMD_SF2 PARAM1_NORFLASH PARAM2_NORFLASH_CANCLE_MASK_AREA 00, 00 00 00 num(dec:0~39)

read SF2 CMOS parameter
CMD_SF2 PARAM1_CMOS PARAM2_CMOS_READ 00, CMOS parameter(dec), COMS parameter(dec) 00 00
ex: CMD_SF2 PARAM1_CMOS PARAM2_CMOS_READ 00, 02 01 00 00
which means read CMOS 201 parameter

write data to SF2 CMOS parameter
CMD_SF2 PARAM1_CMOS PARAM2_CMOS_WRITE 00, CMOS parameter(dec), COMS parameter(dec) context(hex) context(hex)
ex: CMD_SF2 PARAM1_CMOS PARAM2_CMOS_WRITE 00, 02 01 00 C8
which means write 0x00C8CMOS to 201 parameter

read SF2 tailer
CMD_SF2 PARAM1_TR 00 00, 00 00 00 00

SF2 RESET
CMD_SF2 PARAM1_SF2_RESET 00 00, 00 00 00 00

*/

#define CMD_OBC_MODE 0x02
/* OBC MODE 2 N command format
CMD_OBC_MODE 00 00 00, 00 00 00 mode number(0~5)
 */

#define max_save_image 39

void vTask_can(void *param);
typedef struct can_data_format{
	uint8_t cmd;
	uint8_t param1;
	uint8_t param2;
	uint8_t param3;
	uint32_t value;
}CDF;

#endif /* INC_TASK_CAN_H_ */
