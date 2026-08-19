/*
 * UART_API.h
 *
 *  Created on: 2021/05/14
 *      Author: Ivan Yu
 * This file is for below two functionalities 
 * (1) for user and OBC communicates each other by CANBUS or console
 * (2) according to above (1) OBC received command will send related command to SF2 by UART,
 *      all of OBC sending command to SF2 by UART must be written in UART_API.c 
 */

#ifndef UART_API_H_
#define UART_API_H_

#define image_size 0x190000
uint32_t address_table[10];
char send_msg[100];

struct sf2_norflash_info{
	uint8_t area;
	uint8_t image_index;
};

typedef struct sf2_image_info_struct{
    volatile int imgmode_status;
    volatile int AE_exposure;
    volatile int AE_mult_timer;
}sf2_image_info_s;

extern sf2_image_info_s sf2_image_info;



/* for only SF2 reset, OBC needs reset related parameters */
extern uint8_t SF2_RESET;

void init_UART_API();
uint8_t find_address_index_in_table(uint32_t , uint8_t *);
void show_image_tailer();
int read_image_tailer();
void pause_photograph_cmd_to_SF2();
void continue_photograph_cmd_to_SF2();
uint8_t save_current_img_cmd_to_SF2(uint8_t, char *, char * ,struct sf2_norflash_info *);
void read_specify_img_cmd_to_SF2(char *);
void OBC_change_mode(uint8_t);
void SF2_send_one_image();
void set_change_OBC_mode_from_SF2_switch(uint8_t);
void mask_nor_flash_area_cmd_to_SF2(char *);
void cancle_mask_nor_flash_area_cmd_to_SF2(char *);
uint8_t read_SF2_state(uint8_t * , char *);
uint8_t read_CMOS_temperature(int *, char *); 
int read_CMOS_parameter_by_specify_num(char *, char *);
void write_CMOS_parameter_by_specify_num(char *, char *);
void read_norflash_info(uint8_t, uint8_t *, char *);
void let_SF2_reset();
uint8_t read_SF2_RESET();
void set_SF2_RESET(uint8_t);
void SF2_image_capture();
void SF2_send_part_of_image(char *part_num_src);
void SF2_read_image_mode_info();
void SF2_setting_mode0(char*, char*);
void SF2_setting_mode1(char *, char *);
void SF2_setting_mode2(char *);
void SF2_setting_mode3(char *, char *);
void SF2_setting_mode4(char *, char *, char *);
void SF2_setting_mode5(char *);
void SF2_receive_mode0_info(char *, char *, char *);
void SF2_receive_mode1_info(char *, char *, char *);
void SF2_receive_mode2_info(char *, char *, char *);
void SF2_receive_mode3_info(char *, char *, char *);
void SF2_receive_mode4_info(char *);
void SF2_receive_mode5_info(char *);
void SF2_cmos_power_off();
void SF2_read_version();
void SF2_read_adc(char *);
void read_SF2_LVDS_delay_status(char *receive_msg);
#endif /* UART_API_H_ */
