/*
 * tmp275.c
 *
 *  Created on: Jun 11, 2020
 *      Author: mehdi
 */

#include "i2c.h"
#include "tmp275.h"
uint8_t tmp275_res=TMP275_12BIT;
HAL_StatusTypeDef tmp275_init(uint8_t ch)
{
	HAL_I2C_MspInit(&hi2c3);
	MX_I2C3_Init();
	return tmp275_setResolution(ch,TMP275_12BIT);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef tmp275_setResolution(uint8_t ch,uint8_t res)
{

	uint8_t buffer[]={TMP275_CONF_REG,res};
	if(HAL_I2C_Master_Transmit(&hi2c3, (TMP275_BASEADDRESS+ch)<<1, buffer, 2, 1000)!=TMP275_OK)
		return TMP275_ERROR;
	tmp275_res=res;
	return TMP275_OK;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef tmp275_readTemperature(uint8_t ch,int16_t *Temperature)
{
	uint8_t buffer[2];
	uint16_t raw_temp;

//	buffer[0]=TMP275_TMP_REG;
//	if(HAL_I2C_Master_Transmit(&hi2c3,(TMP275_BASEADDRESS+ch)<<1,buffer,1,1000)!=TMP275_OK)
//		return TMP275_ERROR;
//
////	switch(tmp275_res)
////	{
////	case TMP275_9BIT:
////		HAL_Delay(TMP275_9BIT_DELAY);
////		break;
////	case TMP275_10BIT:
////		HAL_Delay(TMP275_10BIT_DELAY);
////		break;
////	case TMP275_11BIT:
////		HAL_Delay(TMP275_11BIT_DELAY);
////		break;
////	case TMP275_12BIT:
////		HAL_Delay(TMP275_12BIT_DELAY);
////		break;
////	}
//
//	if(HAL_I2C_Master_Receive(&hi2c3,(TMP275_BASEADDRESS+ch)<<1,buffer,2,2000)!=TMP275_OK)
//		return TMP275_ERROR;
	if( HAL_I2C_Mem_Read(&hi2c3, (TMP275_BASEADDRESS+ch)<<1, TMP275_TMP_REG, I2C_MEMADD_SIZE_8BIT, buffer, 2, 1000)!=TMP275_OK)
	{
		return TMP275_ERROR;
	}
	raw_temp=(uint16_t)(buffer[0]<<4)|(buffer[1]>>4);
	if(buffer[0]>0x80)
	{
		raw_temp=(~raw_temp)&0xFFF+1;
		raw_temp=(int16_t)raw_temp*10;
		raw_temp=raw_temp>>4;
		*Temperature=-raw_temp;
	}
	else
	{
		raw_temp=(int16_t)raw_temp*10;
		raw_temp=raw_temp>>4;
		*Temperature=raw_temp;

	}
	return TMP275_OK;
}
