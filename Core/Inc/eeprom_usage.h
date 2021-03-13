/*
 * eeprom_usage.h
 *
 *  Created on: Feb 28, 2021
 *      Author: Administrator
 */

#ifndef INC_EEPROM_USAGE_H_
#define INC_EEPROM_USAGE_H_
#include "eeprom.h"
#include "main.h"
//////////////////////////////////Address of variable in EEPROM///////////////////////////////////////
#define		ADD_LAT_deg						0
#define		ADD_LAT_min						1
#define		ADD_LAT_second					2
#define 	ADD_LAT_direction				3

#define 	ADD_LONG_deg					4
#define 	ADD_LONG_min					5
#define 	ADD_LONG_second					6
#define 	ADD_LONG_direction				7

#define 	ADD_S1_LED_TYPE					8
#define 	ADD_S1_LED_DAY_BRIGHTNESS		9
#define 	ADD_S1_LED_DAY_BLINK			10
#define 	ADD_S1_LED_NIGHT_BRIGHTNESS		11
#define 	ADD_S1_LED_NIGHT_BLINK			12
#define 	ADD_S1_LED_ADD_SUNRISE			13
#define 	ADD_S1_LED_ADD_SUNSET			14

#define 	ADD_S2_LED_TYPE					15
#define 	ADD_S2_LED_DAY_BRIGHTNESS		16
#define 	ADD_S2_LED_DAY_BLINK			17
#define 	ADD_S2_LED_NIGHT_BRIGHTNESS		18
#define 	ADD_S2_LED_NIGHT_BLINK			19
#define 	ADD_S2_LED_ADD_SUNRISE			20
#define 	ADD_S2_LED_ADD_SUNSET			21

#define 	ADD_TEC_STATE					22
#define 	ADD_RELAY1_Temperature0			23
#define 	ADD_RELAY1_Edge0				24
#define 	ADD_RELAY1_active0				25
#define 	ADD_RELAY1_Temperature1			26
#define 	ADD_RELAY1_Edge1				27
#define 	ADD_RELAY1_active1				28

#define 	ADD_RELAY2_Temperature0			29
#define 	ADD_RELAY2_Edge0				30
#define 	ADD_RELAY2_active0				31
#define 	ADD_RELAY2_Temperature1			32
#define 	ADD_RELAY2_Edge1				33
#define 	ADD_RELAY2_active1				34


#define 	ADD_PASSWORD_0					35
#define 	ADD_PASSWORD_1					36
#define 	ADD_PASSWORD_2					37
#define 	ADD_PASSWORD_3					38

#define 	ADD_DOOR						39
#define 	ADD_UTC_OFF						40
//////////////////////////////////default values  of variable in EEPROM///////////////////////////////////////
#define		DEFAULT_LAT_deg						35
#define		DEFAULT_LAT_min						42
#define		DEFAULT_LAT_second					0
#define 	DEFAULT_LAT_direction				'N'

#define 	DEFAULT_LONG_deg					51
#define 	DEFAULT_LONG_min					23
#define 	DEFAULT_LONG_second					5316
#define 	DEFAULT_LONG_direction				'E'
#define 	DEFAULT_UTC_OFF						35

#define 	DEFAULT_S1_LED_TYPE					WHITE_LED
#define 	DEFAULT_S1_LED_DAY_BRIGHTNESS		0
#define 	DEFAULT_S1_LED_DAY_BLINK			5
#define 	DEFAULT_S1_LED_NIGHT_BRIGHTNESS		80
#define 	DEFAULT_S1_LED_NIGHT_BLINK			0
#define 	DEFAULT_S1_LED_ADD_SUNRISE			15
#define 	DEFAULT_S1_LED_ADD_SUNSET			-10

#define 	DEFAULT_S2_LED_TYPE					WHITE_LED
#define 	DEFAULT_S2_LED_DAY_BRIGHTNESS		0
#define 	DEFAULT_S2_LED_DAY_BLINK			5
#define 	DEFAULT_S2_LED_NIGHT_BRIGHTNESS		80
#define 	DEFAULT_S2_LED_NIGHT_BLINK			0
#define 	DEFAULT_S2_LED_ADD_SUNRISE			15
#define 	DEFAULT_S2_LED_ADD_SUNSET			-10

#define 	DEFAULT_TEC_STATE					1
#define 	DEFAULT_RELAY1_Temperature0			331
#define 	DEFAULT_RELAY1_Edge0				'U'
#define 	DEFAULT_RELAY1_active0				1
#define 	DEFAULT_RELAY1_Temperature1			0
#define 	DEFAULT_RELAY1_Edge1				'-'
#define 	DEFAULT_RELAY1_active1				0

#define 	DEFAULT_RELAY2_Temperature0			325
#define 	DEFAULT_RELAY2_Edge0				'D'
#define 	DEFAULT_RELAY2_active0				1
#define 	DEFAULT_RELAY2_Temperature1			358
#define 	DEFAULT_RELAY2_Edge1				'U'
#define 	DEFAULT_RELAY2_active1				1


#define 	DEFAULT_PASSWORD_0					'0'
#define 	DEFAULT_PASSWORD_1					'0'
#define 	DEFAULT_PASSWORD_2					'0'
#define 	DEFAULT_PASSWORD_3					'0'

#define 	DEFAULT_DOOR						1200
///////////////////////////////////////////////////////////////////////////////////

uint16_t VirtAddVarTab[NB_OF_VAR];
void Write_defaults(void);

#endif /* INC_EEPROM_USAGE_H_ */
