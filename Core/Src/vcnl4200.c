//#include "main.h"
#include "i2c.h"
#include "vcnl4200.h"
///////////////////////////////////////////////////
HAL_StatusTypeDef vcnl4200_init(void) {
//	HAL_I2C_MspInit(&hi2c3);
//	MX_I2C3_Init();
	uint8_t buffer[4];
	/////////////////////////////////ALS_CONF////////////////////////////////////////
	buffer[0] = VCNL4200_ALS_CONF;
	buffer[1] = VCNL4200_ALS_Shutdown_on | VCNL4200_ALS_Interrupt_disable
			| VCNL4200_ALS_Pers_one | VCNL4200_ALS_INT_SWITCH_als
			| VCNL4200_ALS_IT_ms100; //0x40;//0xFA;//low byte:11 11 101 0 :1/1280 9T~240ms
	buffer[2] = 0x00; //high byte:0000 0000
	if (HAL_I2C_Master_Transmit(&hi2c3, VCNL4200_BASEADDRESS << 1, buffer, 3,
			1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	HAL_Delay(10);
	///////////////////////////////PS_CONF1_CONF2///////////////////////////////////////
	//low byte:PD_duty(7:6) PS_PERS(5:4)	PS_IT(3:1)	PS_SD(0)
	//high byte:reserved(7:4) PS_HD(3)	reserved(2) PS_INT(1:0)
	buffer[0] = VCNL4200_PS_CONF1_CONF2;
	buffer[1] = VCNL4200_PS_Shutdown_on | VCNL4200_PS_9T | VCNL4200_PS_Pers_1
			| VCNL4200_PS_1_160;//0x2a;//0xFA;//low byte:11 11 101 0 :1/1280 9T~240ms
	buffer[2] = 0x0b;	//high byte:0000 1 0	11
	if (HAL_I2C_Master_Transmit(&hi2c3, VCNL4200_BASEADDRESS << 1, buffer, 3,
			1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	HAL_Delay(10);
	///////////////////////////////PS_CONF3_MS///////////////////////////////////////////
	//low byte:Reserved(7) PS_MPS(6:5)	PS_SMART_PERS(4)	PS_AF(3) PS_TRIG(2) PS_SC_ADV(1) PS_SC_EN(0)
	//high byte:reserved(7:6) PS_MS(5)	PS_SP(4) PS_SPO(3) LED_I(2:0)
	buffer[0] = VCNL4200_PS_CONF3_MS;
	buffer[1] = 0x60;	//0x70;//low byte:0 11 1 0 0 0 0
	buffer[2] = 0x27;	//0x22;//high byte:00 1 0 0 010 //100mA
	if (HAL_I2C_Master_Transmit(&hi2c3, VCNL4200_BASEADDRESS << 1, buffer, 3,
			1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	HAL_Delay(10);

	////////////////////////////////////////PS_THDL////////////////////////////////////
	buffer[0] = VCNL4200_PS_THDL;
	buffer[1] = 0x00;	//low byte:0 11 0 0 0 0 0
	buffer[2] = 0x3c;	//high byte:00 1 0 0 010 //100mA
	if (HAL_I2C_Master_Transmit(&hi2c3, VCNL4200_BASEADDRESS << 1, buffer, 3,
			1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	HAL_Delay(10);
	//////////////////////////////////////PS_THDH/////////////////////////////////
	buffer[0] = VCNL4200_PS_THDH;
	buffer[1] = 0x00;	//low byte:0 11 0 0 0 0 0
	buffer[2] = 0x40;	//high byte:00 1 0 0 010 //100mA
	if (HAL_I2C_Master_Transmit(&hi2c3, VCNL4200_BASEADDRESS << 1, buffer, 3,
			1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	HAL_Delay(10);

	return VCNL4200_OK;
}
///////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef vcnl4200_read(uint8_t cmd, uint16_t *value) {
	uint8_t buffer, read_value[2];

	buffer = cmd;

	if (HAL_I2C_Mem_Read(&hi2c3, VCNL4200_BASEADDRESS << 1, cmd,
			I2C_MEMADD_SIZE_8BIT, read_value, 2, 1000) != VCNL4200_OK)
		return VCNL4200_ERROR;
	*value = ((uint16_t) read_value[1] << 8) | ((uint16_t) read_value[0]);
	return VCNL4200_OK;
}
////////////////////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef vcnl4200_ps(uint16_t *value) {
return vcnl4200_read(VCNL4200_PS_Data, value);
}
////////////////////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef vcnl4200_als(uint16_t *value) {
return vcnl4200_read(VCNL4200_ALS_Data, value);
}

