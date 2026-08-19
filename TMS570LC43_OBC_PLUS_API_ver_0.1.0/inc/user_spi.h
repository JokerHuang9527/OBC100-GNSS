/*
 * user_spi.h
 *
 *  Created on: 2019¦~6¤ë21¤é
 *      Author: kusoyao
 */

#ifndef INC_USER_SPI_H_
#define INC_USER_SPI_H_

#include "HL_mibspi.h"
#include "HL_spi.h"

int get_index_spi(spiBASE_t *spi);
int get_index_mibspi(mibspiBASE_t *spi);
mibspiRAM_t *get_mibspi_rxram(mibspiBASE_t *spi);

#endif /* INC_USER_SPI_H_ */
