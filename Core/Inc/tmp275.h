/*
 * tmp275.h
 *
 *  Created on: Jun 11, 2020
 *      Author: mehdi
 */

#ifndef INC_TMP275_H_
#define INC_TMP275_H_

#include "stm32f4xx_hal.h"
#define TMP275_BASEADDRESS	0x48
#define TMP_CH0	0
#define TMP_CH1	1
#define TMP_CH2	2
#define TMP_CH3	3
#define TMP_CH4	4
#define TMP_CH5	5
#define TMP_CH6	6
#define TMP_CH7	7

/*
 * registers
 */
#define	TMP275_TMP_REG		0x00
#define	TMP275_CONF_REG		0x01
#define TMP275_TLOW_REG		0x02
#define TMP275_THIGH_REG	0x03

#define TMP275_OK			HAL_OK
#define TMP275_ERROR		HAL_ERROR

#define TMP275_9BIT				0x00<<5
#define TMP275_10BIT			0x01<<5
#define TMP275_11BIT			0x02<<5
#define TMP275_12BIT			0x03<<5

#define TMP275_9BIT_DELAY	38//37.5ms, 75ms, 150ms, 300ms
#define TMP275_10BIT_DELAY	75//37.5ms, 75ms, 150ms, 300ms
#define TMP275_11BIT_DELAY	150//37.5ms, 75ms, 150ms, 300ms
#define TMP275_12BIT_DELAY	300//37.5ms, 75ms, 150ms, 300ms


HAL_StatusTypeDef tmp275_init(uint8_t ch);
HAL_StatusTypeDef tmp275_readTemperature(uint8_t ch,double *temperature);
HAL_StatusTypeDef tmp275_setResolution(uint8_t ch,uint8_t res);	//1 = 9bit, 2 = 10bit, 3 = 11bit, 4 = 12bit

#endif /* INC_TMP275_H_ */
