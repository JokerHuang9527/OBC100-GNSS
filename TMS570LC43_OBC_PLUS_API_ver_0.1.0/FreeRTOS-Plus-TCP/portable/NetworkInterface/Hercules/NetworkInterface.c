/** ***************************************************************************************************
 * @file NetworkInterface.c
 * @author Lovas Szilard <lovas.szilard@gmail.com>
 * @date 2017.04.18
 * @version 0.4
 * @copyright Lovas Szilard
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 *
 * This code is made as a part of a research, that has been supported by the ighly industrialized region on the west part of
 * Hungary with limited R&D capacity: Research and development programs related to
 * strengthening the strategic futureoriented industries manufacturing technologies and
 * products of regional competences carried out in comprehensive collaboration� program of
 * the National Research, Development and Innovation Fund (NKFI), Hungary, Grant. No. VKSZ_12-1-2013-0038.
 *
 * A k鏚 elk廥z癃廥彋 a Nemzeti Kutat嫳i, Fejleszt廥i 廥 Innov塶i鏀 Alap t嫥ogat嫳嫛al megval鏀ul�
 * VKSZ_12-1-2013-0038:"Strat嶲iai ipari 墔azatok j饘騸emutat� gy嫫t嫳i technol鏬i壾hoz 廥
 * term幧eihez kapcsol鏚� t廨s嶲i kutat嫳i kompetenci壾k meger鰆癃廥e sz幨esk顤�
 * egytm鐰廥ben megval鏀癃ott kutat嫳-fejleszt廥i programmal" projekt t嫥ogatta.
 *
 * Homepage: http://jkk.sze.hu/fooldal
 *
 * @brief FreeRTOS-Plus-TCP NetworkInterface.c port for Texas Instruments TMS570LC4357 microcontroller.
 *
 * @details
 * Dependencies:
 * -------------
 * - FreeRTOS Labs 160112
 * - Modified HALCoGen 04.04.00
 *
 * Changelog:
 * ----------
 * 2016.02.22
 * Initial release
 * 2016.04.15
 * Using vTaskNotifyGiveFromISR/ulTaskNotifyTake for signaling  EMAC RX interrupt
 * 2016.04.25
 * Doxygen style documentation added.
 * 2016.06.19
 * The new TX method uses IRQ, and blocking (it can do about 95 Mbit/sec iperf).
 * 2017.04.18
 * - Bugfix in uint32 xFreeRTOSEMACHWInit(uint8_t macaddr[6U]) (multiple calling of prvEmacDMAInit(hdkif) in the case of cable not connected)
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

/* HALCoGen generated header files */
#include "HL_emac.h"
#include "HL_mdio.h"
#include "HL_phy_dp83640.h"
#include "HL_sys_vim.h"
#include "HL_gio.h"
#include "HL_reg_het.h"
#include "HL_sys_cache.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"

/* HALCoGen generated source. */
#include "HL_emac.c"

#define EMAC_INT_CORE0_RX_THRESH		(0x0U)		// Acknowledge C0RXTHRESH Interrupt
#define EMAC_INT_CORE0_MISC				(0x3U)		// Acknowledge C0MISC Interrupt (STATPEND, HOSTPEND, MDIO LINKINT0, MDIO USERINT0)
#define EMAC_INT_CORE1_RX_THRESH		(0x4U)		// Acknowledge C1RXTHRESH Interrupt
#define EMAC_INT_CORE1_MISC				(0x7U)		// Acknowledge C1MISC Interrupt (STATPEND, HOSTPEND, MDIO LINKINT0, MDIO USERINT0)
#define EMAC_INT_CORE2_RX_THRESH		(0x8U)		// Acknowledge C2RXTHRESH Interrupt
#define EMAC_INT_CORE2_MISC				(0xbU)		// Acknowledge C2MISC Interrupt (STATPEND, HOSTPEND, MDIO LINKINT0, MDIO USERINT0)

/* Missing defines in HL_hw_emac_ctrl.h.. */
/* A HL_hw_emac_ctrl.h-b鏊 hi嫕yz� #define-ok.. */
#define EMAC_CTRL_C0MISEN_USERINT0EN 	(0x00000001U)
#define EMAC_CTRL_C0MISEN_USERINT0EN_SHIFT (0x00000000U)
#define EMAC_CTRL_C0MISEN_LINKINT0EN 	(0x00000002U)
#define EMAC_CTRL_C0MISEN_LINKINT0EN_SHIFT (0x000000001)
#define EMAC_CTRL_C0MISEN_HOSTPENDEN 	(0x00000004U)
#define EMAC_CTRL_C0MISEN_HOSTPENDEN_SHIFT (0x000000002)
#define EMAC_CTRL_C0MISEN_STATPENDEN 	(0x00000008U)
#define EMAC_CTRL_C0MISEN_STATPENDEN_SHIFT (0x000000003)

/* RXINTSTATRAW regiszter threshold pending bitek */
#define RXnTHRESHPEND_BIT(n)		    (1 << (8 + n))

/* EMAC related VIM channels */
/* EMAC-hoz kapcsol鏚� VIM csatorn嫜 */
#define C0_MISC_PULSE					76U
#define C0_TX_PULSE						77U
#define C0_THRSH_PULSE					78U
#define C0_RX_PULSE						79U

/* Maximum number of retries for reading PHY ID */
/* Legfeljebb ennyiszer pr鏏嫮kozik a PHY ID kiolvas嫳嫛al */
#define PHY_INIT_ID_READ_MAX_RETRIES	0xffff

/* Number of TX and RX DMA buffers */
/* Tx 廥 RX EMAC DMA pufferek sz嫥a */
#define EMAC_TXDMA_PBUF_START_ADDRESS	(EMAC_CTRL_RAM_BASE)
//#define EMAC_TXDMA_PBUF_ALLOC 			((SIZE_EMAC_CTRL_RAM / 2) / sizeof(emac_tx_bd_t))
#define EMAC_TXDMA_PBUF_ALLOC 			(3)
#define EMAC_RXDMA_PBUF_START_ADDRESS	(EMAC_CTRL_RAM_BASE + (SIZE_EMAC_CTRL_RAM / 2))
#define EMAC_RXDMA_PBUF_ALLOC			(MAX_RX_PBUF_ALLOC)				/* HALCoGen GUI 廨t幧 */

/* EMAC descriptor flags TX+RX */
/* EMAC descriptor flag-ek TX+RX */
#define EMAC_DSC_FLAG_SOP 				0x80000000u
#define EMAC_DSC_FLAG_EOP 				0x40000000u
#define EMAC_DSC_FLAG_OWNER 			0x20000000u
#define EMAC_DSC_FLAG_EOQ 				0x10000000u
#define EMAC_DSC_FLAG_TDOWNCMPLT 		0x08000000u
#define EMAC_DSC_FLAG_PASSCRC 			0x04000000u
/* EMAC descriptor flags only RX */
/* EMAC descriptor flag-ek csak RX */
#define EMAC_DSC_FLAG_JABBER 			0x02000000u
#define EMAC_DSC_FLAG_OVERSIZE 			0x01000000u
#define EMAC_DSC_FLAG_FRAGMENT 			0x00800000u
#define EMAC_DSC_FLAG_UNDERSIZED 		0x00400000u
#define EMAC_DSC_FLAG_CONTROL 			0x00200000u
#define EMAC_DSC_FLAG_OVERRUN 			0x00100000u
#define EMAC_DSC_FLAG_CODEERROR 		0x00080000u
#define EMAC_DSC_FLAG_ALIGNERROR 		0x00040000u
#define EMAC_DSC_FLAG_CRCERROR			0x00020000u
#define EMAC_DSC_FLAG_NOMATCH 			0x00010000u

/* Byte swap macro */
#define BYTE_SWAP(x)	__rev(x)
//#define BYTE_SWAP(x)	EMACSwizzleData(x)

/* Interrupt macros */
/* Interrupt makr鏦 */
#ifndef traceEMAC_INT_CORE0_RX_THRESH
	#define traceEMAC_INT_CORE0_RX_THRESH()
#endif

#ifndef traceEMAC_INT_CORE0_MISC_LINK_STATUS
	#define traceEMAC_INT_CORE0_MISC_LINK_STATUS()
#endif

#ifndef traceEMAC_INT_CORE0_MISC
	#define traceEMAC_INT_CORE0_MISC()
#endif

#ifndef traceEMAC_INT_CORE0_MISC_USER_COMMAND
	#define traceEMAC_INT_CORE0_MISC_USER_COMMAND()
#endif

#ifndef traceEMAC_INT_CORE0_MISC_HOST
	#define traceEMAC_INT_CORE0_MISC_HOST()
#endif

#ifndef traceEMAC_INT_CORE0_MISC_STAT
	#define traceEMAC_INT_CORE0_MISC_STAT()
#endif

#ifndef traceEMAC_INT_CORE0_TX
	#define traceEMAC_INT_CORE0_TX()
#endif

/* Minimum ethernet frame size without Frame Check Sequence */
/* Minim嫮is ethernet csomag m廨et a Frame Check Sequence n幨k */
#define MIN_ETHERNET_PACKET_SIZE		(60)

#define _CPU_TMS570LS4357_

void vFreeRTOSEMACMiscInterrupt(void);
void vFreeRTOSEMACTxInterrupt(void);
void vFreeRTOSEMACRxThrshInterrupt(void);
void vFreeRTOSEMACRxInterrupt(void);
uint32 xFreeRTOSEMACHWInit(uint8_t macaddr[6U]);
static void prvEmacRxTask(void *pvParameters);
static void prvEmacDMAInit(hdkif_t *hdkif);
static void prvDisableEMACInterrupts(void);
static void prvEnableEMACInterrupts(void);

static BaseType_t xEMACDriverLoggingLevel = 0;

SemaphoreHandle_t xEMACMiscEventSemaphore = NULL;			/* Link, User, Stat, Host esem幯yek szemafor */
volatile unsigned int xEMACMiscEventBit = pdFALSE;			/* Link, User, Stat, Host esem幯yek jelz騸it */

static xTaskHandle prvEmacRxTaskHandle = NULL;
extern TaskHandle_t xIPTaskHandle;
SemaphoreHandle_t xEMACTxEventSemaphore = NULL;

extern BaseType_t xEMACRxEventSemaphoreFulls;
void _dcacheCleanRange_(unsigned int startAddress, unsigned int endAddress)
{
    coreCleanDCByAddress(startAddress, endAddress - startAddress);
}

void _dcacheInvalidateRange_(unsigned int startAddress, unsigned int endAddress)
{
    coreInvalidateDCByAddress(startAddress, endAddress - startAddress);
}

/* The MAC address defined in HL_sys_main.c */
extern uint8 emacAddress[6U];
extern hdkif_t hdkif_data[];

/* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
driver will filter incoming packets and only pass the stack those packets it
considers need processing. */
#if(ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0)
	#define ipCONSIDER_FRAME_FOR_PROCESSING(pucEthernetBuffer) eProcessBuffer
#else
	#define ipCONSIDER_FRAME_FOR_PROCESSING(pucEthernetBuffer) eConsiderFrameForProcessing((pucEthernetBuffer))
#endif


/** ***************************************************************************************************
 * @fn		static void prvDisableEMACInterrupts(void)
 * @brief	Disable EMAC module interrupts in VIM.
 */
static void prvDisableEMACInterrupts(void)
{
	vimREG->REQMASKCLR2 = (uint32)1U << (C0_MISC_PULSE-64U) | (uint32)1U << (C0_TX_PULSE-64U) | (uint32)1U << (C0_THRSH_PULSE-64U) | (uint32)1U << (C0_RX_PULSE-64U);
}

/** ***************************************************************************************************
 * @fn		static void prvEnableEMACInterrupts(void)
 * @brief	Enable EMAC module interrupts in VIM.
 */
static void prvEnableEMACInterrupts(void)
{
	vimREG->REQMASKSET2 = (uint32)1U << (C0_MISC_PULSE-64U) | (uint32)1U << (C0_TX_PULSE-64U) | (uint32)1U << (C0_THRSH_PULSE-64U) | (uint32)1U << (C0_RX_PULSE-64U);
}

/** ***************************************************************************************************
 * @fn		BaseType_t xNetworkInterfaceInitialise(void)
 * @brief	High level function for initializing EMAC module for sending and receiving ethernet frames.
 * 			- Redirects ISR vectors
 * 			- Calls the low level EMAC HW init function.
 * 			- Creates the necessary task/semaphore(s)
 * @return	pdFAIL Error
 * 			pdPASS Success
 */
BaseType_t xNetworkInterfaceInitialise(void)
{
	BaseType_t xReturn = pdFAIL;
	hdkif_t *hdkif = &hdkif_data[0U];

	/* Disable all EMAC interrupts in VIM. */
	/* Az 飉szes EMAC interrupt letilt嫳a  a VIM-ben. */
	prvDisableEMACInterrupts();

    /* EMAC RX 廥 TX tilt嫳a */
    HWREG(EMAC_0_BASE + EMAC_RXCONTROL) = EMAC_RXCONTROL_RXDIS;
    HWREG(EMAC_0_BASE + EMAC_TXCONTROL) = EMAC_TXCONTROL_TXDIS;

	/* Redirect interrupt vectors to the */
    /* Interrupt vektorok 嫢ir嫕y癃嫳a a saj嫢 ISR fgv幯yekre */
	vimChannelMap(C0_MISC_PULSE, C0_MISC_PULSE, &vFreeRTOSEMACMiscInterrupt);
	vimChannelMap(C0_TX_PULSE, C0_TX_PULSE, &vFreeRTOSEMACTxInterrupt);
	vimChannelMap(C0_THRSH_PULSE, C0_THRSH_PULSE, &vFreeRTOSEMACRxThrshInterrupt);
	vimChannelMap(C0_RX_PULSE, C0_RX_PULSE, &vFreeRTOSEMACRxInterrupt);

	/* Megh癉juk az EMAC init fgv幯y彋  */
	// initial ethernet consume a lot of cpu power, user is not always require network, so we don't want to retry and return success here.
	// if user want network and connect cable later, PHY will auto do AutoNegotiate then network is working fine.
	xFreeRTOSEMACHWInit(emacAddress);
	//if(xFreeRTOSEMACHWInit(emacAddress) != EMAC_ERR_OK)xReturn = pdFAIL;
	//else
	{
		/* Els� inicializ嫮嫳kor L彋rehozzuk a szs嶲es RX taszkot 廥 a kapcsol鏚� szemaforokat */
		if(xEMACMiscEventSemaphore == NULL)
		{
			xEMACMiscEventSemaphore = xSemaphoreCreateBinary();
			configASSERT(xEMACMiscEventSemaphore);
		}
		if(xEMACTxEventSemaphore == NULL)
		{
			xEMACTxEventSemaphore = xSemaphoreCreateBinary();
			configASSERT(xEMACTxEventSemaphore);
		}
		if(prvEmacRxTaskHandle == NULL)
		{
			/* Az _dCacheInvalidateRange_() miatt kell privilegiz嫮t m鏚ban futtatni */
			xTaskCreate(prvEmacRxTask, "EmacRx", ipconfigETHERNET_DRIVER_RX_TASK_STACK_SIZE_WORDS, NULL, ipconfigETHERNET_DRIVER_RX_TASK_PRIORITY, &prvEmacRxTaskHandle);
			configASSERT(prvEmacRxTaskHandle);
		}

		/* Minden szs嶲es taszk 廥 szemafor rendben l彋rej飆t */
		if(xEMACMiscEventSemaphore != NULL && prvEmacRxTaskHandle != NULL)
		{
			/* IRQ enged幨yez廥ek */
			/* A MISC interrupt-ok (Link, HOST, STAT) enged幨yez廥e */
			HWREG(EMAC_CTRL_0_BASE + EMAC_CTRL_CnMISCEN(0)) = EMAC_CTRL_C0MISEN_LINKINT0EN | EMAC_CTRL_C0MISEN_HOSTPENDEN | EMAC_CTRL_C0MISEN_STATPENDEN;
			/* Az 1-es phyaddr c璥� eszk飉z link v嫮toz嫳 monitoroz嫳a */
			HWREG(MDIO_BASE + MDIO_USERPHYSEL0) |= MDIO_USERPHYSEL0_LINKINTENB | 0x1U;

			/* Host 廥 Stat megszak癃嫳ok enged幨yez廥e */
			HWREG(EMAC_BASE + EMAC_MACINTMASKSET) = EMAC_MACINTMASKSET_HOSTMASK | EMAC_MACINTMASKSET_STATMASK;

			/* TX 廥 RX megszak癃嫳ok enged幨yez廥e */
			HWREG(hdkif->emac_base + EMAC_TXINTMASKSET) |= ((uint32)1U << EMAC_CHANNELNUMBER);
			HWREG(hdkif->emac_ctrl_base + EMAC_CTRL_CnTXEN(EMAC_CHANNELNUMBER)) |= ((uint32)1U << EMAC_CHANNELNUMBER);
			HWREG(hdkif->emac_base + EMAC_RXINTMASKSET) |= ((uint32)1U << EMAC_CHANNELNUMBER);
			HWREG(hdkif->emac_ctrl_base + EMAC_CTRL_CnRXEN(EMAC_CHANNELNUMBER)) |= ((uint32)1U << EMAC_CHANNELNUMBER);

			/* EMAC RX 廥 TX enged幨yez廥e */
			HWREG(hdkif->emac_base + EMAC_RXCONTROL) = EMAC_RXCONTROL_RXEN;
			HWREG(hdkif->emac_base + EMAC_TXCONTROL) = EMAC_TXCONTROL_TXEN;

			/* Az 飉szes EMAC interrupt enged幨yez廥e a VIM-ben. */
			prvEnableEMACInterrupts();

			/* Csomagok fogad嫳嫕ak ind癃嫳a a HP be嫮l癃嫳嫛al */
			HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER)) = EMAC_RXDMA_PBUF_START_ADDRESS;
			xReturn = pdPASS;
		}
	}

	return xReturn;
}

/** ***************************************************************************************************
 * @fn		BaseType_t xNetworkInterfaceOutput(xNetworkBufferDescriptor_t * const pxDescriptor, BaseType_t xReleaseAfterSend)
 * @brief	Send data over ethernet (EMAC) interface.
 * @param	pxDescriptor pointer to the buffer descriptor
 * @param	xReleaseAfterSend Release the descriptor after send (pdPASS)
 * @return	pdFAIL Error
 * 			pdPASS Success
 */
BaseType_t xNetworkInterfaceOutput(xNetworkBufferDescriptor_t * const pxDescriptor, BaseType_t xReleaseAfterSend)
{
    hdkif_t *hdkif = &hdkif_data[0U];
    uint32 xFlagsPktlen;
    uint16 xTotalLength;
    static emac_tx_bd_t * pxTransmitBufferDescriptor  = (emac_tx_bd_t *)EMAC_TXDMA_PBUF_START_ADDRESS;
    static emac_tx_bd_t * pxLastQueuedBufferDescriptor  = (emac_tx_bd_t *)EMAC_TXDMA_PBUF_START_ADDRESS;

    /* Is the previous transfer done yet? */
    /* Befejez髇鰗t m嫫 az el驆� 嫢vitel? */
    while(EMAC_DSC_FLAG_OWNER == (BYTE_SWAP(pxTransmitBufferDescriptor->flags_pktlen) & EMAC_DSC_FLAG_OWNER))
    {
    	if(xSemaphoreTake(xEMACTxEventSemaphore, ipconfigETHERNET_DRIVER_TX_BLOCK_TIME) == pdFAIL)
    	{
			iptraceWAITING_FOR_TX_DMA_DESCRIPTOR();
    		return(pdFAIL);
    	}
    }

	/* We are going to send the non zero size packets from non zero address only. */
    /* Csak a nem 0 hossz� csomagokat kdj el a nem null c璥r鯷. */
	if(pxDescriptor->xDataLength != 0 && pxDescriptor->pucEthernetBuffer != NULL)
	{
		memcpy((void *)BYTE_SWAP(pxTransmitBufferDescriptor->bufptr), (void *)pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);

		/* Ha a csomag m廨et nem 廨i el a minim嫮is m廨etet, akkor ki kell eg廥z癃eni */
		/* If packet size is less than the minimum, it has to be padded */
		while(pxDescriptor->xDataLength < MIN_ETHERNET_PACKET_SIZE)
		{
			pxDescriptor->xDataLength++;
			*(uint8_t *)(pxDescriptor->pucEthernetBuffer + pxDescriptor->xDataLength) = 0x00;
		}
		_dcacheCleanRange_((uint32)BYTE_SWAP(pxTransmitBufferDescriptor->bufptr),(uint32)BYTE_SWAP(pxTransmitBufferDescriptor->bufptr) + MAX_TRANSFER_UNIT);

		/* Creating new BD. */
		/* 犆 BD l彋rehoz嫳 */
		pxTransmitBufferDescriptor->bufoff_len = BYTE_SWAP(pxDescriptor->xDataLength);
		xTotalLength = pxDescriptor->xDataLength;
		xFlagsPktlen = ((uint32)(xTotalLength) | (EMAC_DSC_FLAG_SOP | EMAC_DSC_FLAG_EOP | EMAC_DSC_FLAG_OWNER));
		pxTransmitBufferDescriptor->flags_pktlen = BYTE_SWAP(xFlagsPktlen);
		pxTransmitBufferDescriptor->next = NULL;

		prvDisableEMACInterrupts();			/* Start of the critcal section. */
		if(HWREG(hdkif->emac_base + EMAC_TXHDP((uint32)EMAC_CHANNELNUMBER)) == NULL)
		{
			/* Elind癃juk az 嫢vitelt az EMAC Tx Hdr DescPtr 甏嫳嫛al... */
			/* Start transmission by writing EMAC Tx Hdr DescPtr, if EMAC is not running... */
			HWREG(hdkif->emac_base + EMAC_TXHDP((uint32)EMAC_CHANNELNUMBER)) = (uint32)(pxTransmitBufferDescriptor);
			pxLastQueuedBufferDescriptor = pxTransmitBufferDescriptor;
		}
		else
		{
			/* ... vagy hozz塻z az  BD-t a lista v嶲廨e. */
			/* ... or append the new BD to the end of the list. */
			pxLastQueuedBufferDescriptor->next = (emac_tx_bd_t *)BYTE_SWAP((uint32_t)pxTransmitBufferDescriptor);
			pxLastQueuedBufferDescriptor = pxTransmitBufferDescriptor;
		}
		prvEnableEMACInterrupts();
		/* End of the critical section. */

		if(pxTransmitBufferDescriptor < (emac_tx_bd_t *)(EMAC_TXDMA_PBUF_START_ADDRESS + EMAC_TXDMA_PBUF_ALLOC))
		{
			pxTransmitBufferDescriptor++;
		}
		else
		{
			pxTransmitBufferDescriptor = (emac_tx_bd_t *)EMAC_TXDMA_PBUF_START_ADDRESS;
		}

		/* Call the standard trace macro to log the send event. */
		iptraceNETWORK_INTERFACE_TRANSMIT();
	}
	else
	{
		/* Try to release the failed network BD */
		/* Megpr鏏嫮juk felszabad癃ani a hib嫳 csomagle甏鏒 */
		xReleaseAfterSend = pdTRUE;
	}

#if(ipconfigZERO_COPY_TX_DRIVER == 0)
	if(xReleaseAfterSend != pdFALSE)
    {
		vReleaseNetworkBufferAndDescriptor(pxDescriptor);
    }
#else
#error ipconfigZERO_COPY_TX_DRIVER not available yet
#endif

    return pdTRUE;
}

/** ***************************************************************************************************
* @fn void vFreeRTOSEMACMiscInterrupt(void)
* @brief Misc Interrupt handler for EMAC in FreeRTOS-Plus-TCP compatibility mode
*/
#pragma INTERRUPT(vFreeRTOSEMACMiscInterrupt, IRQ)
void vFreeRTOSEMACMiscInterrupt(void)
{
    hdkif_t *hdkif = &hdkif_data[0U];

    /* Link Status Change interrupt */
    if((HWREG(MDIO_BASE + MDIO_LINKINTRAW) & MDIO_LINKINTRAW_USERPHY0) == MDIO_LINKINTRAW_USERPHY0)
    {
    	/* Nyugt嫙zuk a megszak癃嫳t */
    	HWREG(MDIO_BASE + MDIO_LINKINTRAW) = MDIO_LINKINTRAW_USERPHY0;
    	traceEMAC_INT_CORE0_MISC_LINK_STATUS();		/* trace macro */
    }

    /* User Command Complete Interrupt */
    if((HWREG(MDIO_BASE + MDIO_USERINTMASKED) & MDIO_LINKINTMASKED_USERPHY0) == MDIO_LINKINTMASKED_USERPHY0)
    {
    	/* Tiltjuk a megszak癃嫳t. Nyugt嫙ni 廥 ra enged幨yezni majd az applik塶i鏮ak kell. */
    	HWREG(MDIO_BASE + MDIO_USERINTMASKCLEAR) = MDIO_USERINTMASKCLEAR_USERACCESS0;
        traceEMAC_INT_CORE0_MISC_USER_COMMAND();	/* trace macro */
    }

    /* HOST interrupt */
	if((HWREG(hdkif->emac_base + EMAC_MACINTSTATMASKED) & EMAC_MACINTSTATMASKED_HOSTPEND) == EMAC_MACINTSTATMASKED_HOSTPEND)
	{
		/* Tiltjuk a megszak癃嫳t. Nyugt嫙ni 廥 ra enged幨yezni majd az applik塶i鏮ak kell. */
		HWREG(hdkif->emac_base + EMAC_MACINTMASKCLEAR) =  EMAC_MACINTSTATMASKED_HOSTPEND;
        traceEMAC_INT_CORE0_MISC_HOST();			/* trace macro */
	}

    /* STAT(istic) interrupt */
	if((HWREG(hdkif->emac_base + EMAC_MACINTSTATMASKED) & EMAC_MACINTSTATMASKED_STATPEND) == EMAC_MACINTSTATMASKED_STATPEND)
	{
		/* Tiltjuk a megszak癃嫳t. Nyugt嫙ni 廥 ra enged幨yezni majd az applik塶i鏮ak kell. */
		HWREG(hdkif->emac_base + EMAC_MACINTMASKCLEAR) =  EMAC_MACINTSTATMASKED_STATPEND;
		traceEMAC_INT_CORE0_MISC_STAT();			/* trace macro */
	}

    traceEMAC_INT_CORE0_MISC();						/* trace macro */

	xEMACMiscEventBit = pdTRUE;						/* Ha a szemafor nem j飆t volna m嶲 l彋re egy biten is jelezz, hogy volt interrupt */

	/* Kdk egy xEMACMiscEventSemaphore-t */
   	if(xEMACMiscEventSemaphore != NULL)
   	{
   		xSemaphoreGiveFromISR(xEMACMiscEventSemaphore, NULL);
   	}

   	EMACCoreIntAck(hdkif->emac_base, (uint32)EMAC_INT_CORE0_MISC);
}

/** ***************************************************************************************************
* @fn void vFreeRTOSEMACTxInterrupt(void)
* @brief TX Interrupt for EMAC in FreeRTOS-Plus-TCP compatibility mode
*/
#pragma INTERRUPT(vFreeRTOSEMACTxInterrupt, IRQ)
void vFreeRTOSEMACTxInterrupt(void)
{
    static hdkif_t *hdkif = &hdkif_data[0U];
    static BaseType_t xHigherPriorityTaskWoken;
    emac_tx_bd_t *pxCurrentBufferDescriptor;

    /* Acknowledge EMAC by writing completion pointer. */
    /* Nyugt嫙zuk az EMAC-nak BD feldolgoz嫳嫢. */
    pxCurrentBufferDescriptor = (emac_tx_bd_t *)HWREG(hdkif->emac_base + EMAC_TXCP(EMAC_CHANNELNUMBER));
    HWREG(hdkif->emac_base + EMAC_TXCP(EMAC_CHANNELNUMBER)) = (uint32_t)pxCurrentBufferDescriptor;

    /* Restart the transmission if there is more packet to transfer. */
    /* 犆raind癃juk az 嫢vitelt, ha van tov墎bi 嫢vitelre v嫫� csomag. */
    if(pxCurrentBufferDescriptor->next != NULL)
    {
    	HWREG(hdkif->emac_base + EMAC_TXHDP((uint32)EMAC_CHANNELNUMBER)) = BYTE_SWAP((uint32)(pxCurrentBufferDescriptor->next));
    }

    if(xIPTaskHandle != NULL && xEMACTxEventSemaphore != NULL)
    {
    	xSemaphoreGiveFromISR(xEMACTxEventSemaphore, &xHigherPriorityTaskWoken);
    }

    traceEMAC_INT_CORE0_TX();			/* trace macro */

    /* Nyugt嫙zuk az EMAC control modul TX megszak癃嫳嫢. */
    /* Acknowledge EMAC control modul TX IRQ */
    EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_TX);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/** ***************************************************************************************************
* @fn void vFreeRTOSEMACRxThrshInterrupt(void)
* @brief RX Threshold Interrupt for EMAC in FreeRTOS-Plus-TCP compatibility mode
*/
#pragma INTERRUPT(vFreeRTOSEMACRxThrshInterrupt, IRQ)
void vFreeRTOSEMACRxThrshInterrupt(void)
{
    static hdkif_t *hdkif = &hdkif_data[0U];

	if((HWREG(hdkif->emac_base + EMAC_RXINTSTATRAW) & RXnTHRESHPEND_BIT(EMAC_CHANNELNUMBER)) == RXnTHRESHPEND_BIT(EMAC_CHANNELNUMBER))
	{
		/* Let璱tjuk a tov墎bi threshold interrupt gener嫮嫳t. 犆ra enged幨yezni majd az RX tasknak kell. */
		HWREG(hdkif->emac_base + EMAC_RXINTMASKCLEAR) = RXnTHRESHPEND_BIT(EMAC_CHANNELNUMBER);
	}

	traceEMAC_INT_CORE0_RX_THRESH();	/* trace macro */
    EMACCoreIntAck(hdkif->emac_base, (uint32)EMAC_INT_CORE0_RX_THRESH);
}

/** ***************************************************************************************************
* @fn void vFreeRTOSEMACRxInterrupt(void)
* @brief RX Interrupt for EMAC in FreeRTOS-Plus-TCP compatibility mode
*/
#pragma INTERRUPT(vFreeRTOSEMACRxInterrupt, IRQ)
void vFreeRTOSEMACRxInterrupt(void)
{
	static BaseType_t xHigherPriorityTaskWoken;
    static hdkif_t *hdkif = &hdkif_data[0U];
    emac_rx_bd_t *pxCurrentBufferDescriptor;

    if(prvEmacRxTaskHandle != NULL)
    {
    	vTaskNotifyGiveFromISR(prvEmacRxTaskHandle, &xHigherPriorityTaskWoken);
    }
    pxCurrentBufferDescriptor = (emac_rx_bd_t *)HWREG(hdkif->emac_base + EMAC_RXCP(EMAC_CHANNELNUMBER));
	HWREG(hdkif->emac_base + EMAC_RXCP(EMAC_CHANNELNUMBER)) = (uint32_t)pxCurrentBufferDescriptor; 	// Nyugt嫙zuk az EMAC-nak BD feldolgoz嫳嫢.
	EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_RX);											// Nyugt嫙zuk az EMAC control modul RX megszak癃嫳嫢.

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/** ***************************************************************************************************
* @fn void prvEmacRxTask(void *pvParameters)
* @brief defered EMAC RX interrupt handler task.
*/
void prvEmacRxTask(void *pvParameters)
{
	hdkif_t *hdkif = &hdkif_data[0U];						/* EMAC pointerek */
	volatile rxch_t *pxRxChannelDMA = &(hdkif->rxchptr);	/* RX DMA pointerek */
	volatile emac_rx_bd_t *pxCurrentBufferDescriptor; 		/* Az aktu嫮is host feldolgoz嫳 alatt 嫮l� BD c璥e */
	volatile emac_rx_bd_t *pxCurrentBufferDescTemp; 		/* Az aktu嫮is host feldolgoz嫳 alatt 嫮l� BD c璥幯ek ment廥廨e szolg嫮l� v嫮toz� */
    volatile emac_rx_bd_t *pxTailBufferDescriptor;			/* A l嫕colt lista utols� eleme (NEXT = NULL) */
    unsigned int xPacketSize;								/* Az 廨kezett csomag m廨ete byte-okban */
    xNetworkBufferDescriptor_t *pxBufferDescriptor;			/* A FreeRTOS-Plus-TCP csomagle甏鎩a, ezen kereszt ker 嫢ad嫳ra az 廨kezett csomag */
    xIPStackEvent_t xRxEvent;								/* A FreeRTOS-Plus-TCP esem幯y le甏鎩a */

    pxCurrentBufferDescriptor = pxRxChannelDMA->active_head;
    pxTailBufferDescriptor = (emac_rx_bd_t *)(0xfc521130);

    while(1)
    {
    	if(ulTaskNotifyTake(pdTRUE, ipconfigETHERNET_DRIVER_RX_TASK_BLOCK_TIME) > 0)
		{
			while(1)
			{
				/* Megn憴z mekkora csomag 廨kezett */
				xPacketSize = BYTE_SWAP(pxCurrentBufferDescriptor->flags_pktlen) & 0xffff;

				/* Invalid嫮juk a cache-t, hogy az mem鏎i墎an l憝�  csomagba ne zavarjon bele */
				_dcacheInvalidateRange_(BYTE_SWAP((uint32_t)pxCurrentBufferDescriptor->bufptr), BYTE_SWAP((uint32_t)pxCurrentBufferDescriptor->bufptr) + xPacketSize);

				/* Csak akkor 嫮lunk neki feldolgozni a csomagot, ha az EMAC 嫢adta az adott BD-t 廥 a SOP (Start Of Packet bit is be van 嫮l癃va */
				if((BYTE_SWAP(pxCurrentBufferDescriptor->flags_pktlen) & EMAC_BUF_DESC_OWNER) != EMAC_BUF_DESC_OWNER && (BYTE_SWAP(pxCurrentBufferDescriptor->flags_pktlen) & EMAC_BUF_DESC_SOP) == EMAC_BUF_DESC_SOP)
				{
    				if(xEMACDriverLoggingLevel > 1)FreeRTOS_debug_printf(("EMACRX: Packet arrived, BD: %p, RXHP: %p\r\n", pxCurrentBufferDescriptor, HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER))));

					if((BYTE_SWAP(pxCurrentBufferDescriptor->flags_pktlen) & EMAC_BUF_DESC_EOP) != EMAC_BUF_DESC_EOP)
					{
						FreeRTOS_debug_printf(("EMACRX: NO EOP: %p, RXHP: %p\r\n", pxCurrentBufferDescriptor, HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER))));
						// Itt mit k幯e csin嫮ni ??
					}

					/* Csomagkezel廥 */
					if(eConsiderFrameForProcessing((const uint8_t *)BYTE_SWAP((uint32_t)pxCurrentBufferDescriptor->bufptr)) == eProcessBuffer)
					{
						if(xEMACDriverLoggingLevel > 1)FreeRTOS_debug_printf(("EMACRX: BD processing: %p, RXHP: %p\r\n", pxCurrentBufferDescriptor, HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER))));

						/* K廨k a FreeRTOS+TCP stack-t鯷 egy megfelel� m廨et� leir鏒 廥 puffert */
						pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(xPacketSize, (TickType_t)ipconfigTCP_MAX_RECV_BLOCK_TIME_TICKS);

						if(pxBufferDescriptor != NULL)
						{

		    				if(xEMACDriverLoggingLevel > 1)FreeRTOS_debug_printf(("EMACRX: Network buffer allocated. NP: %p, BD: %p, RXHP: %p\r\n", pxBufferDescriptor, pxCurrentBufferDescriptor, HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER))));

		    				memcpy((void *)pxBufferDescriptor->pucEthernetBuffer,(void *)(BYTE_SWAP(pxCurrentBufferDescriptor->bufptr)),xPacketSize);
							pxBufferDescriptor->xDataLength = xPacketSize;

							/* The event about to be sent to the TCP/IP is an Rx event. */
							xRxEvent.eEventType = eNetworkRxEvent;

							/* pvData is used to point to the network buffer descriptor that references the received data. */
							xRxEvent.pvData = (void *) pxBufferDescriptor;

							/* 膺adjuk feldolgoz嫳ra a kapcsott csomagot */
							if(xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE)
							{
								/* Nem sikert 嫢adni a puffert, ez廨t felszabad癃juk*/
								vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);

								/* 宄 logoljuk az esem幯yt.. */
								iptraceETHERNET_RX_EVENT_LOST();
							}
							else
							{
								/* The message was successfully sent to the TCP/IP stack.
								Call the standard trace macro to log the occurrence. */
								iptraceNETWORK_INTERFACE_RECEIVE();
							}
						}
						else
						{
							FreeRTOS_debug_printf(("EMACRX: No more free pxBufferDescriptor.\n\r"));
						}
					}
					else
					{
						if(xEMACDriverLoggingLevel > 0)FreeRTOS_debug_printf(("EMACRX: BD %p dropped, RXHP: %p\r\n", pxCurrentBufferDescriptor, HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER))));
					}

					/* Aktu嫮is BD felszabad癃嫳a */
					pxCurrentBufferDescriptor->bufoff_len = BYTE_SWAP(ipTOTAL_ETHERNET_FRAME_SIZE);
					pxCurrentBufferDescriptor->flags_pktlen = BYTE_SWAP(EMAC_BUF_DESC_OWNER);
					pxCurrentBufferDescTemp = (emac_rx_bd_t *)BYTE_SWAP((uint32_t)pxCurrentBufferDescriptor->next);
					pxCurrentBufferDescriptor->next = NULL;

					/* Jelezz a Threshold mehanizmus sz嫥嫫a, hogy felszabadult egy puffer. */
					if(HWREG(hdkif->emac_base + EMAC_RXFREEBUFFER(EMAC_CHANNELNUMBER)) < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS)HWREG(hdkif->emac_base + EMAC_RXFREEBUFFER(EMAC_CHANNELNUMBER)) = 1;

					/* A l嫕colt lista v嶲彋 friss癃j az 廧p felszabad癃ott BD c璥憝el */
					pxTailBufferDescriptor->next = (emac_rx_bd_t *)BYTE_SWAP((uint32_t)pxCurrentBufferDescriptor);

					/* Ellen鰎瞵z, hogy id鰈驆ben nem haszn嫮ta e fel az EMAC a l嫕c v嶲彋 is, ebben az esetben az EOQ bit be van 嫮l癃va */
					if((BYTE_SWAP(pxTailBufferDescriptor->flags_pktlen) & EMAC_BUF_DESC_EOQ) == EMAC_BUF_DESC_EOQ)
					{
						while(HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER)) != 0);
						HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER)) = (uint32_t)pxCurrentBufferDescriptor;
						if(xEMACDriverLoggingLevel > 0)FreeRTOS_debug_printf(("EMACRX: RX restarted at BD: %p\r\n", pxCurrentBufferDescriptor));
					}
					pxTailBufferDescriptor = pxCurrentBufferDescriptor;
					pxCurrentBufferDescriptor = pxCurrentBufferDescTemp;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			/* Nem kaptunk szemafor-t az adott blocking time-on bel, de head pointer tov墎b l廧ett az utols� ellen鰎z廥 鏒a -> IRQ elveszett */
			if(HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER)) == 0)
			{
				/* Ebben az esetben raind癃juk az EMAC v彋elt a head pointer 甏嫳嫛al */
				HWREG(hdkif->emac_base + EMAC_RXHDP(EMAC_CHANNELNUMBER)) = (uint32_t)pxCurrentBufferDescriptor;
				if(xEMACDriverLoggingLevel > 0)FreeRTOS_debug_printf(("EMACRX: RX restarted at BD: %p\r\n", pxCurrentBufferDescriptor));
			}
		}
    }
}

/** ***************************************************************************************************
 * @fn		uint32 xFreeRTOSEMACHWInit(uint8_t macaddr[6U])
 * @brief   Low level function for Initializes the EMAC module for transmission and reception.
 * @param   macaddr MAC Address of the Module.
 * @return  EMAC_ERR_OK if everything gets initialized
 *          EMAC_ERR_CONN in case of an error in connecting.
 */
uint32 xFreeRTOSEMACHWInit(uint8_t macaddr[6U])
{
	uint32_t i;
	uint32 xPhyIdReadCount = 0U;
	volatile uint32 xPhyId = 0U;
	//volatile uint32 phyLinkRetries = 0xFFFFU;
	volatile uint32 phyLinkRetries = 0x000FU;
	uint32 xReturn = EMAC_ERR_OK;
	static uint8_t xFirstInit = 1;

	hdkif_t *hdkif;
	hdkif = &hdkif_data[0U];

	/* A hdkif strukta inicializ嫮嫳a */
	EMACInstConfig(hdkif);

	/* MAC address 嫢m嫳ol嫳a a hdkif strukt墎a */
	for(i=0;i<EMAC_HWADDR_LEN;i++)hdkif->mac_addr[i] = macaddr[(EMAC_HWADDR_LEN - 1U) - i];

	/* Az EMAC 廥 EMAC control modul inicializ嫮嫳a. */

	/* Soft reset EMAC control modul. (T顤li az irq st嫢usz, control regiszterek, 廥 a CPPI ram tartalm嫢) */
    HWREG(hdkif->emac_ctrl_base + EMAC_CTRL_SOFTRESET) = EMAC_CONTROL_RESET;
    while((HWREG(hdkif->emac_ctrl_base + EMAC_CTRL_SOFTRESET) & EMAC_CONTROL_RESET) == EMAC_CONTROL_RESET);
    HWREG(hdkif->emac_base + EMAC_SOFTRESET) = EMAC_SOFT_RESET;
    while((HWREG(hdkif->emac_base + EMAC_SOFTRESET) & EMAC_SOFT_RESET) == EMAC_SOFT_RESET);

    HWREG(hdkif->emac_base + EMAC_MACCONTROL)= 0U;
    HWREG(hdkif->emac_base + EMAC_RXCONTROL)= 0U;
    HWREG(hdkif->emac_base + EMAC_TXCONTROL)= 0U;

    /* Head, Completion pointerek null嫙嫳a az 飉szes csatorn嫕嫮. */
    for(i=0;i<EMAC_MAX_HEADER_DESC;i++)
    {
        HWREG(hdkif->emac_base + EMAC_RXHDP(i)) = 0U;
        HWREG(hdkif->emac_base + EMAC_TXHDP(i)) = 0U;
        HWREG(hdkif->emac_base + EMAC_RXCP(i)) = 0U;
        HWREG(hdkif->emac_base + EMAC_TXCP(i)) = 0U;
    }

    /* Flow control haszn嫮at墏oz szs嶲es regiszterek be嫮l癃嫳a csak a haszn嫮t csatorn嫕. */
    HWREG(hdkif->emac_base + EMAC_RXFREEBUFFER(EMAC_CHANNELNUMBER)) = ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS;
    HWREG(hdkif->emac_base + EMAC_RXFLOWTHRESH(EMAC_CHANNELNUMBER)) &= (0x0U);
    HWREG(hdkif->emac_base + EMAC_RXFLOWTHRESH(EMAC_CHANNELNUMBER)) |= ipconfigRX_FLOWCONTROL_START_LEVEL;
	#if(ipconfigETHERNET_DRIVER_RX_FLOW_CONTROLL == 1)
    HWREG(hdkif->emac_base + EMAC_MACCONTROL) |= EMAC_MACCONTROL_RXBUFFERFLOWEN;			/* Flow control enged幨yez廥e */
	#endif

    /* Valamennyi csatorn嫕 tiltjuk a TX/RX megszak癃嫳okat */
    HWREG(hdkif->emac_base + EMAC_TXINTMASKCLEAR) = 0xFFU;
    HWREG(hdkif->emac_base + EMAC_RXINTMASKCLEAR) = 0xFFU;

    /* Multicast csomagok v彋el嶭ez kellene.. */
    HWREG(hdkif->emac_base + EMAC_MACHASH1) = 0U;
    HWREG(hdkif->emac_base + EMAC_MACHASH2) = 0U;

    /* AZ RX descriptorok SOP mez鰋幯ek offset 廨t幧e. */
    HWREG(hdkif->emac_base + EMAC_RXBUFFEROFFSET) = 0U;

    /* Az MDIO modul inicializ嫮嫳a, State Machine enged幨yez廥e, clock be嫮l癃嫳a. */
	MDIOInit(hdkif->mdio_base, MDIO_FREQ_INPUT, MDIO_FREQ_OUTPUT);

	/* az MDIO init k驆ben van id� be嫮l癃ani az EMAC MAC c璥eket. */
	EMACMACSrcAddrSet(hdkif->emac_base, hdkif->mac_addr);
	for(i=0;i<8U;i++){EMACMACAddrSet(hdkif->emac_base, i, hdkif->mac_addr, EMAC_MACADDR_MATCH);}

	/* PHY ID kiolvas嫳a */
	do
	{
		if((xPhyId = Dp83640IDGet(hdkif->mdio_base,hdkif->phy_addr)) != 0)break;
	}while(xPhyIdReadCount++ < PHY_INIT_ID_READ_MAX_RETRIES);

	if(0U == xPhyId)xReturn = EMAC_ERR_CONNECT; 	/* Hibajelz廥, ha 0-t olvastunk ID-nak */

	if((uint32)0U == ((MDIOPhyAliveStatusGet(hdkif->mdio_base) >> hdkif->phy_addr) & (uint32)0x01U))
	{
		xReturn = EMAC_ERR_CONNECT;
	}

	if(!Dp83640LinkStatusGet(hdkif->mdio_base, (uint32)EMAC_PHYADDRESS, (uint32)phyLinkRetries))
	{
		xReturn = EMAC_ERR_CONNECT;
	}

	/* EMAC link UP */
	if(EMACLinkSetup(hdkif) != EMAC_ERR_OK)
	{
		xReturn = EMAC_ERR_CONNECT;
	}

	/* RX 廥 TX Buffer Descriptorok kialak癃嫳a */
	if(xFirstInit)
		{
		prvEmacDMAInit(hdkif);
		xFirstInit = 0;
		}

	EMACMIIEnable(hdkif->emac_base);
	EMACRxBroadCastEnable(hdkif->emac_base, (uint32)EMAC_CHANNELNUMBER);
	EMACRxUnicastSet(hdkif->emac_base, (uint32)EMAC_CHANNELNUMBER);
	EMACDisableLoopback(hdkif->emac_base);

	return xReturn;
}


/** ***************************************************************************************************
 * @fn		static void prvEmacDMAInit(hdkif_t *hdkif)
 * @brief   Creates linked buffer descriptor lists for sending and receiving ethernet frames
 * @param   hdkif   network interface structure
 * @return  None
 */
static void prvEmacDMAInit(hdkif_t *hdkif)
{
      txch_t *pxTxChannelDMA;
      rxch_t *pxRxChannelDMA;
	  volatile emac_rx_bd_t *pxCurrentBD;			/* BD linkelt lista buffer kialak癃嫳墏oz az aktu嫮is elem c璥e */
	  unsigned int i;

	  pxTxChannelDMA = &(hdkif->txchptr);
	  pxRxChannelDMA = &(hdkif->rxchptr);

	  pxCurrentBD = (void *)EMAC_TXDMA_PBUF_START_ADDRESS;
	  pxTxChannelDMA->free_head = (void *)pxCurrentBD;
	  pxTxChannelDMA->next_bd_to_process = (void *)pxCurrentBD;
	  pxTxChannelDMA->active_tail = NULL;

	  /* TX Buffer descriptor l嫕colt lista kialak癃嫳a */
      for(i = 0; i < EMAC_TXDMA_PBUF_ALLOC; i++)
      {
    	  pxCurrentBD->next = NULL;	/* l嫕colt lista v嶲e */
    	  pxCurrentBD->bufptr = BYTE_SWAP((uint32)pvPortMalloc(ipTOTAL_ETHERNET_FRAME_SIZE));
    	  pxCurrentBD->bufoff_len = BYTE_SWAP(ipTOTAL_ETHERNET_FRAME_SIZE);
    	  pxCurrentBD->flags_pktlen = 0;
    	  pxCurrentBD++;
      }

      pxCurrentBD = (void *)EMAC_RXDMA_PBUF_START_ADDRESS;

      /* RX Buffer descriptor l嫕colt lista kialak癃嫳a */
      pxRxChannelDMA->active_head = pxRxChannelDMA->active_tail = pxRxChannelDMA->free_head = pxCurrentBD;
      for(i = 0; i < EMAC_RXDMA_PBUF_ALLOC; i++)
      {
    	  if (i < (EMAC_RXDMA_PBUF_ALLOC - 1))pxCurrentBD->next = (emac_rx_bd_t *)BYTE_SWAP((uint32)(pxCurrentBD + 1));
    	  else pxCurrentBD->next = NULL;	/* L嫕colt lista v嶲e */

    	  //TODO: Hibakeres廥hez malloc() haszn嫮ata
    	  pxCurrentBD->bufptr = BYTE_SWAP((uint32)pvPortMalloc(ipTOTAL_ETHERNET_FRAME_SIZE));

    	  //pxCurrentBD->bufoff_len = BYTE_SWAP(MAX_TRANSFER_UNIT);
    	  pxCurrentBD->bufoff_len = BYTE_SWAP(ipTOTAL_ETHERNET_FRAME_SIZE);
    	  pxCurrentBD->flags_pktlen = BYTE_SWAP(EMAC_BUF_DESC_OWNER);
    	  pxCurrentBD++;
      }

      /* DMA BD marad幧 teret null嫙嫳a */
      for(; i<(SIZE_EMAC_CTRL_RAM) / sizeof(emac_tx_bd_t); i++)
      {
    	  pxCurrentBD->next = NULL;
    	  pxCurrentBD->bufptr = NULL;
    	  pxCurrentBD->bufoff_len = NULL;
    	  pxCurrentBD->flags_pktlen = NULL;
    	  pxCurrentBD++;
      }
}


