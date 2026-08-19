/*
 * task_spi.c
 *
 *  Created on: 2019�~5��30��
 *      Author: kusoyao
 */

//#include "FreeRTOS.h"
//#include "os_task.h"
//#include "os_semphr.h"
//
//#include "HL_sys_common.h"
//#include "HL_sys_dma.h"
//#include "HL_mibspi.h"
//#include "HL_spi.h"
//
//#include <assert.h>
//
//#include <task_spi.h>
//#include <user_spi.h>
//#include <user_sci.h>
//#include <user_dma.h>
//#include <utils.h>
//
//#include <global.h>
//#include "UART_API.h"
//#include "jpeghandle.h"
//#include "task_tk2_sicd.h"
//
///* Addresses of SPI 16-bit TX/Rx data */
//#if ((__little_endian__ == 1) || (__LITTLE_ENDIAN__ == 1))
//
//#define MIBSPI1_TX_ADDR ((uint32_t)(&(mibspiRAM1->tx[0].data)))
//#define MIBSPI1_RX_ADDR ((uint32_t)(&(mibspiRAM1->rx[0].data)))
//#define MIBSPI2_TX_ADDR ((uint32_t)(&(mibspiRAM2->tx[0].data)))
//#define MIBSPI2_RX_ADDR ((uint32_t)(&(mibspiRAM2->rx[0].data)))
//#define MIBSPI3_TX_ADDR ((uint32_t)(&(mibspiRAM3->tx[0].data)))
//#define MIBSPI3_RX_ADDR ((uint32_t)(&(mibspiRAM3->rx[0].data)))
//#define MIBSPI4_TX_ADDR ((uint32_t)(&(mibspiRAM4->tx[0].data)))
//#define MIBSPI4_RX_ADDR ((uint32_t)(&(mibspiRAM4->rx[0].data)))
//#define MIBSPI5_TX_ADDR ((uint32_t)(&(mibspiRAM5->tx[0].data)))
//#define MIBSPI5_RX_ADDR ((uint32_t)(&(mibspiRAM5->rx[0].data)))
//
//#define SPI1_TX_ADDR ((uint32_t)(&(spiREG1->DAT1)))
//#define SPI1_RX_ADDR ((uint32_t)(&(spiREG1->BUF)) + 2)
//#define SPI2_TX_ADDR ((uint32_t)(&(spiREG2->DAT1)))
//#define SPI2_RX_ADDR ((uint32_t)(&(spiREG2->BUF)) + 2)
//#define SPI3_TX_ADDR ((uint32_t)(&(spiREG3->DAT1)))
//#define SPI3_RX_ADDR ((uint32_t)(&(spiREG3->BUF)) + 2)
//#define SPI4_TX_ADDR ((uint32_t)(&(spiREG4->DAT1)))
//#define SPI4_RX_ADDR ((uint32_t)(&(spiREG4->BUF)) + 2)
//#define SPI5_TX_ADDR ((uint32_t)(&(spiREG5->DAT1)))
//#define SPI5_RX_ADDR ((uint32_t)(&(spiREG5->BUF)) + 2)
//
//#else
//
//#define MIBSPI1_TX_ADDR ((uint32_t)(&(mibspiRAM1->tx[0].data)))
//#define MIBSPI1_RX_ADDR ((uint32_t)(&(mibspiRAM1->rx[0].data)))
//#define MIBSPI2_TX_ADDR ((uint32_t)(&(mibspiRAM2->tx[0].data)))
//#define MIBSPI2_RX_ADDR ((uint32_t)(&(mibspiRAM2->rx[0].data)))
//#define MIBSPI3_TX_ADDR ((uint32_t)(&(mibspiRAM3->tx[0].data)))
//#define MIBSPI3_RX_ADDR ((uint32_t)(&(mibspiRAM3->rx[0].data)))
//#define MIBSPI4_TX_ADDR ((uint32_t)(&(mibspiRAM4->tx[0].data)))
//#define MIBSPI4_RX_ADDR ((uint32_t)(&(mibspiRAM4->rx[0].data)))
//#define MIBSPI5_TX_ADDR ((uint32_t)(&(mibspiRAM5->tx[0].data)))
//#define MIBSPI5_RX_ADDR ((uint32_t)(&(mibspiRAM5->rx[0].data)))
//
//#define SPI1_TX_ADDR ((uint32_t)(&(spiREG1->DAT1)))
//#define SPI1_RX_ADDR ((uint32_t)(&(spiREG1->BUF)))
//#define SPI2_TX_ADDR ((uint32_t)(&(spiREG2->DAT1)))
//#define SPI2_RX_ADDR ((uint32_t)(&(spiREG2->BUF)))
//#define SPI3_TX_ADDR ((uint32_t)(&(spiREG3->DAT1)))
//#define SPI3_RX_ADDR ((uint32_t)(&(spiREG3->BUF)))
//#define SPI4_TX_ADDR ((uint32_t)(&(spiREG4->DAT1)))
//#define SPI4_RX_ADDR ((uint32_t)(&(spiREG4->BUF)))
//#define SPI5_TX_ADDR ((uint32_t)(&(spiREG5->DAT1)))
//#define SPI5_RX_ADDR ((uint32_t)(&(spiREG5->BUF)))
//#endif
//
//#define NUM_SPI 5
//
//#define show_pattern_mode 0
//
//extern QueueHandle_t queue_DMA_reset;
//extern QueueHandle_t queue_SF2_reset;
//
//static const uint8_t DMA_REQ_SPI[NUM_SPI][16] =
//{
//    //MIBSPI1
//    { DMA_REQ1,  DMA_REQ0,  DMA_REQ4,  DMA_REQ5,  DMA_REQ8,  DMA_REQ9, DMA_REQ12, DMA_REQ13,
//      DMA_REQ16, DMA_REQ17, DMA_REQ22, DMA_REQ23, DMA_REQ26, DMA_REQ27, DMA_REQ30, DMA_REQ31},
//    //MIBSPI2
//    { DMA_REQ3,  DMA_REQ2, DMA_REQ32, DMA_REQ33, DMA_REQ34, DMA_REQ35, DMA_REQ36, DMA_REQ37,
//      DMA_REQ38, DMA_REQ39, DMA_REQ40, DMA_REQ41, DMA_REQ42, DMA_REQ43, DMA_REQ44, DMA_REQ45},
//    //MIBSPI3
//    { DMA_REQ15, DMA_REQ14,  DMA_REQ4,  DMA_REQ5,  DMA_REQ8,  DMA_REQ9, DMA_REQ12, DMA_REQ13,
//      DMA_REQ16, DMA_REQ17, DMA_REQ22, DMA_REQ23, DMA_REQ26, DMA_REQ27, DMA_REQ30, DMA_REQ31},
//    //MIBSPI4
//    { DMA_REQ25, DMA_REQ24, DMA_REQ32, DMA_REQ33, DMA_REQ34, DMA_REQ35, DMA_REQ36, DMA_REQ37,
//      DMA_REQ38, DMA_REQ39, DMA_REQ40, DMA_REQ41, DMA_REQ42, DMA_REQ43, DMA_REQ44, DMA_REQ45},
//    //MIBSPI5
//    { DMA_REQ31, DMA_REQ30,  DMA_REQ6,  DMA_REQ7, DMA_REQ10, DMA_REQ11, DMA_REQ14, DMA_REQ15,
//      DMA_REQ18, DMA_REQ19, DMA_REQ22, DMA_REQ23, DMA_REQ24, DMA_REQ25, DMA_REQ28, DMA_REQ29},
//};
//
//static const uint32_t MIBSPI_TX_ADDR[NUM_SPI] = { MIBSPI1_TX_ADDR, MIBSPI2_TX_ADDR, MIBSPI3_TX_ADDR, MIBSPI4_TX_ADDR, MIBSPI5_TX_ADDR};
//static const uint32_t MIBSPI_RX_ADDR[NUM_SPI] = { MIBSPI1_RX_ADDR, MIBSPI2_RX_ADDR, MIBSPI3_RX_ADDR, MIBSPI4_RX_ADDR, MIBSPI5_RX_ADDR};
//
//static const uint32_t SPI_TX_ADDR[NUM_SPI] = { SPI1_TX_ADDR, SPI2_TX_ADDR, SPI3_TX_ADDR, SPI4_TX_ADDR, SPI5_TX_ADDR};
//static const uint32_t SPI_RX_ADDR[NUM_SPI] = { SPI1_RX_ADDR, SPI2_RX_ADDR, SPI3_RX_ADDR, SPI4_RX_ADDR, SPI5_RX_ADDR};
//
//static const uint8_t DMA_MAP_SPI[NUM_SPI][2] = { { 0 , 1}, { 0 , 1}, { 0 , 1}, { 0 , 1}, { 2 , 3}};
//
//SemaphoreHandle_t sync_sem_btc[NUM_SPI] = {0};
//SemaphoreHandle_t sync_sem_hbc[NUM_SPI] = {0};
//
//uint32_t gdma_channel_cp1[NUM_SPI] = {0};
//uint32_t gdma_channel_cp2[NUM_SPI] = {0};
//
//bool mibspi_slave_dma_is_initialized[NUM_SPI] = {false};
//
//void mibspiDMANotificationBTC(void *parameter);
//void mibspiDMANotificationHBC(void *parameter);
//
//int get_mibspi_tg_size(mibspiBASE_t *spi, int group)
//{
//    uint32 start  = (spi->TGCTRL[group] >> 8U) & 0xFFU;
//    uint32 end    = (group == 7U) ? (((spi->LTGPEND & 0x00007F00U) >> 8U) + 1U) : ((spi->TGCTRL[group+1U] >> 8U) & 0xFFU);
//
//    if (end == 0U) {end = 128U;}
//    return end - start;
//}
//
//int mibspi_slave_dma_init(mibspiBASE_t *spi)
//{
//    //mibspi in slave mode,
//    int index = get_index_mibspi(spi);
//    mibspiRAM_t *ptr_rxram = get_mibspi_ram(spi);
//
//	g_dmaCTRL g_dmaCTRLPKT;    /* dma control packet configuration stack */
//
//    uint32_t group = 0;
//    int i;
//
//    uint32_t dma_channel_cp1;
//    uint32_t dma_channel_cp2;
//
//    int TGSIZE = get_mibspi_tg_size(spi, group);
//    printk("init mibspi%d Transfer Group %d, TGSIZE %d\n", index + 1, group, TGSIZE);
//    if((TGSIZE % 8) != 0)
//    {
//       printk("ERROR! TGSIZE % 8 != 0\n");
//       return -1;
//    }
//
//    // dummy data
//#if 0
//    for(i = 0;i < TGSIZE; ++i) {
//       spi_rx_buffer_top[index][i] = ((index + 1) << 8) | i;
//       spi_rx_buffer_bottom[index][i] = ((index + 1) << 8) | i;
//	}
//#endif
//
//    BaseType_t xRunningPrivileged = prvRaisePrivilege();
//
//    if(mibspi_slave_dma_is_initialized[index])
//    {
//       // reset dma, spi
//       spi->TGCTRL[group] &= ~0x80000000U;
//       dma_channel_cp1 = gdma_channel_cp1[index]; //dma has 32 channel to service assigned request
//       dma_channel_cp2 = gdma_channel_cp2[index];
//       dmaSetChEnable(dma_channel_cp1, DMA_SW);
//       dmaSetChEnable(dma_channel_cp2, DMA_SW);
//    }
//    else
//    {
//       // initialize spi and reset dma
//       sync_sem_hbc[index] = xSemaphoreCreateBinary();
//       ASSERT(sync_sem_hbc[index] != NULL );
//       sync_sem_btc[index] = xSemaphoreCreateBinary();
//       ASSERT(sync_sem_btc[index] != NULL );
//
//       dma_channel_cp1 = gdma_channel_cp1[index] = alloc_dma_channel(DMA_HIGH_PRIORITY); //dma has 32 channel to service assigned request
//       dma_channel_cp2 = gdma_channel_cp2[index] = alloc_dma_channel(DMA_HIGH_PRIORITY);
//       ASSERT(dma_channel_cp1 != -1);
//       ASSERT(dma_channel_cp2 != -1);
//    }
//
//
//    spi->DMACTRL[0] =  (0ul << 31) /* Auto-disable of DMA channel after ICOUNT+1 transfers. */
//                           | ((TGSIZE-1) << 24) /* Buffer utilized to trigger DMA transfer. */
//                           | (DMA_MAP_SPI[index][0] << 20) /* RXDMA_MAPx */
//                           | (0ul << 16) /* TXDMA_MAPx */
//                           | (1ul << 15) /* Receive data DMA channel enable. */
//                           | (0ul << 14) /* Transmit data DMA channel enable. */
//                           | (0ul << 13) /* Non-interleaved DMA block transfer. This bit is available in master mode only. */
//                           | (0ul << 8);/* ICOUNTx */
//
//    spi->DMACTRL[1] =  (0ul << 31) /* Auto-disable of DMA channel after ICOUNT+1 transfers. */
//                           | ((TGSIZE-1) << 24) /* Buffer utilized to trigger DMA transfer. */
//                           | (DMA_MAP_SPI[index][1] << 20) /* RXDMA_MAPx */
//                           | (0ul << 16) /* TXDMA_MAPx */
//                           | (1ul << 15) /* Receive data DMA channel enable. */
//                           | (0ul << 14) /* Transmit data DMA channel enable. */
//                           | (0ul << 13) /* Non-interleaved DMA block transfer. This bit is available in master mode only. */
//                           | (0ul << 8);/* ICOUNTx */
//
//    spi->DMACNTLEN = 0ul;
//
//    /* - setting dma control packets */
//
//    /* Setup DMA Control Packed (structure is part of dma.c) */
//    g_dmaCTRLPKT.SADD      = (uint32_t)(&(ptr_rxram->rx[0].data)); /* initial source address */
//    g_dmaCTRLPKT.DADD      = (uint32_t)(&spi_rx_buffer[0][0]); /* initial destination address top */
//    g_dmaCTRLPKT.CHCTRL    = 0ul; /* channel control */
//    g_dmaCTRLPKT.RDSIZE    = ACCESS_16_BIT; /* read size */
//    g_dmaCTRLPKT.WRSIZE    = ACCESS_64_BIT; /* write size */
//    g_dmaCTRLPKT.ELCNT     = TGSIZE;   /* element count */
//
//	/* frame count = 0x1900( g_dmaCTRLPKT.FRCNT = (sizeof(spi_rx_buffer) / sizeof(spi_rx_buffer[0][0])) / (g_dmaCTRLPKT.ELCNT * 2) ),
//	 * but size of spi_rx_buffer already changed so coludn't use this number. now new requirement is image + id,
//	 * so it needs to change size of FRCNT that is 0x15f9 to 0x15fa , which means every time system could read image size are
//	 * g_dmaCTRLPKT.FRCNT x g_dmaCTRLPKT.ELCNT + 512 bytes
//	 * please note SF2-M3 needs to set 0x30002034 register content to 7f(7f+1=0x80 =128 * 4bytes = 512bytes)
//	 */
//	g_dmaCTRLPKT.FRCNT = 0X1194;
//
//    if(g_dmaCTRLPKT.FRCNT > 0x1FFF)
//    {
//       //IFTCOUNT and IETCOUNT limit to 0x1fff
//       printk("Error! mibspi%d FRCNT(0x%x) > 0x1FFF\n", index + 1, g_dmaCTRLPKT.FRCNT);
//       return -1;
//    }
//    g_dmaCTRLPKT.ELSOFFSET = 4ul; /* element source offset */
//    g_dmaCTRLPKT.FRSOFFSET = 0ul; /* frame source offset */
//    g_dmaCTRLPKT.ELDOFFSET = 8ul; /* element destination offset */
//    g_dmaCTRLPKT.FRDOFFSET = TGSIZE * 2; /* frame destination offset */
//    g_dmaCTRLPKT.PORTASGN  = 4ul; /* port b */
//    g_dmaCTRLPKT.TTYPE     = FRAME_TRANSFER ; /* transfer type */
//    g_dmaCTRLPKT.ADDMODERD = ADDR_OFFSET;     /* address mode read */
//    g_dmaCTRLPKT.ADDMODEWR = ADDR_OFFSET;     /* address mode write */
//    g_dmaCTRLPKT.AUTOINIT  = AUTOINIT_ON;     /* autoinit */
//
//    /* Assign DMA Control Packet to Channel 0 */
//    dmaSetCtrlPacket(dma_channel_cp1, g_dmaCTRLPKT);
//
//    /* Assign DMA Control Packet to Channel 1 */
//    dmaSetCtrlPacket(dma_channel_cp2, g_dmaCTRLPKT);
//
//    /* Setup DMA Control Packed (reuse settings from previous where possible) */
//	g_dmaCTRLPKT.SADD      = (uint32_t)(&(ptr_rxram->rx[TGSIZE/2].data)); /* initial source address */
//    g_dmaCTRLPKT.DADD      = (uint32_t)(&spi_rx_buffer[0][TGSIZE/2]);  /* initial destination  address */
//
//    /* Assign DMA request: channel-0 with request line - 8 */
//    dmaReqAssign(dma_channel_cp1, DMA_REQ_SPI[index][DMA_MAP_SPI[index][0]]);
//    dmaReqAssign(dma_channel_cp2, DMA_REQ_SPI[index][DMA_MAP_SPI[index][1]]);
//
//    /* Set the DMA Channel 0 to trigger on h/w request */
//    dmaSetChEnable(dma_channel_cp1, DMA_HW);
//    dmaSetChEnable(dma_channel_cp2, DMA_HW);
//
//    /* Enable DMA Interrupts on Half Block Complete and Block Transfer Complete */
//    /* If the number of frames n is odd, then the HBC interrupt is generated at the end of the frame when
//    (n+1) / 2 number of frames are left in the block */
//    register_dma_intr(HBC, dma_channel_cp2, mibspiDMANotificationHBC, (void *)spi);
//    register_dma_intr(BTC, dma_channel_cp2, mibspiDMANotificationBTC, (void *)spi);
//    dmaEnableInterrupt(dma_channel_cp2, HBC, DMA_INTA);
//    dmaEnableInterrupt(dma_channel_cp2, BTC, DMA_INTA);
//
//    /* Enable MibSPIP Parallel Pin Feature */
//    if(spi == mibspiREG1)
//       mibspiPmodeSet(mibspiREG1, PMODE_NORMAL, group);
//    if(spi == mibspiREG5)
//       mibspiPmodeSet(mibspiREG5, PMODE_4_DATALINE, group);
//
//    /* Load some dummy data in the MibSPIP TX Buffer */
//    //mibspiSetData(spi, group, &spi_rx_buffer_top[index][0]);
//    //mibspiSetData(spi, group, &spi_rx_buffer_bottom[index][0]);
//    mibspiTransfer(spi, group);
//
//    portRESET_PRIVILEGE( xRunningPrivileged );
//    mibspi_slave_dma_is_initialized[index] = true;
//
//    return 0;
//}
//
//void u16_swap(uint16_t *in, int in_size)
//{
//    int i;
//    for(i=0;(i*2)<in_size;i++)
//    {
//        in[i]= (in[i]>>8) | (in[i]<<8);
//    }
//}
//
//void conv_10b_to_8b(uint8_t *out, uint8_t *in, int in_size)
//{
//    //if byte swap per word, swap them 0 <--> 1 , 2 <-> 3, ....
///*
//    uint8_t tmp;
//    for(i = 0; i < in_size; i += 2)
//    {
//        tmp = in[i + 1];
//        in[i + 1] = in[i];
//        in[i] = tmp;
//    }
//*/
//    // 10bit convert to 8bit, method 1, 266ms
///*
//    int i,o;
//    for(i = 0, o = 0; i < in_size; i += 5, o += 4)
//    {
//        out[o + 0] = (in[i + 0] >> 2) | (in[i + 1] & 0x03) << 6;
//        out[o + 1] = (in[i + 1] >> 4) | (in[i + 2] & 0x0f) << 4;
//        out[o + 2] = (in[i + 2] >> 6) | (in[i + 3] & 0x3f) << 2;
//        out[o + 3] = in[i + 4];
//    }
//*/
//    // 10bit convert to 8bit, method 2, 80 ms
//    int i;
//    volatile uint32_t *din = in;
//
//    uint32_t d1, d2, d3, d4, d5;
//
//    for(i = 0; i < in_size; i += 20)
//    {
//        d1 = *din++;
//        d2 = *din++;
//        d3 = *din++;
//        d4 = *din++;
//        d5 = *din++;
//
//        d1 = __rev(d1);
//        d2 = __rev(d2);
//        d3 = __rev(d3);
//        d4 = __rev(d4);
//        d5 = __rev(d5);
//
//        *out++ = (d1 >> 2) & 0xff;
//        *out++ = (d1 >> 12) & 0xff;
//        *out++ = (d1 >> 22) & 0xff;
//
//        *out++ = d2 & 0xff;
//        *out++ = (d2 >> 10) & 0xff;
//        *out++ = (d2 >> 20) & 0xff;
//
//        *out++ = (d3 << 2) & 0xff | (d2 >> 30);
//        *out++ = (d3 >> 8) & 0xff;
//        *out++ = (d3 >> 18) & 0xff;
//
//        *out++ = (d4 << 4) & 0xff | (d3 >> 28);
//        *out++ = (d4 >> 6) & 0xff;
//        *out++ = (d4 >> 16) & 0xff;
//
//        *out++ = (d5 << 6) & 0xff | (d4 >> 26);
//        *out++ = (d5 >> 4) & 0xff;
//        *out++ = (d5 >> 14) & 0xff;
//        *out++ = d5 >> 24;
//    }
//
//    // 10bit convert to 8bit, method 3, .. ms
//    /* TODO
//     *
//     * */
//}
//
//int thumbnail_creat(uint8_t* src,uint8_t* dst )
//{
//    printk("thumbnail_creat\n");
//    uint32_t count_x=0;
//    uint32_t count_y=0;
//    uint32_t thumbnail_pixel=0;
//    //printk("src[0x%x]  dst[0x%x]\n",src,dst);
//
//    for(count_y=0 ; count_y < 200 ; count_y++ ){
//        if((count_y%10) == 0){
//            for(count_x=0 ; count_x < 1920*3 ; count_x++ ){
//                if(((count_x%30) == 0) || ((count_x%30) == 1) || ((count_x%30) == 2)){
//                    *(dst+thumbnail_pixel) = *(src+((1920*3*count_y)+count_x));
//                    thumbnail_pixel++;
//                    //printk("[%d][%d] [0x%x]<==[0x%x]\n",count_y,count_x,(dst+thumbnail_pixel),src+((1920*3*count_y)+count_x));
//                }
//            }
//        }
//    }
//    printk("thumbnail_pixel[%d]\n",thumbnail_pixel);
//}
//
///* for only SF2 reset, OBC needs reset relative parameters */
//void reset_mibspi(int *can_process, mibspiBASE_t *spi, int *index) {
//	mibspiOutofReset(spi);
//	*index = get_index_mibspi(spi);
//	*can_process = 0;
//	memset(spi_rx_buffer, 0, sizeof(spi_rx_buffer));
//	SF2_RESET = 0;
//	printk("task_process reset DMA finished,ready to run SF2 RESET\n");
//}
//
//
//int mibspi_slave_dma_200ms(mibspiBASE_t *spi)
//{
//	vTaskDelay(1000);
//	mibspi_slave_dma_init(spi);
//	int index = get_index_mibspi(spi);
//	//printk("index:%d\n", index);
//	TickType_t timeout = portMAX_DELAY;
//	int32_t action = 0;
//	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//	uint8_t state = 1;
//	init_UART_API();
//	uint32_t t1, t2, td;
//
//	while(1) {
//		switch(action) {
//			case 0:
//				printk("case 0:waitting for receiving image(top part of image)\n");
//				if(!xSemaphoreTake(sync_sem_hbc[index], timeout)) {
//					printk("H\n");
//					mibspi_slave_dma_init(spi);
//				}
//				action=1;
//				save_rti_time(&t1);
//			break;
//
//			case 1:
//				printk("case 1:waitting for receiving image(bottom part of image)\n");
//				if(!xSemaphoreTake(sync_sem_btc[index], timeout)) {
//					printk("B\n");
//					mibspi_slave_dma_init(spi);
//				}
//				action=0;
//				save_rti_time(&t2);
//				td = diff_rti(&t2, &t1);
//				td = rti_to_microsecond(td);
//				checksum(0x60000000, 0x119400);
//				//leo add
//				u16_swap((volatile uint32_t *)0x60000000, 1152000);
/////////////////////////////////////////////////////////////////////
///*               uint32_t address=0x60000000;
//                for(;address<(0x60000000+1152000);)
//                {
//                    memset(address+10,0xff,2);
//                    memset(address+30,0x00,2);
//                    memset(address+50,0xff,2);
//                    memset(address+70,0xdd,2);
//                    memset(address+90,0x55,2);
//                    memset(address+95,0xff,2);
//                    address+=100;
//                }
//*/
/////////////////////////////////////////////////////////////////////
//				//smith add  (store jpeg at : uint8_t image_buffer_jpeg[JPG_BUFFER_SIZE])
//				xSemaphoreGive(sem_image_ready);
//
//				//leo add
//				thumbnail_creat((uint8_t *)0x60000000,&image_buffer_thumbnail[transfer_time*11520]);
//
//				printk("case1[%x]: received part_of_image:elapsed time:%d ms\n",transfer_time, td/1000);
//				xSemaphoreGive(xSPIxFinshSemaphore);
//			break;
//		}
//	}
//}
//
//
//
//#if 0
//int mibspi_slave_dma_200ms(mibspiBASE_t *spi)
//{
//	vTaskDelay(1000);
//	mibspi_slave_dma_init(spi);
//	int index = get_index_mibspi(spi);
//	printk("index:%d\n", index);
//	TickType_t timeout = portMAX_DELAY;
//	int32_t action = 0;
//	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//	uint8_t state = 1;
//	init_UART_API();
//	uint32_t t1, t2, td;
//
//
//	while(1) {
//		switch(action) {
//			case 0:
//				printk("case 0:waitting for receiving image + trailer\n");
//				if(!xSemaphoreTake(sync_sem_hbc_top[index], timeout)) {
//					printk("H\n");
//					mibspi_slave_dma_init(spi);
//				}
//				printk("case 0:received 1/4 image\n");
//				action=1;
//				save_rti_time(&t1);
//			break;
//
//			case 1:
//				printk("case 1:waitting for receiving 1/4 image + 256bytes\n");
//				if(!xSemaphoreTake(sync_sem_btc_top[index], timeout)) {
//					printk("B\n");
//					mibspi_slave_dma_init(spi);
//				}
//				printk("case 1:received 1/2 image + 256bytes\n");
//				//conv_10b_to_8b(&image_buffers.image_buffer_8b[IMAGE_LINE_COUNT/2][0], &spi_rx_buffer_bottom[IMAGE_LINE_COUNT/2][0], IMAGE_BUFFER_SIZE / 2);
//				action=2;
//			break;
//
//			case 2:
//				printk("case 2:waitting for receiving 3/4 image - 256bytes\n");
//				if(!xSemaphoreTake(sync_sem_hbc_bottom[index], timeout)) {
//					printk("H\n");
//					mibspi_slave_dma_init(spi);
//				}
//				printk("case 2:received 3/4 image - 256bytes\n");
//				action=3;
//			break;
//
//			case 3:
//				printk("case 3:waitting for receiving 4/4 image + 512bytes(trailer)\n");
//				if(!xSemaphoreTake(sync_sem_btc_bottom[index], timeout)) {
//					printk("B\n");
//					mibspi_slave_dma_init(spi);
//				}
//				printk("case 3:received 4/4 image + 512bytes(trailer)\n");
//				//conv_10b_to_8b(&image_buffers.image_buffer_8b[IMAGE_LINE_COUNT/2][0], &spi_rx_buffer_bottom[IMAGE_LINE_COUNT/2][0], IMAGE_BUFFER_SIZE / 2);
//
//                //u16_swap(spi_rx_buffer_top, (1152000 / 2));
//                //u16_swap(spi_rx_buffer_bottom, ((IMAGE_BUFFER_SIZE / 2)));
//                //conv_10b_to_8b(&spi_rx_buffer_top[0][0], &spi_rx_buffer_top[0][0], ((IMAGE_BUFFER_SIZE / 2)));
//                //conv_10b_to_8b(&spi_rx_buffer_bottom[0][0], &spi_rx_buffer_bottom[0][0], ((IMAGE_BUFFER_SIZE / 2)));
//                //status = NANDFLASH_Erase(0, 0, 0x200000);
//                //status = NANDFLASH_WriteSpare(0, 0, 0x60000000, 1152000);
//				action=0;
//				save_rti_time(&t2);
//				td = diff_rti(&t2, &t1);
//				td = rti_to_microsecond(td);
//				printk("case3: received 3/4 image + 512bytes elapsed time:%d ms\n", td/1000);
//			break;
//
//		}
//	}
//}
//#endif
//
//#if 0
//int mibspi_slave_dma(mibspiBASE_t *spi)
//{
//    vTaskDelay(1000);
//    mibspi_slave_dma_init(spi);
//    int index = get_index_mibspi(spi);
//    //BaseType_t xRunningPrivileged = prvRaisePrivilege();
//
//    rti_t t1 = {0}, t2 = {0}, t3 = {0}, t4 = {0}, t5 = {0}, told = {0};
//    uint32_t td;
//    TickType_t timeout = 1000;
//    //TickType_t timeout = portMAX_DELAY;
//
//    int count1 = 0, count2 = 0;
//    volatile int lmode;
//
//
//    uint8_t c = 'T';
//    uint8_t r;
//
//
//    printk("s%d spi_rx_buffer at %x\n", index + 1, spi_rx_buffer);
//    printk("image_buffers.image_buffer_8b[0] at %x size %d\n", image_buffers.image_buffer_8b[0], sizeof(image_buffers.image_buffer_8b[0]));
//    printk("image_buffers.image_buffer_10b[0] at %x size %d\n", image_buffers.image_buffer_10b[0], sizeof(image_buffers.image_buffer_10b[0]));
//    printk("image_buffer_jpeg[0] at %x size %d\n", image_buffer_jpeg[0], sizeof(image_buffer_jpeg[0]));
//
//    save_rti_time(&told);
//    while(1)
//    {
//        while(!xSemaphoreTake(sem_image_processed, 1)) // wait processing thread finish
//        {
//        }
//
//        while(1)
//        {
//            // mode should not change between image frame
//            lmode = gmode;
//
//            while(lmode == 3)
//            {
//                vTaskDelay(1);
//                lmode = gmode;
//            }
//
//            // auto send trigger to star tracker -- move to command line
//            sciPollTx(STARTRACKER_PORT, &c, 1);
//
//            // check reply
//            //sciPollRx(STARTRACKER_PORT, &r, 1);
//            //if(r != '1')
//            //    printk("reply fail r = %c\n", r);
//
//            if(lmode == 0)
//                save_rti_time(&t1);
//
//            ///HBC//
//            //wait interrupt
//            if(xSemaphoreTake(sync_sem_hbc[index], timeout))
//            {
//                // got half of data
//            }
//            else
//            {
//                printk("s%d HBC timeout\n", index + 1);
//                //RESET DMA SPI
//                mibspi_slave_dma_init(spi);
//                continue;
//            }
//
//            if(lmode == 0)
//            {
//                // calculate & print time
//                save_rti_time(&t2);
//                td = diff_rti(&t2, &t1);
//                printk("s%d HBC %uus\n", index + 1, rti_to_microsecond(td));
//            }
//            else if(lmode == 1)
//            {
//                memcpy(&image_buffers.image_buffer_10b[0][0], &spi_rx_buffer[0][0], IMAGE_BUFFER_SIZE / 2);
//            }
//            else if(lmode == 2)
//            {
//                conv_10b_to_8b(&image_buffers.image_buffer_8b[0][0], &spi_rx_buffer[0][0], IMAGE_BUFFER_SIZE / 2);
//            }
//
//            if(lmode == 0)
//            {
//                save_rti_time(&t3);
//                td = diff_rti(&t3, &t2);
//                printk("s%d top half %uus\n", index + 1, rti_to_microsecond(td));
//            }
//
//            ///BTC///
//            //wait interrupt
//            if(xSemaphoreTake(sync_sem_btc[index], timeout))
//            {
//            }
//            else
//            {
//                if(lmode == 0)
//                printk("s%d BTC timeout\n", index + 1);
//                //RESET DMA SPI
//                mibspi_slave_dma_init(spi);
//                continue;
//            }
//
//            if(lmode == 0)
//            {
//                // calculate & print time
//                save_rti_time(&t4);
//                td = diff_rti(&t4, &t3);
//                printk("s%d BTC %uus\n", index + 1, rti_to_microsecond(td));
//            }
//            else if(lmode == 1)
//            {
//                //output 10bit data
//                memcpy(&image_buffers.image_buffer_10b[IMAGE_LINE_COUNT/2][0], &spi_rx_buffer[IMAGE_LINE_COUNT/2][0], IMAGE_BUFFER_SIZE / 2);
//            }
//            else if(lmode == 2)
//            {
//                conv_10b_to_8b(&image_buffers.image_buffer_8b[IMAGE_LINE_COUNT/2][0], &spi_rx_buffer[IMAGE_LINE_COUNT/2][0], IMAGE_BUFFER_SIZE / 2);
//            }
//
//            if(lmode == 0)
//            {
//                save_rti_time(&t5);
//                td = diff_rti(&t5, &t4);
//                printk("s%d bottom half %uus\n", index + 1, rti_to_microsecond(td));
//            }
//            break;
//        }
//        //notice processing thread
//        xSemaphoreGive( sem_image_ready );
//    }
//    //portRESET_PRIVILEGE( xRunningPrivileged );
//}
//#endif
//
//
///* call by DMA interrupt */
//void mibspiDMANotificationHBC(void *parameter)
//{
//    mibspiBASE_t *mibspi = parameter;
//    int index = get_index_mibspi(mibspi);
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    //printk("HBC %x .. %x\n", spi_rx_buffer[index][0], spi_rx_buffer[index][SPI_BUFFER_SIZE-1]);
//    xSemaphoreGiveFromISR(sync_sem_hbc[index], &xHigherPriorityTaskWoken);
//    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//}
//
//void mibspiDMANotificationBTC(void *parameter)
//{
//    mibspiBASE_t *mibspi = parameter;
//    int index = get_index_mibspi(mibspi);
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    xSemaphoreGiveFromISR(sync_sem_btc[index], &xHigherPriorityTaskWoken);
//    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//}
//
//void vTask_spi(void *pvParameters)
//{
//    //mibspi_slave_dma(mibspiREG5);
//
//    mibspi_slave_dma_200ms(mibspiREG1);
//    vTaskDelete(NULL);
//}
