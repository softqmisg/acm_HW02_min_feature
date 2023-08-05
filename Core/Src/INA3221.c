/*
 * INA3221.c
 *
 *  Created on: Jul 30, 2020
 *      Author: mehdi
 */

#include "i2c.h"
#include "ina3221.h"
/**
 * address :7bit address
 */
HAL_StatusTypeDef ina3221_readreg(uint8_t address,uint8_t reg,uint8_t *value)
{
	HAL_StatusTypeDef status;
	  status=HAL_I2C_Mem_Read(&hi2c3, address<<1, reg, I2C_MEMADD_SIZE_8BIT, value, 2, 1000);
	  return status;
}
/**
 *
 */
HAL_StatusTypeDef ina3221_writereg(uint8_t address,uint8_t reg,uint8_t *value)
{
	HAL_StatusTypeDef status;
	  status=HAL_I2C_Mem_Write(&hi2c3, address<<1, reg, I2C_MEMADD_SIZE_8BIT, value, 2, 1000);
	  return status;
}
/**
 *
 */
HAL_StatusTypeDef ina3221_init(uint8_t add)
{
	HAL_StatusTypeDef status=HAL_OK;
	uint8_t buffer[2];
//	HAL_I2C_MspInit(&hi2c3);
//	MX_I2C3_Init();
//	buffer[1]=0x80;
//	buffer[0]=0x00;
//	status=ina3221_writereg( add, INA3221_CONF,buffer);
//	HAL_Delay(100);
//	buffer[1]=(0<<7)|(7<<4)|(0<<1)|(1<<0);
//	buffer[0]=(0<<6)|(4<<3)|(7<<0);
//	status=ina3221_writereg( add, INA3221_CONF,buffer);
	return status;
}

/*
 * channel:from enums
 */
HAL_StatusTypeDef ina3221_readdouble(uint8_t channel,double *value)
{
	HAL_StatusTypeDef status;
	uint8_t buffer[2];
	switch(channel)
	{
	case VOLTAGE_TEC:
		status=ina3221_readreg( INA3221_TEC_BASEADDRESS, INA3221_BUSV_CH1,buffer);
		*value=(double)ina3221_buffer2double(buffer,VOLTAGE_COEFF);
		break;
	case CURRENT_TEC:
		status=ina3221_readreg( INA3221_TEC_BASEADDRESS, INA3221_SHUNTV_CH1,buffer);
		*value=(double)ina3221_buffer2double(buffer,CURRENT_COEFF)/RESISTOR_TEC;
		break;
	case VOLTAGE_12V:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_BUSV_CH2,buffer);
		*value=(double)ina3221_buffer2double(buffer,VOLTAGE_COEFF);
		break;
	case CURRENT_12V:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_SHUNTV_CH2,buffer);
		*value=(double)ina3221_buffer2double(buffer,CURRENT_COEFF)/RESISTOR_12V;
		break;
	case VOLTAGE_7V:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_BUSV_CH1,buffer);
		*value=(double)ina3221_buffer2double(buffer,VOLTAGE_COEFF);
		break;
	case CURRENT_7V:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_SHUNTV_CH1,buffer);
		*value=(double)ina3221_buffer2double(buffer,CURRENT_COEFF)/RESISTOR_7V;
		break;
	case VOLTAGE_3V3:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_BUSV_CH3,buffer);
		*value=(double)ina3221_buffer2double(buffer,VOLTAGE_COEFF);
		break;
	case CURRENT_3V3:
		status=ina3221_readreg( INA3221_LED_BASEADDRESS, INA3221_SHUNTV_CH3,buffer);
		*value=(double)ina3221_buffer2double(buffer,CURRENT_COEFF)/RESISTOR_3V3;
		break;
	}
	return status;
}
/**
 * convert 2'scomplement
 */
double ina3221_buffer2double(uint8_t *buffer,double coeff)
{
	double value_d;
	uint16_t value=(uint16_t) (buffer[0]<<8)|(uint16_t)(buffer[1]<<0);
	if(value &0x8000)
	{
		value=value&0x7FFF;
		value=(uint16_t)value-1;
		value=(uint16_t)~value;
		value=(uint16_t)value>>3;
		value_d=(double)value*-1.0*coeff;
	}
	else
	{
		value=(uint16_t)value>>3;
		value_d=(double)value*coeff;
	}
	return value_d;
}
