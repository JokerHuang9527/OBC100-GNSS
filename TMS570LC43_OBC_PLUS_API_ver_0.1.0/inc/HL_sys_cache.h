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

#ifndef HL_SYS_CACHE_H_
#define HL_SYS_CACHE_H_

/**
 *  @defgroup HALCoGen_Cache Cache Maintenance Functions
 *  @brief ARM(TM) Cortex(TM)-R5 Cache Maintenance
 *
 *    Related Files
 *   - HL_sys_cache.c

 *  @{
 */

#ifdef CMSIS_COMPATIBILITY
#include "HL_sys_core.h"

/**
 * \ingroup  HALCoGen_Cache
 * \defgroup CMSIS_Core_CacheFunctions CMSIS Style Cache Functions
 * \brief    Function-like macros that provide compatibility to CMSIS functions that configure Instruction and Data cache.
 *
 * @{
 */

/**
  \brief   Enable I-Cache
  \details Turns on I-Cache
  */
#define SCB_EnableICache()                              coreEnableIC()

/**
  \brief   Disable I-Cache
  \details Turns off I-Cache
  */
#define SCB_DisableICache()                             coreDisableIC()

/**
  \brief   Invalidate I-Cache
  \details Invalidates I-Cache
  */
#define SCB_InvalidateICache()                          _iCacheInvalidate_()

/**
  \brief   Enable D-Cache
  \details Turns on D-Cache
  */
#define SCB_EnableDCache()                              coreEnableDC()

/**
  \brief   Disable D-Cache
  \details Turns off D-Cache
  */
#define SCB_DisableDCache()                             coreDisableDC()

/**
  \brief   Invalidate D-Cache
  \details Invalidates D-Cache
  */
#define SCB_InvalidateDCache()                          _dCacheInvalidate_()

/**
  \brief   Clean D-Cache
  \details Cleans D-Cache
  */
#define SCB_CleanDCache()                               coreCleanDC()

/**
  \brief   Clean & Invalidate D-Cache
  \details Cleans and Invalidates D-Cache
  */
#define SCB_CleanInvalidateDCache()                     coreCleanInvalidateDC()

/**
  \brief   D-Cache Invalidate by address
  \details Invalidates D-Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
#define SCB_InvalidateDCache_by_Addr(addr, dsize)       coreInvalidateDCByAddress((uint32)(addr), (dsize))

/**
  \brief   D-Cache Clean by address
  \details Cleans D-Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
#define SCB_CleanDCache_by_Addr (addr, dsize)           coreCleanDCByAddress((uint32)(addr), (dsize))

/**
  \brief   D-Cache Clean and Invalidate by address
  \details Cleans and invalidates D_Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
#define SCB_CleanInvalidateDCache_by_Addr (addr, dsize) coreCleanInvalidateDCByAddress((uint32)(addr), (dsize))

/*@} end of CMSIS_Core_CacheFunctions */
#endif

void coreEnableIC(void);
void coreDisableIC(void);

void coreEnableDC(void);
void coreDisableDC(void);

void coreInvalidateICDCByAddress(uint32 u32Address, uint32 u32Size);
void coreInvalidateICByAddress(uint32 u32Address, uint32 u32Size);
void coreInvalidateDCByAddress(uint32 u32Address, uint32 u32Size);

void coreCleanDC(void);
void coreCleanInvalidateDC(void);

void coreCleanDCByAddress(uint32 u32Address, uint32 u32Size);
void coreCleanInvalidateDCByAddress(uint32 u32Address, uint32 u32Size);

/**@}*/

#endif /* HL_SYS_CACHE_H_ */
