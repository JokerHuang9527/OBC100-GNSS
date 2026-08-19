/*
 * obc_io.h
 *
 *  Created on: Aug 23, 2022
 *      Author: bensh
 */

/* header files */
#include "global.h"
#include "HL_can.h"
#include "HL_gio.h"
#include "HL_i2c.h"
#include "HL_reg_i2c.h"
#include "HL_sci.h"
#include "HL_spi.h"
#include "HL_het.h"
#include "HL_emif.h"
#include "HL_reg_het.h"
#include "HL_reg_gio.h"
//#include "redposix.h"
#include "HL_mibspi.h"
#include "HL_reg_mibspi.h"

//#include "HL_sys_dma.c"

void Init_IO(void);
//uint8_t GPIOSetBit(uint8_t bit, uint8_t value);
//int GPIOGetBit(uint8_t bit);
//void GPIOTxTest(void);
void UARTTxTest(sciBASE_t *regset);
uint8_t UARTRxTest(sciBASE_t *regset);
uint8_t UARTloopback(sciBASE_t *regset);

void SPIMasterTxTest(spiBASE_t *regset);
uint8_t SPIMasterRxTest(spiBASE_t *regset);
void SPISlaveTxTest(spiBASE_t *regset);
uint8_t SPISlaveRxTest(spiBASE_t *regset, uint8 spinumber);

void I2CMasterTxTest(i2cBASE_t *regset, uint8_t addr);
uint8_t I2CMasterRxTest(i2cBASE_t *regset, uint8_t addr);
uint8_t I2CSlaveRxTest(i2cBASE_t *regset, uint8_t addr);
void I2CSlaveTxTest(i2cBASE_t *regset, uint8_t addr);

//void CANTxTest(canBASE_t *regset, uint8_t msgbox);
//uint8_t CANRxTest(canBASE_t *regset, uint8_t msgbox);
//void I2CTest(i2cBASE_t *regset, uint8_t addr);

uint32 CN20_loopback_test(sciBASE_t *sci, uint8 byte);
void reset_ethernet_phy();
