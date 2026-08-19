/*-----------------------------------------------------------------------*/
/* MMC/SDC (in SPI mode) control module  (C)ChaN, 2007                  */
/*-----------------------------------------------------------------------*/
/* Only rcvr_spi(), xmit_spi(), disk_timerproc() and some macros        */
/* are platform dependent.                                              */
/*-----------------------------------------------------------------------*/


/*
* This file was modified from a sample available from the FatFs
* web site. It was modified to work with an HDK development
* board.
*
*
* jc 20151004 added option to change what SPI is used
*/
//#include "spi.h"


#include <stdint.h>
#include <stdbool.h>
#include "HL_sys_common.h"
#include "HL_gio.h"
#include "HL_spi.h"
#include <assert.h>
#include "fatfs/diskio.h"
#include "mmc-hercules.h"
#include <nand_flash.h>

//#include "SD_Card/fatfs/diskio.h"
//#include "SD_Card/mmc-hercules.h"


/* Definitions for MMC/SDC command */
#define CMD0    (0x40+0)    /* GO_IDLE_STATE */
#define CMD1    (0x40+1)    /* SEND_OP_COND */
#define CMD8    (0x40+8)    /* SEND_IF_COND */
#define CMD9    (0x40+9)    /* SEND_CSD */
#define CMD10    (0x40+10)    /* SEND_CID */
#define CMD12    (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16    (0x40+16)    /* SET_BLOCKLEN */
#define CMD17    (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18    (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23    (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24    (0x40+24)    /* WRITE_BLOCK */
#define CMD25    (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41    (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55    (0x40+55)    /* APP_CMD */
#define CMD58    (0x40+58)    /* READ_OCR */




extern nand_device_t *SD_devs;

gioPORT_t *_spiPORT = 2;
spiBASE_t *_spiREG = 4;


void mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg) {
    _spiPORT = port;
    _spiREG = reg;
}


// asserts the CS pin to the card
static
void DESELECT (void)
{
//  spiREG2->PCDOUT |=  0x01;   // SCS[0] = high
//    spiPORT2->DSET = 0x01;      // SCS[0] = high
//  spiPORT2->DSET = 0x02;      // SCS[1] = high
//  gioPORTA->DSET = 0x04;      // A2 = high

    spiPORT4->DSET = 0x04;      // SCS[2] = high

}

// de-asserts the CS pin to the card
static
void SELECT (void)
{
//  spiREG2->PCDOUT &= ~0x01;   // SCS[0] = low
//    spiPORT2-> DCLR = 0x01;     // SCS[0] = low
//  spiPORT2-> DCLR = 0x02;     // SCS[1] = low
//  gioPORTA->DCLR = 0x04;      // A2 = low

    spiPORT4-> DCLR = 0x04;     // SCS[2] = low
}



// asserts the CS pin to the card
static
void lisco_DESELECT (nand_device_t *h)
{
    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

    printk(" === DESELECT();   ===\n");
    _spiPORT->DSET = 1U<<2U;        // SCS[0] = high
    printk(" === DESELECT(); out  ===\n");
}


// de-asserts the CS pin to the card
static
void lisco_SELECT (nand_device_t *h)
{
    spiBASE_t *spi = h->spi;
   spiDAT1_t *dataconfig_t = &h->dc;

   uint8_t dummy;
   uint8_t Tx_Data = 0x0;
   uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
   uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
   SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
   uint8_t ChipSelect = dataconfig_t->CSNR;

    assert(spi); // call mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg) first
    assert(spi); // call mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg) first
//@20221018    spi-> DCLR = 1U<<2U; ;        // SCS[0] = low
}
#if 0
static
void DESELECT (void)
{
    _spiPORT->DSET = 0x01;        // SCS[0] = high
}


// de-asserts the CS pin to the card
static
void SELECT (void)
{
    assert(_spiPORT); // call mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg) first
    assert(_spiREG); // call mmcSelectSpi(gioPORT_t *port, spiBASE_t *reg) first
    _spiPORT-> DCLR = 0x01;        // SCS[0] = low
}
#endif

/*------------------------------------------------------------------------------
  Write and Read a byte on SPI interface
*------------------------------------------------------------------------------*/
unsigned char lisco_SPI_send (nand_device_t *h, unsigned char outb) {

    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

  while ((spi->FLG & 0x0200) == 0); // Wait until TXINTFLG is set for previous transmission
  spi->DAT1 = outb | 0x100D0000;    // transmit register address


  while ((spi->FLG & 0x0100) == 0); // Wait until RXINTFLG is set when new value is received
  return((unsigned char)spi->BUF);  // Return received value
}


/*--------------------------------------------------------------------------


  Module Private Functions


---------------------------------------------------------------------------*/


static volatile
DSTATUS Stat = STA_NOINIT;    /* Disk status */


static volatile
BYTE Timer1, Timer2;    /* 100Hz decrement timer */


static
BYTE CardType;            /* b0:MMC, b1:SDC, b2:Block addressing */


static
BYTE PowerFlag = 0;    /* indicates if "power" is on */


/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/


static
void xmit_spi(BYTE dat)
{
    unsigned int ui32RcvDat;


    while ((_spiREG->FLG & 0x0200) == 0); // Wait until TXINTFLG is set for previous transmission
    _spiREG->DAT1 = dat | 0x100D0000;    // transmit register address


    while ((_spiREG->FLG & 0x0100) == 0); // Wait until RXINTFLG is set when new value is received
    ui32RcvDat = _spiREG->BUF;  // to get received value
}

static
void lisco_xmit_spi(nand_device_t *h,BYTE dat)
{
    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

    unsigned int ui32RcvDat;


    while ((spi->FLG & 0x0200) == 0); // Wait until TXINTFLG is set for previous transmission
    spi->DAT1 = dat | 0x100D0000;    // transmit register address


    while ((spi->FLG & 0x0100) == 0); // Wait until RXINTFLG is set when new value is received
    ui32RcvDat = spi->BUF;  // to get received value
}


/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI  (Platform dependent)                */
/*-----------------------------------------------------------------------*/




static
BYTE lisco_rcvr_spi (nand_device_t *h)
{
    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

    while ((spi->FLG & 0x0200) == 0); // Wait until TXINTFLG is set for previous transmission
    spi->DAT1 = 0xFF | 0x100D0000;    // transmit register address

    while ((spi->FLG & 0x0100) == 0); // Wait until RXINTFLG is set when new value is received
    return((unsigned char)spi->BUF);  // Return received value
}


static
void rcvr_spi_m (BYTE *dst)
{
    *dst = lisco_rcvr_spi(SD_devs);
}


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                  */
/*-----------------------------------------------------------------------*/


static
BYTE wait_ready (void)
{
    BYTE res;




    Timer2 = 50;    /* Wait for ready in timeout of 500ms */
    lisco_rcvr_spi(SD_devs);
    do
        res = lisco_rcvr_spi(SD_devs);
    while ((res != 0xFF) && Timer2);

    //printk(" wait_ready %x\n",res);
    return res;
}


/*-----------------------------------------------------------------------*/
/* Send 80 or so clock transitions with CS and DI held high. This is    */
/* required after card power up to get it into SPI mode                  */
/*-----------------------------------------------------------------------*/
static
void send_initial_clock_train(void)
{
    unsigned int i;

    /* Ensure CS is held high. */
    DESELECT();

    /* Send 10 bytes over the SSI. This causes the clock to wiggle the */
    /* required number of times. */
    for(i = 0 ; i < 100 ; i++)
    {
        /* Write DUMMY data */
        /* FIFO. */
        //SPI_send (0xFF);
        lisco_SPI_send(SD_devs,0xaa);
    }
}

/*-----------------------------------------------------------------------*/
/* Power Control  (Platform dependent)                                  */
/*-----------------------------------------------------------------------*/
/* When the target system does not support socket power control, there  */
/* is nothing to do in these functions and chk_power always returns 1.  */


static
void power_on (void)
{
    //printk(" === power_on ===\n");
    /*
    * This doesn't really turn the power on, but initializes the
    * SPI port and pins needed to talk to the card.
    */
//sd  spiInit();


    /* Set DI and CS high and apply more than 74 pulses to SCLK for the card */
    /* to be able to accept a native command. */
    send_initial_clock_train();

    //printk(" === send_initial_clock_train(); ===\n");
    PowerFlag = 1;
}


// set the SPI speed to the max setting
static
void lisco_set_max_speed(nand_device_t *h)
{
    spiBASE_t *spi = h->spi;
    spiDAT1_t *dataconfig_t = &h->dc;

    uint8_t dummy;
    uint8_t Tx_Data = 0x0;
    uint32_t Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8_t ChipSelect = dataconfig_t->CSNR;

    // todo jc 20151004 - check if this is portable between hercules controllers/clock speeds
    spi->FMT0 &= 0xFFFF00FF;  // mask out baudrate prescaler
                                    // Max. 5 MBit used for Data Transfer.
    spi->FMT0 |= 5 << 8;    // baudrate prescale 10MHz / (1+1) = 5MBit
}


static
void power_off (void)
{
    PowerFlag = 0;
}


static
int chk_power(void)        /* Socket power state: 0=off, 1=on */
{
    return PowerFlag;
}






/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/


static
bool rcvr_datablock (
    BYTE *buff,            /* Data buffer to store received data */
    UINT btr            /* Byte count (must be even number) */
)
{
    BYTE token;

    Timer1 = 100;
    do {                            /* Wait for data packet in timeout of 100ms */
        token = lisco_rcvr_spi(SD_devs);
    } while ((token == 0xFF) && Timer1);
    if(token != 0xFE) return FALSE;    /* If not valid data token, retutn with error */

    do {                            /* Receive the data block into buffer */
        rcvr_spi_m(buff++);

        rcvr_spi_m(buff++);

    } while (btr -= 2);
    lisco_rcvr_spi(SD_devs);                        /* Discard CRC */

    lisco_rcvr_spi(SD_devs);

    return TRUE;                    /* Return with success */
}






/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                            */
/*-----------------------------------------------------------------------*/


#if _READONLY == 0
static
bool xmit_datablock (
    const BYTE *buff,    /* 512 byte data block to be transmitted */
    BYTE token            /* Data/Stop token */
)
{
    BYTE resp, wc;




    if (wait_ready() != 0xFF) return FALSE;


    lisco_xmit_spi(SD_devs,token);                    /* Xmit data token */
    if (token != 0xFD) {    /* Is data token */
        wc = 0;
        do {                            /* Xmit the 512 byte data block to MMC */
            lisco_xmit_spi(SD_devs,*buff++);
            lisco_xmit_spi(SD_devs,*buff++);
        } while (--wc);
        lisco_xmit_spi(SD_devs,0xFF);                    /* CRC (Dummy) */
        lisco_xmit_spi(SD_devs,0xFF);
        resp = lisco_rcvr_spi(SD_devs);                /* Reveive data response */
        if ((resp & 0x1F) != 0x05)        /* If not accepted, return with error */
            return FALSE;
    }


    return TRUE;
}
#endif /* _READONLY */






/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/
#define LISCO_HW 1
#ifdef LISCO_HW
static
BYTE lisco_send_cmd (
        nand_device_t *h,
        BYTE cmd,        /* Command byte */
        DWORD arg        /* Argument */
)
{
    BYTE n, res;


    if (wait_ready() != 0xFF) return 0xFF;

    /* Send command packet */
    lisco_xmit_spi(SD_devs,cmd);                        /* Command */
    lisco_xmit_spi(SD_devs,(BYTE)(arg >> 24));        /* Argument[31..24] */
    lisco_xmit_spi(SD_devs,(BYTE)(arg >> 16));        /* Argument[23..16] */
    lisco_xmit_spi(SD_devs,(BYTE)(arg >> 8));            /* Argument[15..8] */
    lisco_xmit_spi(SD_devs,(BYTE)arg);                /* Argument[7..0] */
    n = 0xff;
    if (cmd == CMD0) n = 0x95;            /* CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;            /* CRC for CMD8(0x1AA) */
    lisco_xmit_spi(SD_devs,n);


    /* Receive command response */
    if (cmd == CMD12) lisco_rcvr_spi(SD_devs);        /* Skip a stuff byte when stop reading */
    n = 10;                                /* Wait for a valid response in timeout of 10 attempts */
    do{
        res = lisco_rcvr_spi(SD_devs);
    }
    while ((res & 0x80) && --n);

    //printk(" lisco_send_cmd CMD[0x%x] res[0x%02x]\n",cmd,res);
    return res;            /* Return with the response value */
}
#if 1
static
BYTE send_cmd_ret (
    BYTE cmd,        /* Command byte */
    DWORD arg,        /* Argument */
    char* RX_ptr
)
{
    //printk(" send_cmd_ret  RX_ptr = %08x\n",RX_ptr);
    BYTE n, res;
    int status=0;
    unsigned char TX_Buffer[6]=0;

    TX_Buffer[0]=cmd;
    TX_Buffer[1]=((BYTE)(arg >> 24));
    TX_Buffer[2]=((BYTE)(arg >> 16));
    TX_Buffer[3]=((BYTE)(arg >> 8));
    TX_Buffer[4]=((BYTE)(arg));
    TX_Buffer[5]=0xff;
    if (cmd == CMD0) TX_Buffer[5] = 0x95;            /* CRC for CMD0(0) */
    if (cmd == CMD8) TX_Buffer[5] = 0x87;            /* CRC for CMD8(0x1AA) */
    //status = spiRWCS(SD_devs, 6, TX_Buffer, RX_ptr, false);
    status = spiRWCS(SD_devs, 6, TX_Buffer, RX_ptr, true);


    if (cmd == CMD12){
        //rcvr_spi();
        lisco_rcvr_spi(SD_devs);
    }        /* Skip a stuff byte when stop reading */
       n = 10;                                /* Wait for a valid response in timeout of 10 attempts */
       do{
           res = lisco_rcvr_spi(SD_devs);
           //printk(" lisco_rcvr_spi %x\n",res);
       }while ((res & 0x80) && --n);

    return res;            /* Return with the response value */
}
#endif
static
BYTE send_cmd (
    BYTE cmd,        /* Command byte */
    DWORD arg        /* Argument */
)
{

    BYTE n, res;
    int status=0;
    unsigned char TX_Buffer[6]=0;
    unsigned char RX_Buffer[6]=0;
    TX_Buffer[0]=cmd;
    TX_Buffer[1]=((BYTE)(arg >> 24));
    TX_Buffer[2]=((BYTE)(arg >> 16));
    TX_Buffer[3]=((BYTE)(arg >> 8));
    TX_Buffer[4]=((BYTE)(arg));
    TX_Buffer[4]=0xff;
    if (cmd == CMD0) TX_Buffer[4] = 0x95;            /* CRC for CMD0(0) */
    if (cmd == CMD8) TX_Buffer[4] = 0x87;            /* CRC for CMD8(0x1AA) */

    status = spiRWCS(SD_devs, 6, TX_Buffer, RX_Buffer, false);
                                    /* Wait for a valid response in timeout of 10 attempts */
    return res;            /* Return with the response value */
}
#else
static
BYTE send_cmd (
    BYTE cmd,        /* Command byte */
    DWORD arg        /* Argument */
)
{
    BYTE n, res;




    if (wait_ready() != 0xFF) return 0xFF;


    /* Send command packet */
    xmit_spi(cmd);                        /* Command */
    xmit_spi((BYTE)(arg >> 24));        /* Argument[31..24] */
    xmit_spi((BYTE)(arg >> 16));        /* Argument[23..16] */
    xmit_spi((BYTE)(arg >> 8));            /* Argument[15..8] */
    xmit_spi((BYTE)arg);                /* Argument[7..0] */
    n = 0xff;
    if (cmd == CMD0) n = 0x95;            /* CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;            /* CRC for CMD8(0x1AA) */
    xmit_spi(n);


    /* Receive command response */
    if (cmd == CMD12) rcvr_spi();        /* Skip a stuff byte when stop reading */
    n = 10;                                /* Wait for a valid response in timeout of 10 attempts */
    do
        res = rcvr_spi();
    while ((res & 0x80) && --n);


    return res;            /* Return with the response value */
}

#endif
/*-----------------------------------------------------------------------*
* Send the special command used to terminate a multi-sector read.
*
* This is the only command which can be sent while the SDCard is sending
* data. The SDCard spec indicates that the data transfer will stop 2 bytes
* after the 6 byte CMD12 command is sent and that the card will then send
* 0xFF for between 2 and 6 more bytes before the R1 response byte.  This
* response will be followed by another 0xFF byte.  In testing, however, it
* seems that some cards don't send the 2 to 6 0xFF bytes between the end of
* data transmission and the response code.  This function, therefore, merely
* reads 10 bytes and, if the last one read is 0xFF, returns the value of the
* latest non-0xFF byte as the response code.
*
*-----------------------------------------------------------------------*/


static
BYTE send_cmd12 (void)
{
    BYTE n, res, val;


    /* For CMD12, we don't wait for the card to be idle before we send
    * the new command.
    */


    /* Send command packet - the argument for CMD12 is ignored. */
    lisco_xmit_spi(SD_devs,CMD12);
    lisco_xmit_spi(SD_devs,0);
    lisco_xmit_spi(SD_devs,0);
    lisco_xmit_spi(SD_devs,0);
    lisco_xmit_spi(SD_devs,0);
    lisco_xmit_spi(SD_devs,0);


    /* Read up to 10 bytes from the card, remembering the value read if it's
      not 0xFF */
    for(n = 0; n < 10; n++)
    {
        val = lisco_rcvr_spi(SD_devs);
        if(val != 0xFF)
        {
            res = val;
        }
    }


    return res;            /* Return with the response value */
}


/*--------------------------------------------------------------------------


  Public Functions


---------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                */
/*-----------------------------------------------------------------------*/


DSTATUS disk_initialize (
    BYTE drv        /* Physical drive nmuber (0) */
)
{
    BYTE n, ty, ocr[4];


    //printk(" === disk_initialize ===\n");

//    if (drv){
//        printk(" STA_NOINIT\n");
//        return STA_NOINIT;
//    }            /* Supports only single drive */

    if (Stat & STA_NODISK) {
        printk(" No card in the socket\n");
        return Stat;
    }    /* No card in the socket */

    TickType_t delay = 100;

    power_on();                            /* Force socket power on */

    send_initial_clock_train();            /* Ensure the card is in SPI mode */




    SELECT();                /* CS = L */ //set gpio
    unsigned char RX_Buffer[6]=0;

    ty = 0;
    if (lisco_send_cmd(SD_devs,CMD0, 0) == 1) {            /* Enter Idle state */
        Timer1 = 100;                        /* Initialization timeout of 1000 msec */
        if (lisco_send_cmd(SD_devs,CMD8, 0x1AA) == 1) {    /* SDC Ver2+ */
            for (n = 0; n < 4; n++) ocr[n] = lisco_rcvr_spi(SD_devs);
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {    /* The card can work at vdd range of 2.7-3.6V */
                do {
                    if (lisco_send_cmd(SD_devs,CMD55, 0) <= 1 && lisco_send_cmd(SD_devs,CMD41, 1UL << 30) == 0)    break;    /* ACMD41 with HCS bit */
                } while (Timer1);
                if (Timer1 && lisco_send_cmd(SD_devs,CMD58, 0) == 0) {    /* Check CCS bit */
                    for (n = 0; n < 4; n++) ocr[n] = lisco_rcvr_spi(SD_devs);
                    ty = (ocr[0] & 0x40) ? 6 : 2;
                    printk(" disk_initialize init success\n",ocr[0]);
                }
            }
        } else {
            /* SDC Ver1 or MMC */
            ty = (lisco_send_cmd(SD_devs,CMD55, 0) <= 1 && lisco_send_cmd(SD_devs,CMD41, 0) <= 1) ? 2 : 1;    /* SDC : MMC */
            do {
                if (ty == 2) {
                    if (lisco_send_cmd(SD_devs,CMD55, 0) <= 1 && lisco_send_cmd(SD_devs,CMD41, 0) == 0) break;    /* ACMD41 */
                } else {
                    if (lisco_send_cmd(SD_devs,CMD1, 0) == 0) break;                                /* CMD1 */
                }
            } while (Timer1);
            if (!Timer1 || lisco_send_cmd(SD_devs,CMD16, 512) != 0)    /* Select R/W block length */
                ty = 0;
        }
    }
    CardType = ty;
    DESELECT();            /* CS = H */
    lisco_rcvr_spi(SD_devs);            /* Idle (Release DO) */

    if (ty) {            /* Initialization succeded */
        Stat &= ~STA_NOINIT;        /* Clear STA_NOINIT */
        lisco_set_max_speed(SD_devs);
    } else {            /* Initialization failed */
        power_off();
    }

    return Stat;
}






/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                      */
/*-----------------------------------------------------------------------*/


DSTATUS disk_status (
    BYTE drv        /* Physical drive nmuber (0) */
)
{
    if (drv) return STA_NOINIT;        /* Supports only single drive */
    return Stat;
}






/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/


DRESULT disk_read (
    BYTE drv,            /* Physical drive nmuber (0) */
    BYTE *buff,            /* Pointer to the data buffer to store read data */
    DWORD sector,        /* Start sector number (LBA) */
    UINT count            /* Sector count (1..255) */
)
{
    if (drv || !count) /*return RES_PARERR;*/return 4;
    if (Stat & STA_NOINIT) /*return RES_NOTRDY;*/return 3;


    if (!(CardType & 4)) sector *= 512;    /* Convert to byte address if needed */


    SELECT();            /* CS = L */


    if (count == 1) {    /* Single block read */
        if ((lisco_send_cmd(SD_devs,CMD17, sector) == 0)    /* READ_SINGLE_BLOCK */
            && rcvr_datablock(buff, 512)){
            count = 0;
        }
    }
    else {                /* Multiple block read */
        if (lisco_send_cmd(SD_devs,CMD18, sector) == 0) {    /* READ_MULTIPLE_BLOCK */
            do {
                if (!rcvr_datablock(buff, 512)) break;
                buff += 512;
            } while (--count);
            //lisco_send_cmd12(SD_devs);                /* STOP_TRANSMISSION */
            lisco_send_cmd(SD_devs,CMD12, sector);
        }
    }

    DESELECT();            /* CS = H */
    lisco_rcvr_spi(SD_devs);            /* Idle (Release DO) */

    //return count ? RES_ERROR : RES_OK;
    return count ? 1 : 0;
}






/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                      */
/*-----------------------------------------------------------------------*/


#if _READONLY == 0
DRESULT disk_write (
    BYTE drv,            /* Physical drive nmuber (0) */
    const BYTE *buff,    /* Pointer to the data to be written */
    DWORD sector,        /* Start sector number (LBA) */
    UINT count            /* Sector count (1..255) */
)
{
   // if (drv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) /*return RES_NOTRDY;*/return 3;
    if (Stat & STA_PROTECT) /*return RES_WRPRT;*/return 2;


    if (!(CardType & 4)) sector *= 512;    /* Convert to byte address if needed */


    SELECT();            /* CS = L */


    if (count == 1) {    /* Single block write */
        if ((lisco_send_cmd(SD_devs,CMD24, sector) == 0)    /* WRITE_BLOCK */
            && xmit_datablock(buff, 0xFE))
            count = 0;
    }
    else {                /* Multiple block write */
        if (CardType & 2) {
            send_cmd(CMD55, 0); send_cmd(CMD23, count);    /* ACMD23 */
        }
        if (send_cmd(CMD25, sector) == 0) {    /* WRITE_MULTIPLE_BLOCK */
            do {
                if (!xmit_datablock(buff, 0xFC)) break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD))    /* STOP_TRAN token */
                count = 1;
        }
    }


    DESELECT();            /* CS = H */
    lisco_rcvr_spi(SD_devs);            /* Idle (Release DO) */


    //return count ? RES_ERROR : RES_OK;
    return count ? 1 : 0;
}
#endif /* _READONLY */






/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                              */
/*-----------------------------------------------------------------------*/


DRESULT disk_ioctl (
    BYTE drv,        /* Physical drive nmuber (0) */
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    DRESULT res;
    BYTE n, csd[16], *ptr = buff;
    WORD csize;




    //if (drv) return RES_PARERR;


    //res = RES_ERROR;
    res = 2;

    if (ctrl == CTRL_POWER) {
        switch (*ptr) {
        case 0:        /* Sub control code == 0 (POWER_OFF) */
            if (chk_power())
                power_off();        /* Power off */
            //res = RES_OK;
            res = 0;
            break;
        case 1:        /* Sub control code == 1 (POWER_ON) */
            power_on();                /* Power on */
            //res = RES_OK;
            res = 0;
            break;
        case 2:        /* Sub control code == 2 (POWER_GET) */
            *(ptr+1) = (BYTE)chk_power();
            //res = RES_OK;
            res = 0;
            break;
        default :
            //res = RES_PARERR;
            res = 4;
        }
    }
    else {
        if (Stat & STA_NOINIT) /*return RES_NOTRDY;*/return 3;


        SELECT();        /* CS = L */


        switch (ctrl) {
        case GET_SECTOR_COUNT :    /* Get number of sectors on the disk (DWORD) */
            if ((lisco_send_cmd(SD_devs,CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
                if ((csd[0] >> 6) == 1) {    /* SDC ver 2.00 */
                    csize = csd[9] + ((WORD)csd[8] << 8) + 1;
                    *(DWORD*)buff = (DWORD)csize << 10;
                } else {                    /* MMC or SDC ver 1.XX */
                    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                    *(DWORD*)buff = (DWORD)csize << (n - 9);
                }
                //res = RES_OK;
                res = 0;
            }
            break;


        case GET_SECTOR_SIZE :    /* Get sectors on the disk (WORD) */
            *(WORD*)buff = 512;
            //res = RES_OK;
            res = 0;
            break;


        case CTRL_SYNC :    /* Make sure that data has been written */
            if (wait_ready() == 0xFF)
                //res = RES_OK;
                res = 0;
            break;


        case MMC_GET_CSD :    /* Receive CSD as a data block (16 bytes) */
            if (lisco_send_cmd(SD_devs,CMD9, 0) == 0        /* READ_CSD */
                && rcvr_datablock(ptr, 16))
                //res = RES_OK;
                res = 0;
            break;


        case MMC_GET_CID :    /* Receive CID as a data block (16 bytes) */
            if (lisco_send_cmd(SD_devs,CMD10, 0) == 0        /* READ_CID */
                && rcvr_datablock(ptr, 16))
                //res = RES_OK;
                res = 0;
            break;


        case MMC_GET_OCR :    /* Receive OCR as an R3 resp (4 bytes) */
            if (lisco_send_cmd(SD_devs,CMD58, 0) == 0) {    /* READ_OCR */
                for (n = 0; n < 4; n++)
                    *ptr++ = lisco_rcvr_spi(SD_devs);
                //res = RES_OK;
                res = 0;
            }


//        case MMC_GET_TYPE :    /* Get card type flags (1 byte) */
//            *ptr = CardType;
//            res = RES_OK;
//            break;


        default:
            //res = RES_PARERR;
            res = 4;
        }


        DESELECT();            /* CS = H */
        lisco_rcvr_spi(SD_devs);            /* Idle (Release DO) */
    }


    return res;
}






/*-----------------------------------------------------------------------*/
/* Device Timer Interrupt Procedure  (Platform dependent)                */
/*-----------------------------------------------------------------------*/
/* This function must be called in period of 10ms                        */


void disk_timerproc (void)
{
//    BYTE n, s;
    BYTE n;




    n = Timer1;                        /* 100Hz decrement timer */
    if (n) Timer1 = --n;
    n = Timer2;
    if (n) Timer2 = --n;


}


/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module          */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from    */
/* FatFs module. Any valid time must be returned even if  */
/* the system does not support a real time clock.          */


DWORD get_fattime (void)
{


    return    ((2007UL-1980) << 25)    // Year = 2007
            | (6UL << 21)            // Month = June
            | (5UL << 16)            // Day = 5
            | (11U << 11)            // Hour = 11
            | (38U << 5)            // Min = 38
            | (0U >> 1)                // Sec = 0
            ;


}
