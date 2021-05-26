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

#define 	ADD_HYS_Temp					23

#define 	ADD_ENV_TempH					24
#define 	ADD_ENV_TempL					25
#define 	ADD_ENV_active					26

#define 	ADD_CAM_TempH					27
#define 	ADD_CAM_TempL					28
#define 	ADD_CAM_active					29

#define 	ADD_CASE_TempH					30
#define 	ADD_CASE_TempL					31
#define 	ADD_CASE_active					32

#define 	ADD_MB_TempH					33
#define 	ADD_MB_TempL					34
#define 	ADD_MB_active					35

#define 	ADD_TECIN_TempH					36
#define 	ADD_TECIN_TempL					37
#define 	ADD_TECIN_active				38

#define 	ADD_TECOUT_TempH				39
#define 	ADD_TECOUT_TempL				40
#define 	ADD_TECOUT_active				41

#define 	ADD_PASSWORD_ADMIN_0			42
#define 	ADD_PASSWORD_ADMIN_1			43
#define 	ADD_PASSWORD_ADMIN_2			44
#define 	ADD_PASSWORD_ADMIN_3			45

#define 	ADD_DOOR						46
#define 	ADD_UTC_OFF						47

#define 	ADD_PASSWORD_USER_0				48
#define 	ADD_PASSWORD_USER_1				49
#define 	ADD_PASSWORD_USER_2				50
#define 	ADD_PASSWORD_USER_3				51

#define 	ADD_PROFILE_USER				52
#define 	ADD_PROFILE_ADMIN				53

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

#define 	DEFAULT_HYS_Temp					10

#define 	DEFAULT_ENV_TempH					555
#define 	DEFAULT_ENV_TempL					554
#define 	DEFAULT_ENV_active					1

#define 	DEFAULT_CAM_TempH					545
#define 	DEFAULT_CAM_TempL					544
#define 	DEFAULT_CAM_active					1

#define 	DEFAULT_CASE_TempH					400
#define 	DEFAULT_CASE_TempL					100
#define 	DEFAULT_CASE_active					0

#define 	DEFAULT_MB_TempH					402
#define 	DEFAULT_MB_TempL					101
#define 	DEFAULT_MB_active					0

#define 	DEFAULT_TECIN_TempH					403
#define 	DEFAULT_TECIN_TempL					102
#define 	DEFAULT_TECIN_active				0

#define 	DEFAULT_TECOUT_TempH				404
#define 	DEFAULT_TECOUT_TempL				103
#define 	DEFAULT_TECOUT_active				0


#define 	DEFAULT_PASSWORD_ADMIN_0			'0'
#define 	DEFAULT_PASSWORD_ADMIN_1			'0'
#define 	DEFAULT_PASSWORD_ADMIN_2			'0'
#define 	DEFAULT_PASSWORD_ADMIN_3			'1'

#define 	DEFAULT_PASSWORD_USER_0				'0'
#define 	DEFAULT_PASSWORD_USER_1				'0'
#define 	DEFAULT_PASSWORD_USER_2				'0'
#define 	DEFAULT_PASSWORD_USER_3				'0'

#define 	DEFAULT_DOOR						50000

///password page| upgrade page| Wifi page| user option page| Temperature page| Time page| LED page |Dashboard page(1)
#define 	DEFAULT_PROFILE_USER				0xA5//0b10100101
#define 	DEFAULT_PROFILE_ADMIN				0xFF//0b11111111
///////////////////////////////////////////////////////////////////////////////////

uint16_t VirtAddVarTab[NB_OF_VAR];
void Write_defaults(void);
void update_values(void) ;
#endif /* INC_EEPROM_USAGE_H_ */
