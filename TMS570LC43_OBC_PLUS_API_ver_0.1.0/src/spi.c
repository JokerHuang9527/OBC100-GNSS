
#include <stdio.h>
#include "spi.h"
#include "xioctl.h"
#include "dma_spi.h"
#include "HL_sys_dma.h"

io_spi_devices_handle io_spi_devices_handle_table[] = {
    {mibspiREG2, mibspiRAM2, {DMA_CH2 ,DMA_REQ3}, {DMA_CH3 ,DMA_REQ2}, 0, DATA_FORMAT2, 0, 0, mibspi2Init},
    {mibspiREG3, mibspiRAM3, {DMA_CH4 ,DMA_REQ15}, {DMA_CH5 ,DMA_REQ14}, 0, DATA_FORMAT0, 0, 0, mibspi3Init},
    {mibspiREG5, mibspiRAM5, {DMA_CH9 ,DMA_REQ31}, {DMA_CH8 ,DMA_REQ30}, 0, DATA_FORMAT0, 1, 0, mibspi5Init},};

void spi_info(){
    int i;
    io_spi_devices_handle *handle;
    printk_ni("\n=======SPI  INFOMATION======\n");
    printk_ni("Index Speed TxFLAG RxFLAG   State\n");
    for(i = 0; i < 3; i++){
        handle = (io_spi_devices_handle *)(spi_open(i));
        printk_ni("  %02d   ", i+1);
        if(handle->DFMT == DATA_FORMAT0)
            printk_ni("  1M  ");
        else if(handle->DFMT == DATA_FORMAT1)
            printk_ni("  5M  ");
        else if(handle->DFMT == DATA_FORMAT2)
            printk_ni(" 10M  ");
        else if(handle->DFMT == DATA_FORMAT3)
            printk_ni(" 20M  ");

        if(handle->blockflag & IO_SPI_TX_INT)
            printk_ni("  non   ");
        else
            printk_ni("block   ");
        if(handle->blockflag & IO_SPI_RX_INT)
            printk_ni("  non   ");
        else
            printk_ni("block   ");
        if(handle->state)
            printk_ni("work");
        else
            printk_ni("standby");

        printk_ni("\n");
    }
}

int is_spi_ready(void *hnd){
    int index = (int)hnd;
    io_spi_devices_handle* handle = (io_spi_devices_handle *)(spi_open(index));
    if (handle == NULL)
        return E_INVALID_INPUT;
    if (handle->state == 1)
        return E_BUSY;
    return E_SUCCESS;
}

int spi_transfer(void *hnd, uint16 length, uint8* txbuffer,uint8* rxbuffer)
{
    int i;
    int index = (int)hnd;
    uint16 long_len, short_len;
    io_spi_devices_handle* handle = (io_spi_devices_handle *)(spi_open(index));
    if (handle == NULL)
        return E_INVALID_INPUT;
    if (handle->state == 1)
        return E_BUSY;
    long_len = length / 64;
    short_len = length % 64;
    if(index == SPI_DEVICE3){
        handle->init();
        handle->group = 0;
        mibspi_dma_transfer(index, txbuffer, rxbuffer, length);
        if(handle->blockflag == 0)
            while(is_spi_ready(index));
        return E_SUCCESS;
    }
    if(long_len != 0){
        handle->init();
        handle->group = 1;
        mibspi_dma_transfer(index, txbuffer, rxbuffer, long_len);
        if(handle->blockflag == 0)
            while(is_spi_ready(index));
    }
    if(short_len != 0){
        while(is_spi_ready(index)); //wait for long transfer finish, It will become polling mode.
        handle->init();
        handle->group = 0;
        txbuffer += long_len * 128;
        rxbuffer += long_len * 128;
        mibspi_dma_transfer(index, txbuffer, rxbuffer, short_len);
        if(handle->blockflag == 0)
            while(is_spi_ready(index));
    }
    return E_SUCCESS;
}

void *spi_open(int device)
{
//    spi_init(device);
    if((device >= 0) && (device <= 2)){
        io_spi_devices_handle* handle;
        handle = &io_spi_devices_handle_table[device];
        return (void *)handle;
    }
    else
        return NULL;
}

int spi_close(void *handle)
{
    return 0;
}

int spi_ioctl(void *param1, void *param2, void *param3, void *param4)
{
    int device = (int)param1;
    io_spi_devices_handle *handle = (io_spi_devices_handle *)(spi_open(device));
    if (handle == NULL)
            return E_INVALID_INPUT;

    int ctrl_type = (io_ctrl_t) param2;

    switch (ctrl_type){

    case SPI_CTL_BLOCK_MODE:
    {
        uint32 mode = (int) param3;
        int flag = (uint32)param4;

        if ((mode != NONE_BLOCKING) && (mode != BLOCKING))
            return E_INVALID_INPUT;

        if ((flag != IO_SPI_RX_INT) && (flag != IO_SPI_TX_INT))
            return E_INVALID_INPUT;

        if(mode == NONE_BLOCKING)
            handle->blockflag |= flag;
        else
            handle->blockflag &= (!flag);

        return E_SUCCESS;

    }
    case SPI_CTL_BAUDRATE:
    {
        uint8 baud = (uint8)param3;

        if (baud == IO_SPI_SPEED_1M)
            handle->DFMT = DATA_FORMAT0;
        else if (baud == IO_SPI_SPEED_5M)
            handle->DFMT = DATA_FORMAT1;
        else if (baud == IO_SPI_SPEED_10M)
            handle->DFMT = DATA_FORMAT2;
        else if (baud == IO_SPI_SPEED_20M)
            handle->DFMT = DATA_FORMAT3;
        else
            return E_INVALID_INPUT;

        return(E_SUCCESS);
    }
    default:
    {
        return(E_NOT_SUPPORT);
    }

    }


}

/***********MIBSPI Init******************/
void mibspi2Init()
{
uint32 i ;

/** @b initialize @b MIBSPI2 */

/** bring MIBSPI out of reset */
mibspiREG2->GCR0 = 0U;
mibspiREG2->GCR0 = 1U;

/** enable MIBSPI2 multibuffered mode and enable buffer RAM */
mibspiREG2->MIBSPIE = (mibspiREG2->MIBSPIE & 0xFFFFFFFEU) | 1U;

/** MIBSPI2 master mode and clock configuration */
mibspiREG2->GCR1 = (mibspiREG2->GCR1 & 0xFFFFFFFCU) | ((uint32)((uint32)1U << 1U)  /* CLOKMOD */
              | 1U);  /* MASTER */

/** MIBSPI2 enable pin configuration */
mibspiREG2->INT0 = (mibspiREG2->INT0 & 0xFEFFFFFFU) | (uint32)((uint32)0U << 24U);  /* ENABLE HIGHZ */

/** - Delays */
mibspiREG2->DELAY = (uint32)((uint32)27U << 24U)  /* C2TDELAY */
                  | (uint32)((uint32)0U << 16U)  /* T2CDELAY */
                  | (uint32)((uint32)0U << 8U)   /* T2EDELAY */
                  | (uint32)((uint32)0U << 0U);  /* C2EDELAY */

/** - Data Format 0 */
mibspiREG2->FMT0 = (uint32)((uint32)4U << 24U)  /* wdelay */
                 | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                 | (uint32)((uint32)0U << 22U)  /* parity enable */
                 | (uint32)((uint32)0U << 21U)  /* wait on enable */
                 | (uint32)((uint32)0U << 20U)  /* shift direction */
                 | (uint32)((uint32)0U << 17U)  /* clock polarity */
                 | (uint32)((uint32)1U << 16U)  /* clock phase */
                 | (uint32)((uint32)97U << 8U)  /* baudrate prescale */
                 | (uint32)((uint32)16U << 0U); /* data word length */

/** - Data Format 1 */
mibspiREG2->FMT1 = (uint32)((uint32)7U << 24U)  /* wdelay */
                 | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                 | (uint32)((uint32)0U << 22U)  /* parity enable */
                 | (uint32)((uint32)0U << 21U)  /* wait on enable */
                 | (uint32)((uint32)0U << 20U)  /* shift direction */
                 | (uint32)((uint32)0U << 17U)  /* clock polarity */
                 | (uint32)((uint32)1U << 16U)  /* clock phase */
                 | (uint32)((uint32)19U << 8U)  /* baudrate prescale */
                 | (uint32)((uint32)16U << 0U); /* data word length */

/** - Data Format 2 */
mibspiREG2->FMT2 = (uint32)((uint32)7U << 24U)  /* wdelay */
                 | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                 | (uint32)((uint32)0U << 22U)  /* parity enable */
                 | (uint32)((uint32)0U << 21U)  /* wait on enable */
                 | (uint32)((uint32)0U << 20U)  /* shift direction */
                 | (uint32)((uint32)0U << 17U)  /* clock polarity */
                 | (uint32)((uint32)1U << 16U)  /* clock phase */
                 | (uint32)((uint32)9U << 8U)  /* baudrate prescale */
                 | (uint32)((uint32)16U << 0U); /* data word length */

/** - Data Format 3 */
mibspiREG2->FMT3 = (uint32)((uint32)7U << 24U)  /* wdelay */
                 | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                 | (uint32)((uint32)0U << 22U)  /* parity enable */
                 | (uint32)((uint32)0U << 21U)  /* wait on enable */
                 | (uint32)((uint32)0U << 20U)  /* shift direction */
                 | (uint32)((uint32)0U << 17U)  /* clock polarity */
                 | (uint32)((uint32)1U << 16U)  /* clock phase */
                 | (uint32)((uint32)4U << 8U)  /* baudrate prescale */
                 | (uint32)((uint32)16U << 0U); /* data word length */

/** - Default Chip Select */
mibspiREG2->DEF = (uint32)(0xFFU);

/** - wait for buffer initialization complete before accessing MibSPI registers */
/*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
while ((mibspiREG2->FLG & 0x01000000U) != 0U)
{
} /* Wait */

/** enable MIBSPI RAM Parity */
mibspiREG2->PAR_ECC_CTRL = (mibspiREG2->PAR_ECC_CTRL & 0xFFFFFFF0U) | (0x00000005U);

/** - initialize transfer groups */
mibspiREG2->TGCTRL[0U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)0U << 8U);  /* start buffer */

mibspiREG2->TGCTRL[1U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)1U << 8U);  /* start buffer */

mibspiREG2->TGCTRL[2U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U) << 8U);  /* start buffer */

mibspiREG2->TGCTRL[3U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U+0U) << 8U);  /* start buffer */

mibspiREG2->TGCTRL[4U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U+0U+0U) << 8U);  /* start buffer */

mibspiREG2->TGCTRL[5U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U+0U+0U+0U) << 8U);  /* start buffer */

mibspiREG2->TGCTRL[6U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U+0U+0U+0U+0U) << 8U);  /* start buffer */

mibspiREG2->TGCTRL[7U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                       | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                       | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                       | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                       | (uint32)((uint32)(1U+64U+0U+0U+0U+0U+0U) << 8U);  /* start buffer */


mibspiREG2->TGCTRL[8U] = (uint32)(1U+64U+0U+0U+0U+0U+0U+0U) << 8U;

mibspiREG2->LTGPEND = (mibspiREG2->LTGPEND & 0xFFFF00FFU) | (uint32)(((uint32)(1U+64U+0U+0U+0U+0U+0U+0U)-1U) << 8U);

/** - initialize buffer ram */
{
    i = 0U;

#if (1U > 0U)
    {

#if (1U > 1U)

        while (i < (1U-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */
            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE1].DFMT << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */


        i++;
    }
#endif

#if (64U > 0U)
    {

#if (64U > 1U)

        while (i < ((1U+64U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE1].DFMT << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE1].DFMT << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_2)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_2)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_3)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_3)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U+0U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U+0U+0U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U+0U+0U+0U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

        i++;
    }
#endif

#if (0U > 0U)
    {

#if (0U > 1U)

        while (i < ((1U+64U+0U+0U+0U+0U+0U+0U)-1U))
        {
            mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif
        mibspiRAM2->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                  | (uint16)((uint16)0U << 12U) /* chip select hold */
                                  | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                  | (uint16)((uint16)0U << 8U)  /* data format */
                                  | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */
        i++;
    }
#endif
}

/** - set interrupt levels */
mibspiREG2->LVL = (uint32)((uint32)0U << 9U)  /* TXINT */
                | (uint32)((uint32)0U << 8U)  /* RXINT */
                | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                | (uint32)((uint32)0U << 4U)  /* BITERR */
                | (uint32)((uint32)0U << 3U)  /* DESYNC */
                | (uint32)((uint32)0U << 2U)  /* PARERR */
                | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                | (uint32)((uint32)0U << 0U); /* DLENERR */

/** - clear any pending interrupts */
mibspiREG2->FLG |= 0xFFFFU;

/** - enable interrupts */
mibspiREG2->INT0 = (mibspiREG2->INT0 & 0xFFFF0000U)
                 | (uint32)((uint32)0U << 9U)  /* TXINT */
                 | (uint32)((uint32)0U << 8U)  /* RXINT */
                 | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                 | (uint32)((uint32)0U << 4U)  /* BITERR */
                 | (uint32)((uint32)0U << 3U)  /* DESYNC */
                 | (uint32)((uint32)0U << 2U)  /* PARERR */
                 | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                 | (uint32)((uint32)0U << 0U); /* DLENERR */

/** @b initialize @b MIBSPI2 @b Port */

/** - MIBSPI2 Port output values */
mibspiREG2->PC3 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                | (uint32)((uint32)0U << 8U)  /* ENA */
                | (uint32)((uint32)0U << 9U)  /* CLK */
                | (uint32)((uint32)0U << 10U)  /* SIMO */
                | (uint32)((uint32)0U << 11U); /* SOMI */

/** - MIBSPI2 Port direction */
mibspiREG2->PC1 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                | (uint32)((uint32)1U << 8U)  /* ENA */
                | (uint32)((uint32)0U << 9U)  /* CLK */
                | (uint32)((uint32)0U << 10U)  /* SIMO */
                | (uint32)((uint32)1U << 11U); /* SOMI */

/** - MIBSPI2 Port open drain enable */
mibspiREG2->PC6 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                | (uint32)((uint32)0U << 8U)  /* ENA */
                | (uint32)((uint32)0U << 9U)  /* CLK */
                | (uint32)((uint32)0U << 10U)  /* SIMO */
                | (uint32)((uint32)0U << 11U); /* SOMI */


/** - MIBSPI2 Port pullup / pulldown selection */
mibspiREG2->PC8 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                | (uint32)((uint32)1U << 8U)  /* ENA */
                | (uint32)((uint32)1U << 9U)  /* CLK */
                | (uint32)((uint32)1U << 10U)  /* SIMO */
                | (uint32)((uint32)1U << 11U); /* SOMI */


/** - MIBSPI2 Port pullup / pulldown enable*/
mibspiREG2->PC7 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                | (uint32)((uint32)0U << 8U)  /* ENA */
                | (uint32)((uint32)0U << 9U)  /* CLK */
                | (uint32)((uint32)0U << 10U)  /* SIMO */
                | (uint32)((uint32)0U << 11U); /* SOMI */


/* MIBSPI2 set all pins to functional */
mibspiREG2->PC0 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                | (uint32)((uint32)0U << 8U)  /* ENA */
                | (uint32)((uint32)1U << 9U)  /* CLK */
                | (uint32)((uint32)1U << 10U)  /* SIMO */
                | (uint32)((uint32)1U << 11U); /* SOMI */

/** - Finally start MIBSPI2 */
mibspiREG2->GCR1 = (mibspiREG2->GCR1 & 0xFEFFFFFFU) | 0x01000000U;


}
void mibspi3Init(){
    uint32 i ;
    /** @b initialize @b MIBSPI3 */

   /** bring MIBSPI out of reset */
   mibspiREG3->GCR0 = 0U;
   mibspiREG3->GCR0 = 1U;

   /** enable MIBSPI3 multibuffered mode and enable buffer RAM */
   mibspiREG3->MIBSPIE = (mibspiREG3->MIBSPIE & 0xFFFFFFFEU) | 1U;

   /** MIBSPI3 master mode and clock configuration */
   mibspiREG3->GCR1 = (mibspiREG3->GCR1 & 0xFFFFFFFCU) | ((uint32)((uint32)1U << 1U)  /* CLOKMOD */
                 | 1U);  /* MASTER */

   /** MIBSPI3 enable pin configuration */
   mibspiREG3->INT0 = (mibspiREG3->INT0 & 0xFEFFFFFFU) | (uint32)((uint32)0U << 24U);  /* ENABLE HIGHZ */

   /** - Delays */
   mibspiREG3->DELAY = (uint32)((uint32)8U << 24U)  /* C2TDELAY */
                     | (uint32)((uint32)0U << 16U)  /* T2CDELAY */
                     | (uint32)((uint32)0U << 8U)   /* T2EDELAY */
                     | (uint32)((uint32)0U << 0U);  /* C2EDELAY */

   /** - Data Format 0 */
   mibspiREG3->FMT0 = (uint32)((uint32)7U << 24U)  /* wdelay */
                    | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                    | (uint32)((uint32)0U << 22U)  /* parity enable */
                    | (uint32)((uint32)0U << 21U)  /* wait on enable */
                    | (uint32)((uint32)0U << 20U)  /* shift direction */
                    | (uint32)((uint32)0U << 17U)  /* clock polarity */
                    | (uint32)((uint32)0U << 16U)  /* clock phase */
                    | (uint32)((uint32)97U << 8U)  /* baudrate prescale */
                    | (uint32)((uint32)16U << 0U); /* data word length */

   /** - Data Format 1 */
   mibspiREG3->FMT1 = (uint32)((uint32)7U << 24U)  /* wdelay */
                    | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                    | (uint32)((uint32)0U << 22U)  /* parity enable */
                    | (uint32)((uint32)0U << 21U)  /* wait on enable */
                    | (uint32)((uint32)0U << 20U)  /* shift direction */
                    | (uint32)((uint32)0U << 17U)  /* clock polarity */
                    | (uint32)((uint32)0U << 16U)  /* clock phase */
                    | (uint32)((uint32)19U << 8U)  /* baudrate prescale */
                    | (uint32)((uint32)16U << 0U); /* data word length */

   /** - Data Format 2 */
   mibspiREG3->FMT2 = (uint32)((uint32)7U << 24U)  /* wdelay */
                    | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                    | (uint32)((uint32)0U << 22U)  /* parity enable */
                    | (uint32)((uint32)0U << 21U)  /* wait on enable */
                    | (uint32)((uint32)0U << 20U)  /* shift direction */
                    | (uint32)((uint32)0U << 17U)  /* clock polarity */
                    | (uint32)((uint32)0U << 16U)  /* clock phase */
                    | (uint32)((uint32)9U << 8U)  /* baudrate prescale */
                    | (uint32)((uint32)16U << 0U); /* data word length */

   /** - Data Format 3 */
   mibspiREG3->FMT3 = (uint32)((uint32)7U << 24U)  /* wdelay */
                    | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                    | (uint32)((uint32)0U << 22U)  /* parity enable */
                    | (uint32)((uint32)0U << 21U)  /* wait on enable */
                    | (uint32)((uint32)0U << 20U)  /* shift direction */
                    | (uint32)((uint32)0U << 17U)  /* clock polarity */
                    | (uint32)((uint32)0U << 16U)  /* clock phase */
                    | (uint32)((uint32)4U << 8U)  /* baudrate prescale */
                    | (uint32)((uint32)16U << 0U); /* data word length */

   /** - Default Chip Select */
   mibspiREG3->DEF = (uint32)(0xFFU);

   /** - wait for buffer initialization complete before accessing MibSPI registers */
   /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
   while ((mibspiREG3->FLG & 0x01000000U) != 0U)
   {
   } /* Wait */

   /** enable MIBSPI RAM Parity */
   mibspiREG3->PAR_ECC_CTRL = (mibspiREG3->PAR_ECC_CTRL & 0xFFFFFFF0U) | (0x00000005U);

   /** - initialize transfer groups */
   mibspiREG3->TGCTRL[0U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)0U << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[1U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)1U << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[2U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U) << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[3U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U+0U) << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[4U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U+0U+0U) << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[5U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U+0U+0U+0U) << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[6U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U+0U+0U+0U+0U) << 8U);  /* start buffer */

   mibspiREG3->TGCTRL[7U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                          | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                          | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                          | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                          | (uint32)((uint32)(1U+64U+0U+0U+0U+0U+0U) << 8U);  /* start buffer */


   mibspiREG3->TGCTRL[8U] = (uint32)(1U+64U+0U+0U+0U+0U+0U+0U) << 8U;

   mibspiREG3->LTGPEND = (mibspiREG3->LTGPEND & 0xFFFF00FFU) | (uint32)(((uint32)(1U+64U+0U+0U+0U+0U+0U+0U)-1U) << 8U);

   /** - initialize buffer ram */
   {
       i = 0U;

#if (1U > 0U)
       {

#if (1U > 1U)

           while (i < (1U-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */
               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE2].DFMT << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */


           i++;
       }
#endif

#if (64U > 0U)
       {

#if (64U > 1U)

           while (i < ((1U+64U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE2].DFMT << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)1U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)io_spi_devices_handle_table[SPI_DEVICE2].DFMT << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_1)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_1)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_1)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)1U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_1)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U+0U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U+0U+0U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U+0U+0U+0U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

           i++;
       }
#endif

#if (0U > 0U)
       {

#if (0U > 1U)

           while (i < ((1U+64U+0U+0U+0U+0U+0U+0U)-1U))
           {
               mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                         | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                         | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                         | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                         | (uint16)((uint16)0U << 8U)  /* data format */
                                         | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */

               i++;
           }
#endif
           mibspiRAM3->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                     | (uint16)((uint16)0U << 12U) /* chip select hold */
                                     | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                     | (uint16)((uint16)0U << 8U)  /* data format */
                                     | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */
           i++;
       }
#endif
   }

   /** - set interrupt levels */
   mibspiREG3->LVL = (uint32)((uint32)0U << 9U)  /* TXINT */
                   | (uint32)((uint32)0U << 8U)  /* RXINT */
                   | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                   | (uint32)((uint32)0U << 4U)  /* BITERR */
                   | (uint32)((uint32)0U << 3U)  /* DESYNC */
                   | (uint32)((uint32)0U << 2U)  /* PARERR */
                   | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                   | (uint32)((uint32)0U << 0U); /* DLENERR */

   /** - clear any pending interrupts */
   mibspiREG3->FLG |= 0xFFFFU;

   /** - enable interrupts */
   mibspiREG3->INT0 = (mibspiREG3->INT0 & 0xFFFF0000U)
                    | (uint32)((uint32)0U << 9U)  /* TXINT */
                    | (uint32)((uint32)0U << 8U)  /* RXINT */
                    | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                    | (uint32)((uint32)0U << 4U)  /* BITERR */
                    | (uint32)((uint32)0U << 3U)  /* DESYNC */
                    | (uint32)((uint32)0U << 2U)  /* PARERR */
                    | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                    | (uint32)((uint32)0U << 0U); /* DLENERR */

   /** @b initialize @b MIBSPI3 @b Port */

   /** - MIBSPI3 Port output values */
   mibspiREG3->PC3 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)1U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)1U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)0U << 8U)  /* ENA */
                   | (uint32)((uint32)0U << 9U)  /* CLK */
                   | (uint32)((uint32)0U << 10U)  /* SIMO */
                   | (uint32)((uint32)0U << 11U); /* SOMI */

   /** - MIBSPI3 Port direction */
   mibspiREG3->PC1 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)1U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)1U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)0U << 8U)  /* ENA */
                   | (uint32)((uint32)1U << 9U)  /* CLK */
                   | (uint32)((uint32)1U << 10U)  /* SIMO */
                   | (uint32)((uint32)0U << 11U); /* SOMI */

   /** - MIBSPI3 Port open drain enable */
   mibspiREG3->PC6 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)0U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)0U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)0U << 8U)  /* ENA */
                   | (uint32)((uint32)0U << 9U)  /* CLK */
                   | (uint32)((uint32)0U << 10U)  /* SIMO */
                   | (uint32)((uint32)0U << 11U); /* SOMI */


   /** - MIBSPI3 Port pullup / pulldown selection */
   mibspiREG3->PC8 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)1U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)1U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)1U << 8U)  /* ENA */
                   | (uint32)((uint32)1U << 9U)  /* CLK */
                   | (uint32)((uint32)1U << 10U)  /* SIMO */
                   | (uint32)((uint32)1U << 11U); /* SOMI */


   /** - MIBSPI3 Port pullup / pulldown enable*/
   mibspiREG3->PC7 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)0U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)0U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)0U << 8U)  /* ENA */
                   | (uint32)((uint32)0U << 9U)  /* CLK */
                   | (uint32)((uint32)0U << 10U)  /* SIMO */
                   | (uint32)((uint32)0U << 11U); /* SOMI */


   /* MIBSPI3 set all pins to functional */
   mibspiREG3->PC0 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                   | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                   | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                   | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                   | (uint32)((uint32)0U << 4U)  /* SCS[4] */
                   | (uint32)((uint32)0U << 5U)  /* SCS[5] */
                   | (uint32)((uint32)0U << 8U)  /* ENA */
                   | (uint32)((uint32)1U << 9U)  /* CLK */
                   | (uint32)((uint32)1U << 10U)  /* SIMO */
                   | (uint32)((uint32)1U << 11U); /* SOMI */

   /** - Finally start MIBSPI3 */
   mibspiREG3->GCR1 = (mibspiREG3->GCR1 & 0xFEFFFFFFU) | 0x01000000U;
}
void mibspi5Init(){
    uint32 i ;
    /** @b initialize @b MIBSPI5 */

    /** bring MIBSPI out of reset */
    mibspiREG5->GCR0 = 0U;
    mibspiREG5->GCR0 = 1U;

    /** enable MIBSPI5 multibuffered mode and enable buffer RAM */
    mibspiREG5->MIBSPIE = (mibspiREG5->MIBSPIE & 0xFFFFFFFEU) | 1U;

    /** MIBSPI5 master mode and clock configuration */
    mibspiREG5->GCR1 = (mibspiREG5->GCR1 & 0xFFFFFFFCU) | ((uint32)((uint32)0U << 1U)  /* CLOKMOD */
                  | 0U);  /* MASTER */

    /** MIBSPI5 enable pin configuration */
    mibspiREG5->INT0 = (mibspiREG5->INT0 & 0xFEFFFFFFU) | (uint32)((uint32)0U << 24U);  /* ENABLE HIGHZ */

    /** - Delays */
    mibspiREG5->DELAY = (uint32)((uint32)8U << 24U)  /* C2TDELAY */
                      | (uint32)((uint32)0U << 16U)  /* T2CDELAY */
                      | (uint32)((uint32)0U << 8U)   /* T2EDELAY */
                      | (uint32)((uint32)0U << 0U);  /* C2EDELAY */

    /** - Data Format 0 */
    mibspiREG5->FMT0 = (uint32)((uint32)0U << 24U)  /* wdelay */
                     | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                     | (uint32)((uint32)0U << 22U)  /* parity enable */
                     | (uint32)((uint32)0U << 21U)  /* wait on enable */
                     | (uint32)((uint32)0U << 20U)  /* shift direction */
                     | (uint32)((uint32)0U << 17U)  /* clock polarity */
                     | (uint32)((uint32)0U << 16U)  /* clock phase */
                     | (uint32)((uint32)3U << 8U)  /* baudrate prescale */
                     | (uint32)((uint32)16U << 0U); /* data word length */

    /** - Data Format 1 */
    mibspiREG5->FMT1 = (uint32)((uint32)0U << 24U)  /* wdelay */
                     | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                     | (uint32)((uint32)0U << 22U)  /* parity enable */
                     | (uint32)((uint32)0U << 21U)  /* wait on enable */
                     | (uint32)((uint32)0U << 20U)  /* shift direction */
                     | (uint32)((uint32)0U << 17U)  /* clock polarity */
                     | (uint32)((uint32)0U << 16U)  /* clock phase */
                     | (uint32)((uint32)7U << 8U)  /* baudrate prescale */
                     | (uint32)((uint32)16U << 0U); /* data word length */

    /** - Data Format 2 */
    mibspiREG5->FMT2 = (uint32)((uint32)0U << 24U)  /* wdelay */
                     | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                     | (uint32)((uint32)0U << 22U)  /* parity enable */
                     | (uint32)((uint32)0U << 21U)  /* wait on enable */
                     | (uint32)((uint32)0U << 20U)  /* shift direction */
                     | (uint32)((uint32)0U << 17U)  /* clock polarity */
                     | (uint32)((uint32)0U << 16U)  /* clock phase */
                     | (uint32)((uint32)97U << 8U)  /* baudrate prescale */
                     | (uint32)((uint32)16U << 0U); /* data word length */

    /** - Data Format 3 */
    mibspiREG5->FMT3 = (uint32)((uint32)0U << 24U)  /* wdelay */
                     | (uint32)((uint32)0U << 23U)  /* parity Polarity */
                     | (uint32)((uint32)0U << 22U)  /* parity enable */
                     | (uint32)((uint32)0U << 21U)  /* wait on enable */
                     | (uint32)((uint32)0U << 20U)  /* shift direction */
                     | (uint32)((uint32)0U << 17U)  /* clock polarity */
                     | (uint32)((uint32)0U << 16U)  /* clock phase */
                     | (uint32)((uint32)97U << 8U)  /* baudrate prescale */
                     | (uint32)((uint32)16U << 0U); /* data word length */

    /** - Default Chip Select */
    mibspiREG5->DEF = (uint32)(0xFFU);

    /** - wait for buffer initialization complete before accessing MibSPI registers */
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
    while ((mibspiREG5->FLG & 0x01000000U) != 0U)
    {
    } /* Wait */

    /** enable MIBSPI RAM Parity */
    mibspiREG5->PAR_ECC_CTRL = (mibspiREG5->PAR_ECC_CTRL & 0xFFFFFFF0U) | (0x00000005U);

    /** - initialize transfer groups */
    mibspiREG5->TGCTRL[0U] = (uint32)((uint32)0U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)0U << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[1U] = (uint32)((uint32)0U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)1U << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[2U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U) << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[3U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U+0U) << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[4U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U+0U+0U) << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[5U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U+0U+0U+0U) << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[6U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U+0U+0U+0U+0U) << 8U);  /* start buffer */

    mibspiREG5->TGCTRL[7U] = (uint32)((uint32)1U << 30U)  /* oneshot */
                           | (uint32)((uint32)0U << 29U)  /* pcurrent reset */
                           | (uint32)((uint32)TRG_ALWAYS << 20U)  /* trigger event */
                           | (uint32)((uint32)TRG_DISABLED << 16U)  /* trigger source */
                           | (uint32)((uint32)(1U+0U+0U+0U+0U+0U+0U) << 8U);  /* start buffer */


    mibspiREG5->TGCTRL[8U] = (uint32)(1U+0U+0U+0U+0U+0U+0U+0U) << 8U;

    mibspiREG5->LTGPEND = (mibspiREG5->LTGPEND & 0xFFFF00FFU) | (uint32)(((uint32)(1U+0U+0U+0U+0U+0U+0U+0U)-1U) << 8U);

    /** - initialize buffer ram */
    {
        i = 0U;

#if (1U > 0U)
        {

#if (1U > 1U)

            while (i < (1U-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */
                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)1U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */


            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)1U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)3U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)1U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_0)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_2)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_2)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_3)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_3)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_4)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U+0U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_5)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U+0U+0U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_6)) & (uint16)0x00FFU);  /* chip select */

            i++;
        }
#endif

#if (0U > 0U)
        {

#if (0U > 1U)

            while (i < ((1U+0U+0U+0U+0U+0U+0U+0U)-1U))
            {
                mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                          | (uint16)((uint16)0U << 12U)  /* chip select hold */
                                          | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                          | (uint16)((uint16)0U << 11U)  /* lock transmission */
                                          | (uint16)((uint16)0U << 8U)  /* data format */
                                          | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */

                i++;
            }
#endif
            mibspiRAM5->tx[i].control = (uint16)((uint16)4U << 13U)  /* buffer mode */
                                      | (uint16)((uint16)0U << 12U) /* chip select hold */
                                      | (uint16)((uint16)0U << 10U)  /* enable WDELAY */
                                      | (uint16)((uint16)0U << 8U)  /* data format */
                                      | ((uint16)(~((uint16)0xFFU ^ (uint16)CS_7)) & (uint16)0x00FFU);  /* chip select */
            i++;
        }
#endif
    }

    /** - set interrupt levels */
    mibspiREG5->LVL = (uint32)((uint32)0U << 9U)  /* TXINT */
                    | (uint32)((uint32)0U << 8U)  /* RXINT */
                    | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                    | (uint32)((uint32)0U << 4U)  /* BITERR */
                    | (uint32)((uint32)0U << 3U)  /* DESYNC */
                    | (uint32)((uint32)0U << 2U)  /* PARERR */
                    | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                    | (uint32)((uint32)0U << 0U); /* DLENERR */

    /** - clear any pending interrupts */
    mibspiREG5->FLG |= 0xFFFFU;

    /** - enable interrupts */
    mibspiREG5->INT0 = (mibspiREG5->INT0 & 0xFFFF0000U)
                     | (uint32)((uint32)0U << 9U)  /* TXINT */
                     | (uint32)((uint32)0U << 8U)  /* RXINT */
                     | (uint32)((uint32)0U << 6U)  /* OVRNINT */
                     | (uint32)((uint32)0U << 4U)  /* BITERR */
                     | (uint32)((uint32)0U << 3U)  /* DESYNC */
                     | (uint32)((uint32)0U << 2U)  /* PARERR */
                     | (uint32)((uint32)0U << 1U)  /* TIMEOUT */
                     | (uint32)((uint32)0U << 0U); /* DLENERR */

    /** @b initialize @b MIBSPI5 @b Port */

    /** - MIBSPI5 Port output values */
    mibspiREG5->PC3 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)1U << 8U)  /* ENA */
                    | (uint32)((uint32)0U << 9U)  /* CLK */
                    | (uint32)((uint32)0U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)0U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)0U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)0U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)0U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)0U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)0U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)0U << 27U); /* SOMI[3] */

    /** - MIBSPI5 Port direction */
    mibspiREG5->PC1 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)1U << 8U)  /* ENA */
                    | (uint32)((uint32)0U << 9U)  /* CLK */
                    | (uint32)((uint32)0U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)1U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)0U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)0U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)0U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)1U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)1U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)1U << 27U); /* SOMI[3] */

    /** - MIBSPI5 Port open drain enable */
    mibspiREG5->PC6 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)0U << 8U)  /* ENA */
                    | (uint32)((uint32)0U << 9U)  /* CLK */
                    | (uint32)((uint32)0U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)0U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)0U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)0U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)0U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)0U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)0U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)0U << 27U); /* SOMI[3] */

    /** - MIBSPI5 Port pullup / pulldown selection */
    mibspiREG5->PC8 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)1U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)1U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)1U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)1U << 8U)  /* ENA */
                    | (uint32)((uint32)1U << 9U)  /* CLK */
                    | (uint32)((uint32)1U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)1U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)1U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)1U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)1U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)1U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)1U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)1U << 27U); /* SOMI[3] */

    /** - MIBSPI5 Port pullup / pulldown enable*/
    mibspiREG5->PC7 = (uint32)((uint32)0U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)0U << 8U)  /* ENA */
                    | (uint32)((uint32)0U << 9U)  /* CLK */
                    | (uint32)((uint32)0U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)0U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)0U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)0U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)0U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)0U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)0U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)0U << 27U); /* SOMI[3] */

    /* MIBSPI5 set all pins to functional */
    mibspiREG5->PC0 = (uint32)((uint32)1U << 0U)  /* SCS[0] */
                    | (uint32)((uint32)0U << 1U)  /* SCS[1] */
                    | (uint32)((uint32)0U << 2U)  /* SCS[2] */
                    | (uint32)((uint32)0U << 3U)  /* SCS[3] */
                    | (uint32)((uint32)0U << 8U)  /* ENA */
                    | (uint32)((uint32)1U << 9U)  /* CLK */
                    | (uint32)((uint32)1U << 10U)  /* SIMO[0] */
                    | (uint32)((uint32)1U << 11U)  /* SOMI[0] */
                    | (uint32)((uint32)1U << 17U)  /* SIMO[1] */
                    | (uint32)((uint32)1U << 18U)  /* SIMO[2] */
                    | (uint32)((uint32)1U << 19U)  /* SIMO[3] */
                    | (uint32)((uint32)1U << 25U)  /* SOMI[1] */
                    | (uint32)((uint32)1U << 26U)  /* SOMI[2] */
                    | (uint32)((uint32)1U << 27U); /* SOMI[3] */

  /** - Finally start MIBSPI5 */
    mibspiREG5->GCR1 = (mibspiREG5->GCR1 & 0xFEFFFFFFU) | 0x01000000U;


}
