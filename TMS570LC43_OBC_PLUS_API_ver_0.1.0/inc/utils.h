/*
 * utils.h
 *
 *  Created on: 2019¦~1¤ë8¤é
 *      Author: kusoyao
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include <stdint.h>

typedef struct rti {
    uint32_t frc;
    //uint32_t uc;
}rti_t ;

void save_rti_time(rti_t *buf);
uint32_t diff_rti(rti_t *now, rti_t *past);
void copy_rti_time(rti_t *to, rti_t *from);
uint32_t rti_to_nanosecond(uint32_t count);
uint32_t rti_to_microsecond(uint32_t count);
uint32_t rti_to_millisecond(uint32_t count);
uint32_t rti_to_second(uint32_t count);

void dump_memory_uint8(const uint8_t *start, int len);
void dump_memory_uint16(const uint16_t *start, int len);
void dump_memory_uint32(const uint32_t *start, int len);
void fill_memory_uint8(uint8_t *start, int len, uint8_t value);
void fill_memory_uint16(uint16_t *start, int len, uint16_t value);
void fill_memory_uint32(uint32_t *start, int len, uint32_t value);
uint32_t compare_memory_pattern_uint8(const uint8_t *start, int len, uint8_t pattern);
uint32_t compare_memory_pattern_uint16(const uint16_t *start, int len, uint16_t pattern);
uint32_t compare_memory_pattern_uint32(const uint32_t *start, int len, uint32_t pattern);
uint32_t compare_memory_uint8(const uint8_t *start1, const uint8_t *start2, int len);
uint32_t compare_memory_uint16(const uint16_t *start1, const uint16_t *start2, int len);
uint32_t compare_memory_uint32(const uint32_t *start1, const uint32_t *start2, int len);

void system_sw_reset();
void checksum(uint32_t , uint32_t);

#endif /* INC_UTILS_H_ */
