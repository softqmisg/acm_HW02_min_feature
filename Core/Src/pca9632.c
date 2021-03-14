#include "i2c.h"
#include "pca9632.h"

//////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_init(void) {
	HAL_I2C_MspInit(&hi2c3);
	MX_I2C3_Init();
	uint8_t buffer[5];
	buffer[0] = PCA9632_CMD_MODE1;
	buffer[1] = 0x00; //00000001//normal & accept all call
	if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
			500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);

	buffer[0] = PCA9632_CMD_MODE2;
	buffer[1] = 0x35; //00110101//blinking/invert/stop/totem pole
	if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
			500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);
	return PCA9632_OK;
}

//////////////////////////////////////////////////////////////////
//	LDRx = 00 — LED driver x is off (default power-up state).
//	LDRx = 01 — LED driver x is fully on (individual brightness and group dimming/blinking
//	not controlled).
//	LDRx = 10 — LED driver x individual brightness can be controlled through its PWMx
//	register.
//	LDRx = 11 — LED driver x individual brightness and group dimming/blinking can be
//	controlled through its PWMx register and the GRPPWM registers.

HAL_StatusTypeDef pca9632_setouttype(uint8_t leds, uint8_t type) {

	uint8_t buffer[2];
	//read current LED type
	if (pca9632_getouttype(&buffer[1]) != PCA9632_OK)
	{
//		printf("read type pca932 error\n\r");
		return PCA9632_ERROR;
	}

	if (leds & 0x01) {
		buffer[1] &= 0xFC; //LED0
		buffer[1] |= (type << 0);
	}
	if (leds & 0x02) {
		buffer[1] &= 0xF3; //LED1
		buffer[1] |= (type << 2);
	}
	if (leds & 0x04) {
		buffer[1] &= 0xCF; //LED2
		buffer[1] |= (type << 4);
	}
	if (leds & 0x08) {
		buffer[1] &= 0x3F; //LED3
		buffer[1] |= (type << 6);
	}
	//LEDS in pwm mode
	buffer[0] = PCA9632_CMD_LEDOUT;

	if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
			500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);
	return PCA9632_OK;
}
///////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_getouttype(uint8_t *type) {
	uint8_t buffer[2];
	//read current LED type
	buffer[0] = PCA9632_CMD_LEDOUT;
	if (HAL_I2C_Mem_Read(&hi2c3, PCA9632_BASEADDRESS << 1, buffer[0],
			I2C_MEMADD_SIZE_8BIT, type, 1, 500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);
	return HAL_OK;
}
///////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_getbrightness(uint8_t leds,uint8_t *bright) {
	uint8_t buffer[2];
	//read current LED type
	if (leds & 0x01)
		buffer[0] = PCA9632_CMD_PWM0;
	if (leds & 0x02)
		buffer[0] = PCA9632_CMD_PWM1;
	if (leds & 0x04)
		buffer[0] = PCA9632_CMD_PWM2;
	if (leds & 0x08)
		buffer[0] = PCA9632_CMD_PWM3;
	if (HAL_I2C_Mem_Read(&hi2c3, PCA9632_BASEADDRESS << 1, buffer[0],
			I2C_MEMADD_SIZE_8BIT, bright, 1, 500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);
	return HAL_OK;
}

///////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_setbrighness(uint8_t leds, uint8_t percent_brightness) {
	uint8_t buffer[2];
	uint16_t tmp = ((uint16_t) percent_brightness * 256 / 100);
	if (tmp > 255)
		tmp = 255;
	buffer[1] = (uint8_t) tmp;
	if (leds & 0x01) {
		buffer[0] = PCA9632_CMD_PWM0;
		if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
				500) != PCA9632_OK)
			return PCA9632_ERROR;
		HAL_Delay(20);
	}
	if (leds & 0x02) {
		buffer[0] = PCA9632_CMD_PWM1;
		if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
				500) != PCA9632_OK)
			return PCA9632_ERROR;
		HAL_Delay(20);
	}
	if (leds & 0x04) {
		buffer[0] = PCA9632_CMD_PWM2;
		if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
				500) != PCA9632_OK)
			return PCA9632_ERROR;
		HAL_Delay(20);
	}
	if (leds & 0x08) {
		buffer[0] = PCA9632_CMD_PWM3;
		if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
				500) != PCA9632_OK)
			return PCA9632_ERROR;
		HAL_Delay(20);
	}
	return PCA9632_OK;
}
///////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_setgroupblink(double blink_second) {
	uint8_t buffer[2];
	buffer[0] = PCA9632_CMD_GRPFREQ;
	buffer[1] = (uint8_t) ((double) 24 * blink_second-1.0 );
	if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
			500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);

	buffer[0] = PCA9632_CMD_GRPPWM;
	buffer[1] = 128;
	if (HAL_I2C_Master_Transmit(&hi2c3, PCA9632_BASEADDRESS << 1, buffer, 2,
			500) != PCA9632_OK)
		return PCA9632_ERROR;
	HAL_Delay(20);
	return PCA9632_OK;

}
///////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef pca9632_setbrighnessblinking(uint8_t leds,
		uint8_t percent_brightness, double second) {
	uint8_t buffer[2];
	double seconds;
	//read current LED type
	if (percent_brightness == 0) {
		if (pca9632_setouttype(leds, LEDTYPE_OFF) != PCA9632_OK)
			return PCA9632_ERROR;
	} else if (second < 0.1 && percent_brightness == 100) {
		if (pca9632_setouttype(leds, LEDTYPE_ON) != PCA9632_OK)
			return PCA9632_ERROR;
	} else {
		if (second < 0.1) {
			if (pca9632_setouttype(leds, LEDTYPE_BRIGHTNESS) != PCA9632_OK)
				return PCA9632_ERROR;
		} else {
			if (pca9632_setouttype(leds, LEDTYPE_BLINK) != PCA9632_OK)
				return PCA9632_ERROR;
			if (pca9632_setgroupblink(second))
				return PCA9632_ERROR;
		}
		if(pca9632_setbrighness(leds, percent_brightness)!=PCA9632_OK)
			return PCA9632_ERROR;
	}
	if(pca9632_getbrightness( leds,&buffer[1])!=PCA9632_OK)
		return PCA9632_ERROR;
	return PCA9632_OK;
}
//////////////////////////////////////////////////////////////////
