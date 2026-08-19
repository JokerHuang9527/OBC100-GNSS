/**
 * @file   HL_sys_cache.c
 * @author Christian Herget
 * @date   27 Sep 2016
 * @brief  Cortex(TM)-R5 Cache Maintanance Functions
 *
 */

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

/* USER CODE BEGIN (0) */
/* USER CODE END */

#include "HL_hal_stdtypes.h"
#include "HL_sys_cache.h"

/* USER CODE BEGIN (1) */
/* USER CODE END */


/* USER CODE BEGIN (2) */
/* USER CODE END */

/** Constants to access register bits */
#define CORE_SCTLR_I_MASK (1u << 12) /* Enables L1 instruction cache */
#define CORE_SCTLR_C_MASK (1u <<  2) /* Enables L1 data cache */

/** @note Implementation defined constants, these are only valid for RM57Lx and TMS570LC43x */
#define CORE_CACHE_LINE_SIZE_IN_BYTES (32u)
#define CORE_CACHE_LINE_SIZE_MASK     (0x0000001Fu)

#define CORE_CACHE_NUMBER_OF_SETS     (256u)
#define CORE_CACHE_NUMBER_OF_WAYS     (4u)   /* Associativity of cache = 4 */

/* Cache Size = CORE_CACHE_LINE_SIZE_IN_BYTES * CORE_CACHE_NUMBER_OF_SETS * CORE_CACHE_NUMBER_OF_WAYS */
/*              32 Byte                       * 256                       * 4                         = 32kB */

/** Macros for Assembler Functions */
#define CORE_DATA_DATA_SYNCHRONIZATION_BARRIER   __asm(" DSB")
#define CORE_DATA_MEMORY_BARRIER                 __asm(" DMB")
#define CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER __asm(" ISB")

/* USER CODE BEGIN (3) */
/* USER CODE END */

/** Private function prototypes to access CP15 registers */
#if (__TI_COMPILER_VERSION__ >= 15012002)
static uint32 cp15ReadSCTLR(void);
static void   cp15WriteSCTLR(uint32 u32Register);

static void cp15ExecuteICIALLU(void);             /* Invalidate All Instruction Caches */
static void cp15WriteICIMVAU(uint32 u32Address);  /* Invalidate Instruction Cache Line by MVA to PoU */
static void cp15WriteDCIMVAC(uint32 u32Address);  /* Invalidate data cache line by MVA to PoC */
static void cp15WriteDCCMVAC(uint32 u32Address);  /* Clean data cache line by MVA to PoC */
static void cp15WriteDCCSW(uint32 u32Register);   /* Clean data cache line by set/way*/
static void cp15WriteDCCIMVAC(uint32 u32Address); /* Clean and invalidate data cache line by MVA to PoC */
static void cp15WriteDCCISW(uint32 u32Register);  /* Clean and invalidate data cache line by set/way */
static void cp15ExecuteDCIALLU(void);             /* Invalidate all Data Caches */
#else
uint32 cp15ReadSCTLR(void);
void   cp15WriteSCTLR(uint32 u32Register);

void cp15ExecuteICIALLU(void);             /* Invalidate All Instruction Caches */
void cp15WriteICIMVAU(uint32 u32Address);  /* Invalidate Instruction Cache Line by MVA to PoU */
void cp15WriteDCIMVAC(uint32 u32Address);  /* Invalidate data cache line by MVA to PoC */
void cp15WriteDCCMVAC(uint32 u32Address);  /* Clean data cache line by MVA to PoC */
void cp15WriteDCCSW(uint32 u32Register);   /* Clean data cache line by set/way */
void cp15WriteDCCIMVAC(uint32 u32Address); /* Clean and invalidate data cache line by MVA to PoC */
void cp15WriteDCCISW(uint32 u32Register);  /* Clean and invalidate data cache line by set/way */
void cp15ExecuteDCIALLU(void);             /* Invalidate all Data Caches */
#endif

/* USER CODE BEGIN (4) */
/* USER CODE END */

/**  @fn void coreEnableIC(void)
 *   @brief Enable Instruction Cache
 *
 *   Invalidates Instruction Cache, then Turns on Instruction Cache
 *
 */
void coreEnableIC(void)
{
/* USER CODE BEGIN (5) */
/* USER CODE END */

    /* Read System Control Register configuration data */
    uint32 u32SCTLR = cp15ReadSCTLR();

    /* set instruction cache enable bit */
    u32SCTLR |= CORE_SCTLR_I_MASK;

    /* Invalidate entire instruction cache */
    cp15ExecuteICIALLU();

    /* enabled instruction cache */
    cp15WriteSCTLR(u32SCTLR);

    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (6) */
/* USER CODE END */
}

/**  @fn void coreDisableIC(void)
 *   @brief Disable Instruction Cache
 *
 *   Turns off Instruction Cache
 *
 */
void coreDisableIC(void)
{
/* USER CODE BEGIN (7) */
/* USER CODE END */

    /* Read System Control Register configuration data */
    uint32 u32SCTLR = cp15ReadSCTLR();

    /* clear instruction cache enable bit */
    u32SCTLR &= ~(CORE_SCTLR_I_MASK);

    /* disabled instruction cache */
    cp15WriteSCTLR(u32SCTLR);

    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (8) */
/* USER CODE END */
}

/**  @fn void coreEnableDC(void)
 *   @brief Enable Data Cache
 *
 *   Invalidates the Data Cache, then Turns on Data Cache
 *
 */
void coreEnableDC(void)
{
/* USER CODE BEGIN (9) */
/* USER CODE END */

    /* Read System Control Register configuration data */
    uint32 u32SCTLR = cp15ReadSCTLR();

    /* set data cache enable bit */
    u32SCTLR |= CORE_SCTLR_C_MASK;

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;

    /* Invalidate entire data cache */
    cp15ExecuteDCIALLU();

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;

    /* enabled data cache */
    cp15WriteSCTLR(u32SCTLR);

/* USER CODE BEGIN (10) */
/* USER CODE END */
}

/**  @fn void coreDisableDC(void)
 *   @brief Disable Data Cache
 *
 *   Turns off Data Cache, then Cleans and Invalidates the Data Cache
 *
 */
void coreDisableDC(void)
{
/* USER CODE BEGIN (11) */
/* USER CODE END */

    /* Read System Control Register configuration data */
    uint32 u32SCTLR = cp15ReadSCTLR();

    /* clear data cache enable bit */
    u32SCTLR &= ~(CORE_SCTLR_C_MASK);

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;

    /* disable data cache */
    cp15WriteSCTLR(u32SCTLR);

    /* Clean and Invalidate the entire Data Cache */
    coreCleanInvalidateDC();

/* USER CODE BEGIN (12) */
/* USER CODE END */
}

/**  @fn void coreInvalidateICDCByAddress(uint32 u32Address, uint32 u32Size)
 *   @brief Invalidate Instruction and Data Cache Content by Address and Size
 *
 *   @param[in] u32Address:  Start address of the memory range whose content in the caches should be invalidate
 *
 *   @param[in] u32Size:    Size of the memory range to be invalidated
 *
 *   This function will invalidate the content of the instruction and data cache for a given memory range (physical address)
 *
 *   @code{.c}
    coreInvalidateICDCByAddress(0x00000000u, 128u);
@endcode
 *
 */
void coreInvalidateICDCByAddress(uint32 u32Address, uint32 u32Size)
{
/* USER CODE BEGIN (13) */
/* USER CODE END */

    /* Adjust Start Address to align with Cache Line Size (256bit / 32byte) */
    uint32 u32Remainder = u32Address & CORE_CACHE_LINE_SIZE_MASK;
    u32Address            = u32Address & (~CORE_CACHE_LINE_SIZE_MASK);

    /* Adjust Size to align with Cache Line Size (256bit / 32byte) */
    u32Size = u32Size + u32Remainder + CORE_CACHE_LINE_SIZE_MASK;

    /* Convert Size to increments of Cache Line Size */
    uint32 u32CacheLines = u32Size / CORE_CACHE_LINE_SIZE_IN_BYTES;

    uint32 u32I = 0ul;
    for (u32I = 0ul ; u32I < u32CacheLines ; u32I++)
    {
        /* Invalidate Instruction Cache Line */
        cp15WriteICIMVAU(u32Size);

        /* Invalidate Data Cache Line */
        cp15WriteDCIMVAC(u32Address);

        u32Address += CORE_CACHE_LINE_SIZE_IN_BYTES;
    }

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (14) */
/* USER CODE END */
}

/**  @fn void coreInvalidateICByAddress(uint32 u32Address, uint32 u32Size)
 *   @brief Invalidate Instruction Cache Content by Address and Size
 *
 *   @param[in] u32Address:  Start address of the memory range whose content in the instruction cache should be invalidate
 *
 *   @param[in] u32Size:    Size of the memory range to be invalidated
 *
 *   This function will invalidate the content of the instruction cache in for a given memory range (physical address)
 *
 */
void coreInvalidateICByAddress(uint32 u32Address, uint32 u32Size)
{
/* USER CODE BEGIN (15) */
/* USER CODE END */

    /* Adjust Start Address to align with Cache Line Size (256bit / 32byte) */
    uint32 u32Remainder = u32Address & CORE_CACHE_LINE_SIZE_MASK;
    u32Address            = u32Address & (~CORE_CACHE_LINE_SIZE_MASK);

    /* Adjust Size to align with Cache Line Size (256bit / 32byte) */
    u32Size = u32Size + u32Remainder + CORE_CACHE_LINE_SIZE_MASK;

    /* Convert Size to increments of Cache Line Size */
    uint32 u32CacheLines = u32Size / CORE_CACHE_LINE_SIZE_IN_BYTES;

    uint32 u32I = 0ul;
    for (u32I = 0ul ; u32I < u32CacheLines ; u32I++)
    {
        /* Invalidate Instruction Cache Line */
        cp15WriteICIMVAU(u32Size);

        u32Address += CORE_CACHE_LINE_SIZE_IN_BYTES;
    }

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (16) */
/* USER CODE END */
}

/**  @fn void coreInvalidateDCByAddress(uint32 u32Address, uint32 u32Size)
 *   @brief Invalidate Data Cache Content by Address and Size
 *
 *   @param[in] u32Address:  Start address of the memory range whose content in the data cache should be invalidate
 *
 *   @param[in] u32Size:    Size of the memory range to be invalidated
 *
 *   This function will invalidate the content of the data cache in for a given memory range (physical address)
 *
 */
void coreInvalidateDCByAddress(uint32 u32Address, uint32 u32Size)
{
/* USER CODE BEGIN (17) */
/* USER CODE END */

    /* Adjust Start Address to align with Cache Line Size (256bit / 32byte) */
    uint32 u32Remainder = u32Address & CORE_CACHE_LINE_SIZE_MASK;
    u32Address            = u32Address & (~CORE_CACHE_LINE_SIZE_MASK);

    /* Adjust Size to align with Cache Line Size (256bit / 32byte) */
    u32Size = u32Size + u32Remainder + CORE_CACHE_LINE_SIZE_MASK;

    /* Convert Size to increments of Cache Line Size */
    uint32 u32CacheLines = u32Size / CORE_CACHE_LINE_SIZE_IN_BYTES;

    uint32 u32I = 0ul;
    for (u32I = 0ul ; u32I < u32CacheLines ; u32I++)
    {
        /* Invalidate Data Cache Line */
        cp15WriteDCIMVAC(u32Address);

        u32Address += CORE_CACHE_LINE_SIZE_IN_BYTES;
    }

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (18) */
/* USER CODE END */
}

/**  @fn void coreCleanDCByAddress(uint32 u32Address, uint32 u32Size)
 *   @brief Clean Data Cache Content by Address and Size
 *
 *   @param[in] u32Address:  Start address of the memory range whose content in the data cache should be cleaned
 *
 *   @param[in] u32Size:    Size of the memory range to be cleaned
 *
 *   This function will clean the content of the data cache in for a given memory range (physical address)
 *
 */
void coreCleanDCByAddress(uint32 u32Address, uint32 u32Size)
{
/* USER CODE BEGIN (19) */
/* USER CODE END */

    /* Adjust Start Address to align with Cache Line Size (256bit / 32byte) */
    uint32 u32Remainder = u32Address & CORE_CACHE_LINE_SIZE_MASK;
    u32Address            = u32Address & (~CORE_CACHE_LINE_SIZE_MASK);

    /* Adjust Size to align with Cache Line Size (256bit / 32byte) */
    u32Size = u32Size + u32Remainder + CORE_CACHE_LINE_SIZE_MASK;

    /* Convert Size to increments of Cache Line Size */
    uint32 u32CacheLines = u32Size / CORE_CACHE_LINE_SIZE_IN_BYTES;

    uint32 u32I = 0ul;
    for (u32I = 0ul ; u32I < u32CacheLines ; u32I++)
    {
        /* Invalidate Data Cache Line */
        cp15WriteDCCMVAC(u32Address + 1);

        u32Address += CORE_CACHE_LINE_SIZE_IN_BYTES;
    }

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (20) */
/* USER CODE END */
}

/**  @fn void coreCleanInvalidateDCByAddress(uint32 u32Address, uint32 u32Size)
 *   @brief Clean and Invalidate Data Cache Content by Address and Size
 *
 *   @param[in] u32Address:  Start address of the memory range whose content in the data cache should be cleaned and invalidated
 *
 *   @param[in] u32Size:    Size of the memory range to be cleaned and invalidated
 *
 *   This function will clean and invalidate the content of the data cache in for a given memory range (physical address)
 *
 */
void coreCleanInvalidateDCByAddress(uint32 u32Address, uint32 u32Size)
{
/* USER CODE BEGIN (21) */
/* USER CODE END */

    /* Adjust Start Address to align with Cache Line Size (256bit / 32byte) */
    uint32 u32Remainder = u32Address & CORE_CACHE_LINE_SIZE_MASK;
    u32Address            = u32Address & (~CORE_CACHE_LINE_SIZE_MASK);

    /* Adjust Size to align with Cache Line Size (256bit / 32byte) */
    u32Size = u32Size + u32Remainder + CORE_CACHE_LINE_SIZE_MASK;

    /* Convert Size to increments of Cache Line Size */
    uint32 u32CacheLines = u32Size / CORE_CACHE_LINE_SIZE_IN_BYTES;

    uint32 u32I = 0ul;
    for (u32I = 0ul ; u32I < u32CacheLines ; u32I++)
    {
        /* Clean and Invalidate Data Cache Line */
        cp15WriteDCCIMVAC(u32Address);

        u32Address += CORE_CACHE_LINE_SIZE_IN_BYTES;
    }

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (22) */
/* USER CODE END */
}

/**  @fn void coreCleanDC(void)
 *   @brief Clean entire Data Cache
 *
 *   This function will clean the entire data cache
 *
 *   @note This function is not portable and will only work on RM57Lx and TMS570LC43x
 *
 */
void coreCleanDC(void)
{
/* USER CODE BEGIN (23) */
/* USER CODE END */

    uint32 u32NumSets = CORE_CACHE_NUMBER_OF_SETS - 1u;

    /* Loop trough Sets */
#pragma MUST_ITERATE(CORE_CACHE_NUMBER_OF_SETS, CORE_CACHE_NUMBER_OF_SETS);
#pragma UNROLL(CORE_CACHE_NUMBER_OF_SETS);
    do
    {
        uint32 u32NumWays = CORE_CACHE_NUMBER_OF_WAYS - 1u;

        /* Loop trough Ways */
#pragma MUST_ITERATE(CORE_CACHE_NUMBER_OF_WAYS, CORE_CACHE_NUMBER_OF_WAYS);
#pragma UNROLL(CORE_CACHE_NUMBER_OF_WAYS);
        do
        {
            /* factor in the way number and the index number (set) */
            uint32 u32DCCSW = (u32NumWays << 30) | (u32NumSets << 5);

            /* Clean by set/way */
            cp15WriteDCCSW(u32DCCSW);

            /* Decrement the way number */
            u32NumWays--;

        } while (u32NumWays < 0xFFFFFFFFul);

        /* Decrement the set number */
        u32NumSets--;
    } while (u32NumSets < 0xFFFFFFFFul);

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (24) */
/* USER CODE END */
}

/**  @fn void coreCleanInvalidateDC(void)
 *   @brief Clean and Invalidate entire Data Cache
 *
 *   This function will clean and invalidate the entire data cache
 *
 *   @note This function is not portable and will only work on RM57Lx and TMS570LC43x
 *
 */
void coreCleanInvalidateDC(void)
{
/* USER CODE BEGIN (25) */
/* USER CODE END */

    uint32 u32NumSets = CORE_CACHE_NUMBER_OF_SETS - 1u;

    /* Loop trough Sets */
#pragma MUST_ITERATE(CORE_CACHE_NUMBER_OF_SETS, CORE_CACHE_NUMBER_OF_SETS);
#pragma UNROLL(CORE_CACHE_NUMBER_OF_SETS);
    do
    {
        uint32 u32NumWays = CORE_CACHE_NUMBER_OF_WAYS - 1u;

        /* Loop trough Ways */
#pragma MUST_ITERATE(CORE_CACHE_NUMBER_OF_WAYS, CORE_CACHE_NUMBER_OF_WAYS);
#pragma UNROLL(CORE_CACHE_NUMBER_OF_WAYS);
        do
        {
            /* factor in the way number and the index number (set) */
            uint32 u32DCCISW = (u32NumWays << 30) | (u32NumSets << 5);

            /* Clean and invalidate by set/way */
            cp15WriteDCCISW(u32DCCISW);

            /* Decrement the way number */
            u32NumWays--;

        } while (u32NumWays < 0xFFFFFFFFul);

        /* Decrement the set number */
        u32NumSets--;
    } while (u32NumSets < 0xFFFFFFFFul);

    CORE_DATA_DATA_SYNCHRONIZATION_BARRIER;
    CORE_INSTRUCTION_SYNCHRONIZATION_BARRIER;

/* USER CODE BEGIN (26) */
/* USER CODE END */
}



/* USER CODE BEGIN (27) */
/* USER CODE END */

#define CORE_CP15 (15)
#define CORE_C0    (0)
#define CORE_C1    (1)
#define CORE_C5    (5)
#define CORE_C6    (6)
#define CORE_C7    (7)
#define CORE_C10  (10)
#define CORE_C11  (11)
#define CORE_C13  (13)
#define CORE_C14  (14)
#define CORE_C15  (15)
#define CORE_SBZ   (0) /* Should Be Zero */

/* USER CODE BEGIN (28) */
/* USER CODE END */

/** @brief Read SCTLR, System Control register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline uint32 cp15ReadSCTLR(void)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c1  0        c0  0        */
    return __MRC(CORE_CP15, 0, CORE_C1, CORE_C0, 0);
}
#else
    __asm("    .align 4\r\n"
      "    .armfunc cp15ReadSCTLR\r\n"
      "    .state32\r\n"
      "cp15ReadSCTLR:\r\n"
      "    mrc p15, #0, A1, c1, c0, #0\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (29) */
/* USER CODE END */

/** @brief Write SCTLR, System Control register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteSCTLR(uint32 u32Register)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c1  0        c0  0        */
    __MCR(CORE_CP15, 0, u32Register, CORE_C1, CORE_C0, 0);
}
#else
    __asm("    .align 4\r\n"
      "    .armfunc cp15WriteSCTLR\r\n"
      "    .state32\r\n"
      "cp15WriteSCTLR:\r\n"
      "    mcr p15, #0, A1, c1, c0, #0\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (30) */
/* USER CODE END */

/** @brief ICIALLU, Invalidate All Instruction Caches */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15ExecuteICIALLU(void)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c7  0        c5  0        */

    __MCR(CORE_CP15, 0, CORE_SBZ, CORE_C7, CORE_C5, 0);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15ExecuteICIALLU\r\n"
      "    .state32\r\n"
      "cp15ExecuteICIALLU:\r\n"
      "    mov A1, #0\r\n"
      "    mcr p15, #0, A1, c7, c5, #0\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (31) */
/* USER CODE END */

/** @brief Write ICIMVAU, Invalidate Instruction Cache Line by MVA to PoU register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteICIMVAU(uint32 u32Address)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c7  0        c5  1        */

    __MCR(CORE_CP15, 0, u32Address, CORE_C7, CORE_C5, 1);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteICIMVAU\r\n"
      "    .state32\r\n"
      "cp15WriteICIMVAU:\r\n"
      "    mcr p15, #0, A1, c7, c5, #1\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (32) */
/* USER CODE END */

/** @brief Write DCIMVAC, Invalidate data cache by MVA to PoC register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteDCIMVAC(uint32 u32Address)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c7  0        c6  1        */

    __MCR(CORE_CP15, 0, u32Address, CORE_C7, CORE_C6, 1);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteDCIMVAC\r\n"
      "    .state32\r\n"
      "cp15WriteDCIMVAC:\r\n"
      "    mcr p15, #0, A1, c7, c6, #1\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (33) */
/* USER CODE END */

/** @brief Write DCCMVAC, Clean data cache line by MVA to PoC register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteDCCMVAC(uint32 u32Address)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c7  0        c10 1        */

    __MCR(CORE_CP15, 0, u32Address, CORE_C7, CORE_C10, 1);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteDCCMVAC\r\n"
      "    .state32\r\n"
      "cp15WriteDCCMVAC:\r\n"
      "    mcr p15, #0, A1, c7, c10, #1\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (34) */
/* USER CODE END */

/** @brief Write DCCSW, Clean data cache line by set/way register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteDCCSW(uint32 u32Register)
{
    __MCR(CORE_CP15, 0, u32Register, CORE_C7, CORE_C10, 2);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteDCCSW\r\n"
      "    .state32\r\n"
      "cp15WriteDCCSW:\r\n"
      "    mcr p15, #0, A1, c7, c10, #2\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (35) */
/* USER CODE END */

/** @brief Write DCCIMVAC, Clean and invalidate data cache line by MVA to PoC register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteDCCIMVAC(uint32 u32Address)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c7  0        c14 1        */

    __MCR(CORE_CP15, 0, u32Address, CORE_C7, CORE_C14, 1);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteDCCIMVAC\r\n"
      "    .state32\r\n"
      "cp15WriteDCCIMVAC:\r\n"
      "    mcr p15, #0, A1, c7, c14, #1\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (36) */
/* USER CODE END */

/** @brief Write DCCISW, Clean and invalidate data cache line by set/way register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15WriteDCCISW(uint32 u32Register)
{
    __MCR(CORE_CP15, 0, u32Register, CORE_C7, CORE_C14, 2);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15WriteDCCISW\r\n"
      "    .state32\r\n"
      "cp15WriteDCCISW:\r\n"
      "    mcr p15, #0, A1, c7, c14, #2\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (37) */
/* USER CODE END */

/** @brief Write DCIALLU, Invalidate all Data Caches register */
#if (__TI_COMPILER_VERSION__ >= 15012003)
static __inline void cp15ExecuteDCIALLU(void)
{
    /* CRn Opcode_1 CRm Opcode_2 */
    /* c15 0        c5  0        */

    __MCR(CORE_CP15, 0, CORE_SBZ, CORE_C15, CORE_C5, 0);
}
#else
__asm("    .align 4\r\n"
      "    .armfunc cp15ExecuteDCIALLU\r\n"
      "    .state32\r\n"
      "cp15ExecuteDCIALLU:\r\n"
      "    mov A1, #0\r\n"
      "    mcr p15, #0, A1, c15, c5, #0\r\n"
      "    bx lr\r\n"
      "\r\n");
#endif

/* USER CODE BEGIN (38) */
/* USER CODE END */

