/*
 * INA3221.h
 *
 *  Created on: Jul 30, 2020
 *      Author: mehdi
 */

#ifndef INC_INA3221_H_
#define INC_INA3221_H_

#define	INA3221_LED_BASEADDRESS	0x42
#define	INA3221_TEC_BASEADDRESS	0x43
/**
 * registers definition ch1,ch2,ch3
 */
#define INA3221_CONF		0x00
#define INA3221_SHUNTV_CH1	0x01
#define INA3221_BUSV_CH1	0x02
#define INA3221_SHUNTV_CH2	0x03
#define INA3221_BUSV_CH2	0x04
#define INA3221_SHUNTV_CH3	0x05
#define INA3221_BUSV_CH3	0x06
#define INA3221_CA_CH1		0x07
#define INA3221_WA_CH1		0x08
#define INA3221_CA_CH2		0x09
#define INA3221_WA_CH2		0x0A
#define INA3221_CA_CH3		0x0B
#define INA3221_WA_CH3		0x0C
#define INA3221_SHUNTV_SUM	0x0D
#define INA3221_SHUNTV_SUMLIM	0x0E
#define INA3221_MASK_EN		0x0F
#define INA3221_VALID_UP	0x10
#define INA3221_VALID_LOW	0x11
#define INA3221_MANUFACTURE_ID	0xFE
#define INA3221_DIE_ID	0xFF
////////////////////////////////
#define VOLTAGE_COEFF	8e-3
#define CURRENT_COEFF	40e-6
///////////////////////////
#define RESISTOR_TEC	(double)0.02
#define RESISTOR_12V	(double)0.05
#define RESISTOR_7V		(double)0.05
#define RESISTOR_3V3	(double)0.1
///////////////////////////
enum {VOLTAGE_TEC=0,CURRENT_TEC,VOLTAGE_12V,CURRENT_12V,VOLTAGE_7V,CURRENT_7V,VOLTAGE_3V3,CURRENT_3V3};
//////////////////////////
double ina3221_buffer2double(uint8_t *buffer,double coeff);
HAL_StatusTypeDef ina3221_readreg(uint8_t address,uint8_t reg,uint8_t *value);
HAL_StatusTypeDef ina3221_readfloat(uint8_t channel,double *value);
HAL_StatusTypeDef ina3221_init(void);

#endif /* INC_INA3221_H_ */
