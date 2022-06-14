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

#define 	ADD_DUMMY						0
#define		ADD_LAT_second					1
#define		ADD_LAT_deg						2
#define		ADD_LAT_min						3
#define 	ADD_LAT_direction				4

#define 	ADD_LONG_deg					5
#define 	ADD_LONG_min					6
#define 	ADD_LONG_second					7
#define 	ADD_LONG_direction				8

#define 	ADD_S1_LED_TYPE					9
#define 	ADD_S1_LED_DAY_BRIGHTNESS		10
#define 	ADD_S1_LED_DAY_BLINK			11
#define 	ADD_S1_LED_NIGHT_BRIGHTNESS		12
#define 	ADD_S1_LED_NIGHT_BLINK			13
#define 	ADD_S1_LED_ADD_SUNRISE			14
#define 	ADD_S1_LED_ADD_SUNSET			15

#define 	ADD_S2_LED_TYPE					16
#define 	ADD_S2_LED_DAY_BRIGHTNESS		17
#define 	ADD_S2_LED_DAY_BLINK			18
#define 	ADD_S2_LED_NIGHT_BRIGHTNESS		19
#define 	ADD_S2_LED_NIGHT_BLINK			20
#define 	ADD_S2_LED_ADD_SUNRISE			21
#define 	ADD_S2_LED_ADD_SUNSET			22

#define 	ADD_TEC_STATE					23

#define 	ADD_HYS_Temp					24

#define 	ADD_ENV_TempH					25
#define 	ADD_ENV_TempL					26
#define 	ADD_ENV_active					27

#define 	ADD_CAM_TempH					28
#define 	ADD_CAM_TempL					29
#define 	ADD_CAM_active					30

#define 	ADD_CASE_TempH					31
#define 	ADD_CASE_TempL					32
#define 	ADD_CASE_active					33

#define 	ADD_MB_TempH					34
#define 	ADD_MB_TempL					35
#define 	ADD_MB_active					36

#define 	ADD_TECIN_TempH					37
#define 	ADD_TECIN_TempL					38
#define 	ADD_TECIN_active				39

#define 	ADD_TECOUT_TempH				40
#define 	ADD_TECOUT_TempL				41
#define 	ADD_TECOUT_active				42

#define 	ADD_PASSWORD_ADMIN_0			43
#define 	ADD_PASSWORD_ADMIN_1			44
#define 	ADD_PASSWORD_ADMIN_2			45
#define 	ADD_PASSWORD_ADMIN_3			46

#define 	ADD_DOOR						47
#define 	ADD_UTC_OFF						48

#define 	ADD_PASSWORD_USER_0				49
#define 	ADD_PASSWORD_USER_1				50
#define 	ADD_PASSWORD_USER_2				51
#define 	ADD_PASSWORD_USER_3				52

#define 	ADD_PROFILE_USER				53
#define 	ADD_PROFILE_ADMIN				54



//////////////////////////////////default values  of variable in EEPROM///////////////////////////////////////
#define		DEFAULT_LAT_deg						35
#define		DEFAULT_LAT_min						50
#define		DEFAULT_LAT_second					2400
#define 	DEFAULT_LAT_direction				'N'

#define 	DEFAULT_LONG_deg					50
#define 	DEFAULT_LONG_min					56
#define 	DEFAULT_LONG_second					2076
#define 	DEFAULT_LONG_direction				'E'
#define 	DEFAULT_UTC_OFF						35

#define 	DEFAULT_S1_LED_TYPE					WHITE_LED
#define 	DEFAULT_S1_LED_DAY_BRIGHTNESS		0
#define 	DEFAULT_S1_LED_DAY_BLINK			5
#define 	DEFAULT_S1_LED_NIGHT_BRIGHTNESS		5
#define 	DEFAULT_S1_LED_NIGHT_BLINK			5
#define 	DEFAULT_S1_LED_ADD_SUNRISE			0
#define 	DEFAULT_S1_LED_ADD_SUNSET			0

#define 	DEFAULT_S2_LED_TYPE					WHITE_LED
#define 	DEFAULT_S2_LED_DAY_BRIGHTNESS		0
#define 	DEFAULT_S2_LED_DAY_BLINK			5
#define 	DEFAULT_S2_LED_NIGHT_BRIGHTNESS		5
#define 	DEFAULT_S2_LED_NIGHT_BLINK			5
#define 	DEFAULT_S2_LED_ADD_SUNRISE			0
#define 	DEFAULT_S2_LED_ADD_SUNSET			0

#define 	DEFAULT_TEC_STATE					1

#define 	DEFAULT_HYS_Temp					20

#define 	DEFAULT_ENV_TempH					400
#define 	DEFAULT_ENV_TempL					100
#define 	DEFAULT_ENV_active					1

#define 	DEFAULT_CAM_TempH					500
#define 	DEFAULT_CAM_TempL					100
#define 	DEFAULT_CAM_active					1

#define 	DEFAULT_CASE_TempH					400
#define 	DEFAULT_CASE_TempL					100
#define 	DEFAULT_CASE_active					0

#define 	DEFAULT_MB_TempH					400
#define 	DEFAULT_MB_TempL					100
#define 	DEFAULT_MB_active					0

#define 	DEFAULT_TECIN_TempH					400
#define 	DEFAULT_TECIN_TempL					100
#define 	DEFAULT_TECIN_active				0

#define 	DEFAULT_TECOUT_TempH				400
#define 	DEFAULT_TECOUT_TempL				100
#define 	DEFAULT_TECOUT_active				0


#define 	DEFAULT_PASSWORD_ADMIN_0			'0'
#define 	DEFAULT_PASSWORD_ADMIN_1			'0'
#define 	DEFAULT_PASSWORD_ADMIN_2			'0'
#define 	DEFAULT_PASSWORD_ADMIN_3			'0'

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
