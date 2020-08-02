/*
 * veml6030.h
 *
 *  Created on: Aug 2, 2020
 *      Author: mehdi
 */

#ifndef INC_VEML6030_H_
#define INC_VEML6030_H_

#define	VEML6030_BASEADDRESS	0x10

/*
 * register address
 */

#define VEML6030_ALS_CONF0		0x00
#define VEML6030_ALS_WH			0x01
#define VEML6030_ALS_WL			0x02
#define VEML6030_POWER_SAVING	0x03
#define VEML6030_ALS_DATA		0x04
#define VEML6030_WHITE_DATA		0x05
#define VEML6030_ALS_INT		0x06
/*
 *bits of VEML6030_ALS_CON0
 */
#define CONF0_ALS_SD	0
enum {
	ALS_PWR_ON = 0, ALS_SHUT_DN = 1
};
#define CONF0_ALS_INT_EN	1
enum {
	ALS_INT_DIS = 0, ALS_INT_EN = 1
};
#define CONF0_ALS_PERS		4
enum {
	ALS_PERS1 = 0, ALS_PERS2 = 1, ALS_PERS4 = 2, ALS_PERS4 = 3
};
#define CONF0_ALS_IT	6
enum {
	ALS_IT_25ms = 12,
	ALS_IT_50ms = 8,
	ALS_IT_100ms = 0,
	ALS_IT_200ms = 1,
	ALS_IT_400ms = 2,
	ALS_IT_800ms = 3
};
#define CONF0_ALS_GAIN	11
enum {
	ALS_GAIN_1 = 0, ALS_GAIN_2 = 1, ALS_GAIN_1_8 = 2, ALS_GAIN_1_4 = 3
};
////////////////////////////////////////////////////////////////////////////
#define VEML6030_OK HAL_OK
#define VEML6030_ERROR	HAL_ERROR
//////////////////////////////////////////////////////////////////////////////
HAL_StatusTypeDef veml6030_init();
HAL_StatusTypeDef veml6030_sethigh(uint16_t value);
HAL_StatusTypeDef veml6030_setlow(uint16_t value);
HAL_StatusTypeDef veml6030_als(uint16_t *value);
HAL_StatusTypeDef veml6030_white(uint16_t *value);
#endif /* INC_VEML6030_H_ */
