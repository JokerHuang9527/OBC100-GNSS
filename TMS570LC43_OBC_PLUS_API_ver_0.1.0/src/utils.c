/*
 * utils.c
 *
 *  Created on: 2019¦~1¤ë8¤é
 *      Author: kusoyao
 */

#include "FreeRTOS.h"

#include "utils.h"
#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYSECR        (*(volatile uint32 *)0xFFFFFFE0U)

#define portRTI_CNT0_FRC0_REG   ( * ( ( volatile uint32_t * ) 0xFFFFFC10 ) )
#define portRTI_CNT0_UC0_REG    ( * ( ( volatile uint32_t * ) 0xFFFFFC14 ) )
#define freq_rtifrc ( configCPU_CLOCK_HZ / 2 ) //divide 2 since RTICPUCx set to 1
#define period_ns ( 1000000000. / freq_rtifrc )
#define period_us ( 1000000. / freq_rtifrc )
#define period_ms ( 1000. / freq_rtifrc )
#define period_sec ( 1. / freq_rtifrc )
void save_rti_time(rti_t *buf)
{
    buf->frc = portRTI_CNT0_FRC0_REG;        //read this first
    //buf->uc = portRTI_CNT0_UC0_REG;
}

uint32_t diff_rti(rti_t *now, rti_t *past)
{
    uint32_t diff;

    if(now->frc >= past->frc)
        diff = now->frc - past->frc;
    else
    {
        diff = ~(past->frc - now->frc) + 1;
        //diff = 0xffffffff - past->frc + now->frc + 1;
    }
    return diff;
}

void copy_rti_time(rti_t *to, rti_t *from)
{
    to->frc = from->frc;
    //to->uc = from->uc;
}

uint32_t rti_to_nanosecond(uint32_t count)
{
    return period_ns * count;
}
uint32_t rti_to_microsecond(uint32_t count)
{
    return period_us * count;
}
uint32_t rti_to_millisecond(uint32_t count)
{
    return period_ms * count;
}
uint32_t rti_to_second(uint32_t count)
{
    return period_sec * count;
}
void dump_memory_uint8(const uint8_t *start, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        if(((uint32_t)start & 0xf) == 0)
            printk("%08x ", start);
        printk("%02x ", *start);
        if(((uint32_t)start & 0xf) == 0xf)
            printk("\n");
        start++;
    }
    printk("\n");
}

void dump_memory_uint16(const uint16_t *start, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        if(((uint32_t)start & 0xf) == 0)
            printk("%08x ", start);
        printk("%04x ", *start);
        if(((uint32_t)start & 0xf) == 0xe)
            printk("\n");
        start++;
    }
    printk("\n");
}

void dump_memory_uint32(const uint32_t *start, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        if(((uint32_t)start & 0xf) == 0)
            printk("%08x ", start);
        printk("%08x ", *start);
        if(((uint32_t)start & 0xf) == 0xc)
            printk("\n");
        start++;
    }
    printk("\n");
}

void fill_memory_uint8(uint8_t *start, int len, uint8_t value)
{
    int i;
    for(i = 0; i < len; i++)
    {
        *start = value;
        start++;
    }
}

void fill_memory_uint16(uint16_t *start, int len, uint16_t value)
{
    int i;
    for(i = 0; i < len; i++)
    {
        *start = value;
        start++;
    }
}

void fill_memory_uint32(uint32_t *start, int len, uint32_t value)
{
    int i;
    for(i = 0; i < len; i++)
    {
        *start = value;
        start++;
    }
}

uint32_t compare_memory_pattern_uint8(const uint8_t *start, int len, uint8_t pattern)
{
    int i;
    uint8_t xor;
    uint8_t blen = 8;
    uint8_t bb[8] = {0};
    uint32_t bit1to0[8] = {0};
    uint32_t bit0to1[8] = {0};
    uint32_t bit1to0_total = 0;
    uint32_t bit0to1_total = 0;

    for(i = 0; i< blen; ++i)
    {
        if(pattern & (1 << i))
            bb[i] = 1;
        else
            bb[i] = 0;
    }

    while(len--)
    {
        xor = (*start) ^ pattern;
        if(xor != 0)
        {
            for(i = 0; i< blen; ++i)
            {
                if(xor & (1 << i))
                {
                   if(bb[i])
                       bit1to0[i]++;
                   else
                       bit0to1[i]++;
                }
            }
        }

        start++;
    }

    for(i = blen-1; i >= 0; --i)
    {
        bit1to0_total += bit1to0[i];
        bit0to1_total += bit0to1[i];
    }
    if((bit1to0_total + bit0to1_total) > 0)
    {
        printk("          1->0    0->1\n");
        for(i = blen-1; i >= 0; --i)
        {
            // if true, only 1->0 is possible
            if(pattern & (1<<i))
                printk("bit %2d:%8u\n", i, bit1to0[i]);
            else
                printk("bit %2d:        %8u\n", i, bit0to1[i]);
        }

        printk("Total :%8u%8u\n", bit1to0_total, bit0to1_total);
    }
    return bit1to0_total + bit0to1_total;
}

uint32_t compare_memory_pattern_uint16(const uint16_t *start, int len, uint16_t pattern)
{
    int i;
    uint16_t xor;
    const uint8_t blen = 16;
    uint8_t bb[16] = {0};
    uint32_t bit1to0[16] = {0};
    uint32_t bit0to1[16] = {0};
    uint32_t bit1to0_total = 0;
    uint32_t bit0to1_total = 0;

    for(i = 0; i< blen; ++i)
    {
        if(pattern & (1 << i))
            bb[i] = 1;
        else
            bb[i] = 0;
    }

    while(len--)
    {
        xor = (*start) ^ pattern;
        if(xor != 0)
        {
            for(i = 0; i< blen; ++i)
            {
                if(xor & (1 << i))
                {
                   if(bb[i])
                       bit1to0[i]++;
                   else
                       bit0to1[i]++;
                }
            }
        }

        start++;
    }

    for(i = blen-1; i >= 0; --i)
    {
        bit1to0_total += bit1to0[i];
        bit0to1_total += bit0to1[i];
    }
    if((bit1to0_total + bit0to1_total) > 0)
    {
        printk("          1->0    0->1\n");
        for(i = blen-1; i >= 0; --i)
        {
            // if true, only 1->0 is possible
            if(pattern & (1<<i))
                printk("bit %2d:%8u\n", i, bit1to0[i]);
            else
                printk("bit %2d:        %8u\n", i, bit0to1[i]);
        }

        printk("Total :%8u%8u\n", bit1to0_total, bit0to1_total);
    }
    return bit1to0_total + bit0to1_total;
}

uint32_t compare_memory_pattern_uint32(const uint32_t *start, int len, uint32_t pattern)
{
    int i;
    uint32_t xor;
    uint8_t blen = 32;
    uint8_t bb[32] = {0};
    uint32_t bit1to0[32] = {0};
    uint32_t bit0to1[32] = {0};
    uint32_t bit1to0_total = 0;
    uint32_t bit0to1_total = 0;

    for(i = 0; i< blen; ++i)
    {
        if(pattern & (1 << i))
            bb[i] = 1;
        else
            bb[i] = 0;
    }

    while(len--)
    {
        xor = (*start) ^ pattern;
        if(xor != 0)
        {
            for(i = 0; i< blen; ++i)
            {
                if(xor & (1 << i))
                {
                   if(bb[i])
                       bit1to0[i]++;
                   else
                       bit0to1[i]++;
                }
            }
        }

        start++;
    }

    for(i = blen-1; i >= 0; --i)
    {
        bit1to0_total += bit1to0[i];
        bit0to1_total += bit0to1[i];
    }
    if((bit1to0_total + bit0to1_total) > 0)
    {
        printk("          1->0    0->1\n");
        for(i = blen-1; i >= 0; --i)
        {
            // if true, only 1->0 is possible
            if(pattern & (1<<i))
                printk("bit %2d:%8u\n", i, bit1to0[i]);
            else
                printk("bit %2d:        %8u\n", i, bit0to1[i]);
        }

        printk("Total :%8u%8u\n", bit1to0_total, bit0to1_total);
    }
    return bit1to0_total + bit0to1_total;
}
uint32_t compare_memory_uint8(const uint8_t *start1, const uint8_t *start2, int len)
{
    int i;
    uint32_t res = 0;
    for(i = 0; i < len; i++)
    {
        if((*start1) != (*start2))
            res++;

        start1++;
        start2++;
    }
    return res;
}

uint32_t compare_memory_uint16(const uint16_t *start1, const uint16_t *start2, int len)
{
    int i;
    uint32_t res = 0;
    for(i = 0; i < len; i++)
    {
        if((*start1) != (*start2))
            res++;

        start1++;
        start2++;
    }
    return res;
}

uint32_t compare_memory_uint32(const uint32_t *start1, const uint32_t *start2, int len)
{
    int i;
    uint32_t res = 0;
    for(i = 0; i < len; i++)
    {
        if((*start1) != (*start2))
            res++;

        start1++;
        start2++;
    }
    return res;
}

void system_sw_reset()
{
}

void checksum(uint32_t address, uint32_t length) {
	uint32_t *str_address = address;
	uint32_t *end_address = address + length;
	uint32_t c_temp = 0;
	uint32_t checksum = 0;
	uint32_t data = 0;
	uint32_t temp = 0;
	uint32_t mdata = 0;
	int i = 0;

	printk("str_address for checksum:%x\n", str_address);
	printk("end_address for checksum:%x\n", end_address);

	while(str_address < end_address) {
		data = *str_address;
		temp = (0x000000ff & data) << 16;
		mdata = temp;
		temp = (0x0000ff00 & data) << 16;
		mdata = mdata | temp;
		temp = (0x00ff0000 & data) >> 16;
		mdata = mdata | temp;
		temp = (0xff000000 & data) >> 16;
		mdata = mdata | temp;
		if(i<=1) {
			printk("data:%x, mdata:%x\n", data, mdata);
		}
		c_temp = c_temp ^ mdata;
		c_temp = c_temp <<1 | (c_temp>>31);
		c_temp &= 0xffffffff;
		str_address += 1;
		i++;
	}
	checksum = c_temp;
	printk("checksum:%x\n", checksum);

}
