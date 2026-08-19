/*----------------------------------------------------------------------------*/
/* sys_link_freeRTOS.cmd                                                      */
/*                                                                            */
/* 
* Copyright (C) 2009-2016 Texas Instruments Incorporated - www.ti.com  
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/*                                                                            */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN (0) */
/* USER CODE END */
/*----------------------------------------------------------------------------*/
/* Linker Settings                                                            */

--retain="*(.intvecs)"

/* USER CODE BEGIN (1) */
/* USER CODE END */

/*----------------------------------------------------------------------------*/
/* Memory Map                                                                 */

MEMORY
{
    VECTORS (X)  : origin=0x00000000 length=0x00000020
    KERNEL  (RX) : origin=0x00000020 length=0x00008000 /*fill=0xAA00FF55*/
    FLASH0  (RX) : origin=0x00008020 length=0x001F7FE0 /*fill=0xAA00FF55*/
    FLASH1  (RX) : origin=0x00200000 length=0x00200000 /*fill=0xAA00FF55*/
    FLASH7   (R) : origin=0xF0200000 length=0x00020000 /*fill=0xffffffff*/ /* Bank 7 (128kB, FEE) */
    STACKS  (RW) : origin=0x08000000 length=0x00006000
    KRAM    (RW) : origin=0x08006000 length=0x00000800
    RAM     (RW) : origin=(0x08006000+0x00000800) length=(0x0007a000 - 0x00000800)
    
/* USER CODE BEGIN (2) */
    SRAM1   (RWX): origin=0x60000000 length=0x200000
    SRAM2   (RWX): origin=0x64000000 length=0x200000
    SRAM3   (RWX): origin=0x68000000 length=0x200000
/* USER CODE END */
}

/* USER CODE BEGIN (3) */
/* USER CODE END */

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */

SECTIONS
{
    .intvecs : {} > VECTORS
    /* FreeRTOS Kernel in protected region of Flash */
    .kernelTEXT  palign(32) : {} > KERNEL
    .cinit       palign(32) : {} > KERNEL
    .pinit       palign(32) : {} > KERNEL
    /* Rest of code to user mode flash region */
    .text        palign(32) : {} > FLASH0 | FLASH1
    .const       palign(32) : {} > FLASH0 | FLASH1
    /* FreeRTOS Kernel data in protected region of RAM */
    .kernelBSS    : {} > KRAM
    .kernelHEAP   : {} > RAM
    .bss          : {} > RAM
    .data         : {} > RAM    

/* USER CODE BEGIN (4) */
    .sram1_section : RUN = SRAM1, LOAD = FLASH0 | FLASH1
    				 LOAD_START(SRAM1LoadStart), LOAD_END(SRAM1LoadEnd), LOAD_SIZE(SRAM1Size),
    				 RUN_START(SRAM1StartAddr), RUN_END(SRAM1EndAddr)
    .sram2_section : RUN = SRAM2, LOAD = FLASH0 | FLASH1
    				 LOAD_START(SRAM2LoadStart), LOAD_END(SRAM2LoadEnd), LOAD_SIZE(SRAM2Size),
    				 RUN_START(SRAM2StartAddr), RUN_END(SRAM2EndAddr)
	.sram3_section : RUN = SRAM3, LOAD = FLASH0 | FLASH1
    				 LOAD_START(SRAM3LoadStart), LOAD_END(SRAM3LoadEnd), LOAD_SIZE(SRAM3Size),
    				 RUN_START(SRAM3StartAddr), RUN_END(SRAM3EndAddr)

/* USER CODE END */
}

/* USER CODE BEGIN (5) */
/* USER CODE END */

/*----------------------------------------------------------------------------*/
/* Misc                                                                       */

/* USER CODE BEGIN (6) */
/* USER CODE END */

/*----------------------------------------------------------------------------*/
