/*
 * obc_io.c
 *
 *  Created on: Aug 23, 2022
 *      Author: Ben Deng
 */

/* header files */
#include <FreeRTOS.h>
#include <os_task.h>
#include "global.h"
#include "HL_can.h"
#include "HL_gio.h"
#include "HL_i2c.h"
#include "HL_sci.h"
#include "HL_spi.h"
#include "HL_het.h"
#include "HL_emif.h"
#include "HL_reg_het.h"
#include "HL_reg_gio.h"
#include "HL_adc.h"
#include "HL_mibspi.h"
#include "HL_reg_mibspi.h"
#include "HL_reg_i2c.h"
//#include "redposix.h"
#include "HL_sys_core.h"
#include "xioctl.h"


void dmaEnable(void);

/* main.c ------------------------------------------------------------------- */
// Initialize all IO controllers.
void Init_IO(void)
{
    _enable_IRQ();

    sciInit();
//    uart_init(UART_COM1);
//    uart_init(UART_COM2);
//    uart_init(UART_COM3);
//    uart_init(UART_COM4);

    adcInit();
    canInit();

    spiInit();
    mibspiInit();

    i2cInit();
    gioInit();
    gpio_init();
    global_init();

    dmaEnable();
    // emif_SDRAMInit(); not using until SDRAM tested
    //hetInit();

    /*
    // The following sets the proper direction for all GPIO
    gioSetDirection(hetPORT2, 0xFFFFFFEA);
#if 1
    gioSetDirection(hetPORT1, 0x9CFF6BEF);
#else
    gioSetDirection(hetPORT1, 0x9CFF7BEF);
#endif
    gioSetBit(hetPORT2, 12, 1);
    gioSetBit(hetPORT1, 20, 1);
#if 1
    gioSetBit(hetPORT2, 6, 1);
    gioSetBit(gioPORTA, 3, 1);
#else
    gioSetBit(hetPORT1, 12, 1);
    gioSetBit(hetPORT1, 14, 1);
#endif
*/
}


/*  @param[in] bit number 0-13 that specifies the output GPIO to be written to.

*   @param[in] value binary value to write to GPIO
*
*   Writes a value to the specified GPIO pin
*
*   GPIO0 - 2_23
*   GPIO1 - 1_25 - input
*   GPIO2 - 2_21
*   GPIO3 - 1_9
*   GPIO4 - 1_3
*   GPIO5 - 1_7
*   GPIO6 - 2_22
*   GPIO7 - 1_1
*   GPIO8 - 1_24 - input
*   GPIO9 - 1_20
*   GPIO10 - A7 - input
*   GPIO11 - 1_15 - input
*   GPIO12 - B1
*   GPIO13 - B0 */
/*int GPIOSetBit(uint8_t bit, uint8_t value) {
    switch (bit) {
    case 0:
        gioSetBit(hetPORT2, 23, value);
        break;
    case 1:
        while (1)
            ; // cannot write to input
    case 2:
        gioSetBit(hetPORT2, 21, value);
        break;
    case 3:
        gioSetBit(hetPORT1, 9, value);
        break;
    case 4:
        gioSetBit(hetPORT1, 3, value);
        break;
    case 5:
        gioSetBit(hetPORT1, 7, value);
        break;
    case 6:
        gioSetBit(hetPORT2, 22, value);
        break;
    case 7:
        gioSetBit(hetPORT1, 1, value);
        break;
    case 8:
        while (1)
            ; // cannot write to input
    case 9:
        gioSetBit(hetPORT1, 20, value);
        break;
    case 10:
        while (1)
            ; // cannot write to input
    case 11:
        while (1)
            ; // cannot write to input
    case 12:
        gioSetBit(gioPORTB, 1, value);
        break;
    case 13:
        gioSetBit(gioPORTB, 0, value);
        break;
    default:
        return -1;
    }
    return 0;
}*/

/*  @param[in] bit number 0-13 that specifies the GPIO to be read.

*   @param[in] value binary value read from GPIO
*
*   Writes a value to the specified GPIO pin
*
*   GPIO0 - 2_23
*   GPIO1 - 1_25 - input
*   GPIO2 - 2_21
*   GPIO3 - 1_9
*   GPIO4 - 1_3
*   GPIO5 - 1_7
*   GPIO6 - 2_22
*   GPIO7 - 1_1
*   GPIO8 - 1_24 - input
*   GPIO9 - 1_20
*   GPIO10 - A7 - input
*   GPIO11 - 1_15 - input
*   GPIO12 - B1
*   GPIO13 - B0 */
/*int GPIOGetBit(uint8_t bit) {

    switch (bit) {
    case 0:
        return gioGetBit(hetPORT2, 23);
    case 1:
        return gioGetBit(hetPORT1, 25);
    case 2:
        return gioGetBit(hetPORT2, 21);
    case 3:
        return gioGetBit(hetPORT1, 9);
    case 4:
        return gioGetBit(hetPORT1, 3);
    case 5:
        return gioGetBit(hetPORT1, 7);
    case 6:
        return gioGetBit(hetPORT2, 22);
    case 7:
        return gioGetBit(hetPORT1, 1);
    case 8:
        return gioGetBit(hetPORT1, 24);
    case 9:
        return gioGetBit(hetPORT1, 20);
    case 10:
        return gioGetBit(gioPORTA, 7);
    case 11:
        return gioGetBit(hetPORT1, 15);
    case 12:
        return gioGetBit(gioPORTB, 1);
    case 13:
        return gioGetBit(gioPORTB, 0);
    default:
        return -1;
    }
}*/

/* Rapidly alternates all output GPIOs on stack header on and off once
 */
/*void GPIOTxTest(void) {
    int i;
    for (i = 0; i <= 13; i++) {
        if ((i == 1) || (i == 8) || (i == 10) || (i == 11)) { // make sure inputs are not toggled
            // do nothing
        } else {
            GPIOSetBit(i, 1);
        }
    }
    for (i = 0; i <= 13; i++) {
        if ((i == 1) || (i == 8) || (i == 10) || (i == 11)) { // make sure inputs are not toggled
            // do nothing
        } else {
            GPIOSetBit(i, 0);
        }
    }
} */


/* Testing UART Communication ----------------------------------------------- */

void UARTTxTest(sciBASE_t *regset)
{
    // Description:
    // UARTTxTest tests if UART Transmitter is working properly.
    //
    // input .:. [regset] SCI UART register module address
    //
    _enable_IRQ();
    sciEnableNotification(regset, SCI_TX_INT);

    int i;
    for (i = 0; i < 10; i++)
    {
        sciSendByte(regset, 0b00110101);
        printk(".");
        vTaskDelay(5); // Delay is included here because the Tx is faster than Rx, then Rx could not count properly.
    }
}

uint8_t UARTRxTest(sciBASE_t *regset)
{
    // Description:
    // Receives 1000 bytes of pattern 00110101 over UART
    // Make sure that this is running before the master sends data.
    //
    // input .:. [regset] SCI UART register module address
    // output .:. [pings] the number of bytes successfully read. The result should be 100.
    //
    _enable_IRQ();

    uint8_t data;
    int i = 0;
    uint8_t pings = 0;

    while (i < 10)
    {
       data = 0;
       if (sciIsRxReady(regset) != 0)
       {
       }
       else
       {
           data = sciReceiveByte(regset);
           if (data == 0b00110101)
           {
               pings++;
           }
       }
       i++;
       printk(".");
    }
    return pings;
}

uint8_t UARTloopback(sciBASE_t *regset)
{
    _enable_IRQ();

    uint8_t data;
    int i = 0;
    uint8_t pings = 0;

    while (i < 10)
    {
       data = 0;
       //sciSendByte(regset, 0b00110101);
       if (sciIsRxReady(regset) != 0)
       {
           //printk("Rx is not ready! \n");
       }
       else
       {
           data = CN20_loopback_test(regset, 0b00110101);
           if (data == 0b00110101)
           {
               //printk("I received it! \n");
               pings++;
           }
       }
       i++;
       printk(".");
    }
    return pings;
}


// SPI Communication ------------------------------------------------- *

// SPI Master Tx
void SPIMasterTxTest(spiBASE_t *regset)
{
    // struct is implemented in the TI HAL and uses uppercase true and false
    spiDAT1_t dataconfig1_t;

    dataconfig1_t.CS_HOLD = FALSE; // If true, HW chip select kept active between words
    dataconfig1_t.WDEL = TRUE; // activation of delay between transmission words
    dataconfig1_t.DFSEL = SPI_FMT_0; // data word format selection 0U, 1U, 2U, 3U
    dataconfig1_t.CSNR = 0xFE; // 0xFE => CS0 // 0xFD => CS1 // 0xFB => CS2 // 0xF7 => CS3 // 0xEF => CS4 // 0xDF => CS5 // 0xBF => CS6 // 0x7F => CS7

    uint16_t data = 0b0011010100110101;
    int i;
    for (i=0; i< 10; i++)
    {
        spiTransmitData(regset, &dataconfig1_t, 1, &data);
        printk(".");
        vTaskDelay(5);
    }
}

// SPI Master Rx
//uint8_t SPIMasterRxTest(spiBASE_t *regset)
//{
//    int i;
//    spi_read(SPI_DEVICE2, 4, spi_rx_buffer);
//    vTaskDelay(20);
//
////    uint8_t data[8] = {0};
////    printk("before spi receive data = %x \n",data);
////    spiReceiveData(regset, &dataconfig1_t,  8, &data[0]);
//    for(i=0; i<16; i++){
//        printk("after spi receive data = %x\n",spi_rx_buffer[i]);
//    }
//    return 0;
//}
uint8_t SPIMasterRxTest(spiBASE_t *regset)
{
    spiDAT1_t dataconfig1_t;

    dataconfig1_t.CS_HOLD = FALSE;
    dataconfig1_t.WDEL = TRUE;
    dataconfig1_t.DFSEL = SPI_FMT_0;
    dataconfig1_t.CSNR = 0xFE;

    uint16_t data;
    uint8_t i = 0;
    uint8_t pings = 0;

    while (i < 10)
    {
        data = 0;
        spiReceiveData(regset, &dataconfig1_t, 1, &data);
        if (data == 0b0011010100110101)
        {
            pings++;
            printk(".");
            //vTaskDelay(15);
        }
        i++;
    }
    return pings;
}

// SPI Slave Tx
void SPISlaveTxTest(spiBASE_t *regset)
{
    // Run this on a dev board to send data to OBC100
    // Make sure to set the board to be a slave
    //
    spiDAT1_t dataconfig1_t;

    dataconfig1_t.CS_HOLD = FALSE;
    dataconfig1_t.WDEL = TRUE;
    dataconfig1_t.DFSEL = SPI_FMT_0;
    dataconfig1_t.CSNR = 0xFE;

    uint16_t data = 0b0011010100110101;
    int i;
    for (i = 0; i < 10; i++)
    {
        spiTransmitData(regset, &dataconfig1_t, 1, &data);
        printk(".");
        //vTaskDelay(5);
    }
}

// SPI Slave Rx
uint8_t SPISlaveRxTest(spiBASE_t *regset, uint8 spinumber)
{
    //uint16_t TX_Data_Slave[16] = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };
    //uint16_t RX_Data_Slave [16] = { 0 };

    spiDAT1_t dataconfig1_t;

    dataconfig1_t.CS_HOLD = FALSE;
    dataconfig1_t.WDEL = TRUE;
    dataconfig1_t.DFSEL = SPI_FMT_0;
    dataconfig1_t.CSNR = 0xFE;

    //spiInit();
    //spiSendAndGetData(spiREG5, &dataconfig1_t, 16, TX_Data_Slave, RX_Data_Slave);
    //spiReceiveData(spiREG5, &dataconfig1_t, 16, RX_Data_Slave);
    //return RX_Data_Slave[1];

    uint16_t data;
    uint8_t i = 0;
    uint8_t pings = 0;

    while (i < 10)
    {
        data = 0;
        spiReceiveData(regset, &dataconfig1_t, 1, &data);
        if (data == 0b0011010100110101)
        {
            pings++;
            printk(".");
            vTaskDelay(5);
        }
        i++;
    }
    return i;
}

// I2C Communication ------------------------------------------------- *
// I2C Master Tx
void I2CMasterTxTest(i2cBASE_t *regset, uint8_t slave_addr)
{

     // Send 1000 bytes of byte 00110101 as an I2C master.
     // Make sure that another board is ready to receive data before running this.
     //
     // [in] regset -> I2C register to test
     // [in] addr -> I2C slave address to send data to
    i2cInit();

    i2cSetSlaveAdd(regset, slave_addr);

    i2cSetDirection(regset, I2C_TRANSMITTER);
    i2cSetCount(regset, 10);
    i2cSetMode(regset, I2C_MASTER);
    i2cSetStop(regset);
    i2cSetStart(regset);

    int i;
    for (i = 0; i < 10; i++)
    {
        i2cSendByte(regset, 0b00110101);
        printk(".");
        vTaskDelay(5);
    }

    // Wait until Bus Busy is cleared
    while (i2cIsBusBusy(regset) == true)
        ;

    // Wait until Stop is detected
    while (i2cIsStopDetected(regset) == 0)
        ;

    // Clear the Stop condition
    i2cClearSCD(regset);
}

// I2C Master Rx
uint8_t I2CMasterRxTest(i2cBASE_t *regset, uint8_t slave_addr)
{

     // Receives 1000 bytes of byte 00110101 as an I2C master.
     // Make sure that another board is ready to send data before running this.
     //
     // [in] regset -> I2C register to test
     // [in] addr -> I2C slave address to receive data from
     // [out] return the number of successfully read bytes
    i2cInit();

    uint8_t data = 0;

    i2cSetSlaveAdd(regset, slave_addr);
    i2cSetDirection(regset, I2C_RECEIVER);
    i2cSetCount(regset, 10);
    i2cSetMode(regset, I2C_MASTER);
    i2cSetStop(regset);
    i2cSetStart(regset);

    int i = 0;
    int pings = 0;
    while (pings < 10)
    {
        if (i2cIsRxReady(regset) != 0)
        {
            data = 0;
            data = i2cReceiveByte(regset);
            if (data == 0b00110101)
            {
                pings++;
                printk(".");
            }
            i++;
        }
    }

    // Wait until Bus Busy is clear
    while (i2cIsBusBusy(regset) == true)
        ;

    // Wait until Stop is detected
    while (i2cIsStopDetected(regset) == 0)
        ;

    // Clear the Stop condition
    i2cClearSCD(regset);

    return pings;
}

// I2C Slave Rx
uint8_t I2CSlaveRxTest(i2cBASE_t *regset, uint8_t slave_addr)
{

     // Receives 1000 bytes of byte 00110101 as an I2C slave.
     // Make sure that this is running before the master sends data.
     //
     // [in] regset -> I2C register to test
     // [in] addr -> sets slave(self) address to this value
     // [out] return the number of successfully read bytes

    i2cInit();  // Initialize i2c driver

    i2cSetMode(regset, I2C_SLAVE);
    i2cSetOwnAdd(regset, slave_addr);
    i2cSetDirection(regset, I2C_RECEIVER);
    i2cSetCount(regset, 10);

    uint8_t data = 0;
    int i = 0;
    int pings = 0;

    while (i < 10)
    {
        if (i2cIsRxReady(regset) != 0)
        {
            data = 0;
            data = i2cReceiveByte(regset);
            if (data == 0b00110101)
            {
                pings++;
                printk(".");
            }
            i++;
        }
    }

    // Wait until Bus Busy is cleared
    while (i2cIsBusBusy(regset) == true)
        ;

    // Wait until Stop is detected
    while (i2cIsStopDetected(regset) == 0)
        ;

    // Clear the stop condition
    i2cClearSCD(regset);

    return pings;
}

// I2C Slave Tx
void I2CSlaveTxTest(i2cBASE_t *regset, uint8_t addr)
{

     // Transmits 1000 bytes of byte 00110101 as an I2C master.
     // Make sure that this is running before the master requests data.
     //
     // [in] regset -> I2C register to test
     // [in] addr -> sets slave (self) address to this value

    i2cInit();

    i2cSetMode(regset, I2C_SLAVE);
    i2cSetOwnAdd(regset, addr);
    i2cSetDirection(regset, I2C_TRANSMITTER);
    i2cSetCount(regset, 10);

    int i;
    for (i = 0; i < 10; i++)
    {
        i2cSendByte(regset, 0b00110101);
        printk(".");
        vTaskDelay(5);
    }

    // Wait until Bus Busy is cleared
    while (i2cIsBusBusy(regset) == true)
        ;

    // Wait until Stop is detected
    while (i2cIsStopDetected(regset) == 0)
        ;

    // Clear the Stop condition
    i2cClearSCD(regset);
}

/*void I2CTest(i2cBASE_t *i2creg, uint8_t addr)
{
    i2cInit();
    i2cSetOwnAdd(i2creg, slave_address);
    i2cSetDirection(i2creg, I2C_RECEIVER);
    memset( &RX_Data_Sla, 0, sizeof(RX_Data_Sla));
    i2cSetCount(i2creg, 6);
    i2cReceive(i2creg, 6, RX_Data_Sla);
    i2cSetCount(i2creg, 0);
    i2cSend(i2creg, 0, i2c_long_buff);

    while (i2cIsBusBusy(i2creg) == true);

    while (i2cIsStopDetected(i2creg) == 0);

    i2cClearSCD(i2creg);
} */


/* CAN Communication -------------------------------------------------
// CAN bus Tx
void CANTxTest(canBASE_t *regset, uint8_t msgbox)
{
    /*
     * Sends 1000 bytes (125 CAN frames) of 00110101
     * Make sure that another bord is ready to receive data before running this.
     *
     * [in] regset -> CAN register to test
     * [in] msgbox -> Message box to use for Tx. Typically canMESSAGE_BOX1.
     *
    int i;
    uint8_t data[8] = {53, 53, 53, 53, 53, 53, 53, 53};
    for (i = 0; i < 125; i++)
    {
        while (canIsTxMessagePending(regset, msgbox))
            ;
        canTransmit(regset, msgbox, data);
    }
}

// CAN bus Rx
uint8_t CANRxTest(canBASE_t *regset, uint8_t msgbox)
{
    /*
     * Receives 1000 bytes (125 CAN frames) of 00110101
     * Make sure to run this before message is sent from another board.
     *
     * [in] regset -> CAN register to test
     * [in] msgbox -> Message box to use for Tx. Typically canMESSAGE_BOX2.
     * [out] return number of successfully received packets (should be 125)
     *
    uint8_t data[8] = {0};
    int i = 0;
    int j = 0;
    int pings = 0;

    while (i < 125)
    {
        while (!canIsRxMessageArrived(regset, msgbox))
            ;
        for (j = 0; j < 8; j++)
        {
            data[j] = 0;
        }
        canGetData(regset, msgbox, data);
        for (j = 0; j < 8; j++)
        {
            if (data[j] == 53)
            {
                pings++;
            }
        }
        i++;
    }
    return (pings / 8);
} */

uint32 CN20_loopback_test(sciBASE_t *sci, uint8 byte)
{
    // CN20_loopback_test
    //
    // Description:
    // It performs external loopback test (this means you should
    // have a wire connecting the TX pin to RX pin).
    //
    // [input] (sciBASE_t) sci register address
    // [input] (uint8) the transmitter data
    // [output] (uint32) the received data
    //
    uint32 received_byte;
    while ((sci->FLR & (uint32)SCI_TX_INT) == 0U)
    {
    } /* Wait */
    while ((sci->FLR & (uint32)SCI_RX_INT) == 0U)
    {
        sci->TD = byte;
    } /* Wait */
    received_byte = (sci->RD & (uint32)0x000000FFU);
    return received_byte;
}
/** ***************************************************************************
 * @fn      void reset_ethernet_phy()
 * @brief   Reset Ethernet PHY.
 */
void reset_ethernet_phy()
{
    //RESET ETHERNET PHY, need delay time
    gioSetPort(gioPORTA, 0x00); // set PIN 5 Low
    vTaskDelay(1);
    gioSetPort(gioPORTA, 0x20); // set PIN 5 high
    vTaskDelay(1);
}
