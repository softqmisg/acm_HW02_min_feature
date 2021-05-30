/*
 * veml6030.c
 *
 *  Created on: Aug 2, 2020
 *      Author: mehdi
 */

#include "i2c.h"
#include "veml6030.h"

/**
 * INIT chip
 */

HAL_StatusTypeDef veml6030_init() {
	uint8_t buffer[3];
	uint16_t tmp;

//	HAL_I2C_MspInit(&hi2c3);
//	MX_I2C3_Init();
	////////////////////////////
	tmp = (uint16_t) ALS_PWR_ON << CONF0_ALS_SD
			| (uint16_t) ALS_INT_DIS << CONF0_ALS_INT_EN
			| (uint16_t) ALS_PERS1 << CONF0_ALS_PERS
			| (uint16_t) ALS_IT_100ms << CONF0_ALS_IT
			| (uint16_t) ALS_GAIN_1 << CONF0_ALS_GAIN;

	buffer[0] = VEML6030_ALS_CONF0;
	buffer[1] = (uint8_t) (tmp & 0xff);
	buffer[2] = (uint8_t) (tmp >> 8);
	if (HAL_I2C_Master_Transmit(&hi2c3, VEML6030_BASEADDRESS << 1, buffer, 3,
			1000) != VEML6030_OK)
		return VEML6030_ERROR;
	return VEML6030_OK;
}
/**
 * Set High Threshold
 */
HAL_StatusTypeDef veml6030_sethigh(uint16_t value)
{
	uint8_t buffer[3];
	buffer[0]=VEML6030_ALS_WH;
	buffer[1]=(uint8_t) (value & 0xff);
	buffer[2]= (uint8_t) (value>>8);
	if (HAL_I2C_Master_Transmit(&hi2c3, VEML6030_BASEADDRESS << 1, buffer, 3,
			1000) != VEML6030_OK)
		return VEML6030_ERROR;
	return VEML6030_OK;
}
/**
 * Set low Threshold
 */
HAL_StatusTypeDef veml6030_setlow(uint16_t value)
{
	uint8_t buffer[3];
	buffer[0]=VEML6030_ALS_WL;
	buffer[1]=(uint8_t) (value & 0xff);
	buffer[2]= (uint8_t) (value>>8);
	if (HAL_I2C_Master_Transmit(&hi2c3, VEML6030_BASEADDRESS << 1, buffer, 3,
			1000) != VEML6030_OK)
		return VEML6030_ERROR;
	return VEML6030_OK;
}
/**
 * read als
 */
HAL_StatusTypeDef veml6030_als(uint16_t *value)
{
	uint8_t read_value[2];

	if(HAL_I2C_Mem_Read(&hi2c3, VEML6030_BASEADDRESS<<1,VEML6030_ALS_DATA , I2C_MEMADD_SIZE_8BIT, read_value, 2, 1000)!=VEML6030_OK)
		return VEML6030_ERROR;
	*value = ((uint16_t) read_value[1] << 8) | ((uint16_t) read_value[0]);
	return VEML6030_OK;
}
/**
 * read while
 */
HAL_StatusTypeDef veml6030_white(uint16_t *value)
{
	uint8_t read_value[2];

	if(HAL_I2C_Mem_Read(&hi2c3, VEML6030_BASEADDRESS<<1,VEML6030_WHITE_DATA, I2C_MEMADD_SIZE_8BIT, read_value, 2, 1000)!=VEML6030_OK)
		return VEML6030_ERROR;
	*value = ((uint16_t) read_value[1] << 8) | ((uint16_t) read_value[0]);
	return VEML6030_OK;
}
