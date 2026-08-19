/*
 * dma_spi.h
 *
 *  Created on: 2023¦~12¤ë1¤é
 *      Author: user
 */

#ifndef INC_DMA_SPI_H_
#define INC_DMA_SPI_H_

#include "HL_sys_common.h"
#include "HL_mibspi.h"
#include "HL_sys_dma.h"
#include "global.h"
#include "spi.h"
#include "user_dma.h"

int mibspi_dma_transfer(void *hnd, uint8* txsadd, uint8* rxsadd, uint16 length);
void dmaConfigCtrlPacket(uint32 sadd,uint32 dadd,uint32 dsize);
void dmaConfigCtrlTxPacket(uint32 sadd,uint32 dadd,uint16 ElmntCnt, uint16 FrameCnt);
void dmaConfigCtrlRxPacket(uint32 sadd,uint32 dadd,uint16 ElmntCnt, uint16 FrameCnt);
void mibspiDmaConfig(mibspiBASE_t *mibspi, uint32 channel, uint32 txchannel, uint32 rxchannel, uint16 tgPSTART);
int get_mibspi_tg_size(mibspiBASE_t *spi, int group);
void spiDMANotificationRX(void *parameter);
#endif /* INC_DMA_SPI_H_ */
