/*
 * eeprom_usage.c
 *
 *  Created on: Feb 28, 2021
 *      Author: Administrator
 */
#include "eeprom_usage.h"
#include "astro.h"
uint16_t VirtAddVarTab[NB_OF_VAR];
extern LED_t S1_LED_Value, S2_LED_Value;
extern RELAY_t RELAY1_Value,RELAY2_Value;
extern uint8_t TEC_STATE_Value;
extern POS_t LAT_Value;
extern POS_t LONG_Value;
extern char PASSWORD_ADMIN_Value[5];
extern char PASSWORD_USER_Value[5];
extern uint16_t DOOR_Value;
extern int8_t UTC_OFF_Value;
/////////////////////////////////////read value of parameter from eeprom///////////////////////////////
void update_values(void) {
	uint16_t tmp;
	EE_ReadVariable(VirtAddVarTab[ADD_LAT_deg], &tmp);LAT_Value.deg=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LAT_min],&tmp); LAT_Value.min=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LAT_second],&tmp); LAT_Value.second=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LAT_direction],&tmp);LAT_Value.direction=(char)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_LONG_deg], &tmp);LONG_Value.deg=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LONG_min],&tmp);LONG_Value.min=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LONG_second],&tmp);LONG_Value.second=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_LONG_direction],&tmp);LONG_Value.direction=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_UTC_OFF],&tmp);UTC_OFF_Value=(int8_t)tmp;
	HAL_Delay(100);


	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_TYPE], &tmp);S1_LED_Value.TYPE_Value=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_DAY_BRIGHTNESS],&tmp);S1_LED_Value.DAY_BRIGHTNESS_Value=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_DAY_BLINK],&tmp);S1_LED_Value.DAY_BLINK_Value=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BRIGHTNESS],&tmp);S1_LED_Value.NIGHT_BRIGHTNESS_Value=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BLINK], &tmp);S1_LED_Value.NIGHT_BLINK_Value=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNRISE],&tmp);S1_LED_Value.ADD_SUNRISE_Value=(int8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNSET],&tmp);S1_LED_Value.ADD_SUNSET_Value=(int8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_TYPE], &tmp);S2_LED_Value.TYPE_Value=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_DAY_BRIGHTNESS],&tmp);S2_LED_Value.DAY_BRIGHTNESS_Value=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_DAY_BLINK],&tmp);S2_LED_Value.DAY_BLINK_Value =(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BRIGHTNESS],&tmp);S2_LED_Value.NIGHT_BRIGHTNESS_Value=(uint16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BLINK], &tmp);S2_LED_Value.NIGHT_BLINK_Value=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNRISE],&tmp);S2_LED_Value.ADD_SUNRISE_Value=(int8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNSET],&tmp);S2_LED_Value.ADD_SUNSET_Value=(int8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_TEC_STATE], &tmp);TEC_STATE_Value=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_Temperature0],&tmp);RELAY1_Value.Temperature[0]=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_Edge0],&tmp);RELAY1_Value.Edge[0]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_active0],&tmp);RELAY1_Value.active[0]=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_Temperature1], &tmp);RELAY1_Value.Temperature[1]=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_Edge1],&tmp);RELAY1_Value.Edge[1]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY1_active1],&tmp);RELAY1_Value.active[1]=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_Temperature0],&tmp);RELAY2_Value.Temperature[0]=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_Edge0],&tmp);RELAY2_Value.Edge[0]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_active0],&tmp);RELAY2_Value.active[0]=(uint8_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_Temperature1], &tmp);RELAY2_Value.Temperature[1]=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_Edge1],&tmp);RELAY2_Value.Edge[1]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_RELAY2_active1],&tmp);RELAY2_Value.active[1]=(uint8_t)tmp;
	HAL_Delay(100);

	PASSWORD_ADMIN_Value[4] = 0;
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_0],&tmp);PASSWORD_ADMIN_Value[0]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_1],&tmp);PASSWORD_ADMIN_Value[1]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_2],&tmp);PASSWORD_ADMIN_Value[2]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_3], &tmp);PASSWORD_ADMIN_Value[3]=(char)tmp;
	HAL_Delay(100);

	PASSWORD_USER_Value[4] = 0;
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_USER_0],&tmp);PASSWORD_USER_Value[0]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_USER_1],&tmp);PASSWORD_USER_Value[1]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_USER_2],&tmp);PASSWORD_USER_Value[2]=(char)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_PASSWORD_USER_3], &tmp);PASSWORD_USER_Value[3]=(char)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_DOOR],&tmp);DOOR_Value=(uint16_t)tmp;
	HAL_Delay(100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Write_defaults(void)
{
	EE_WriteVariable(VirtAddVarTab[ADD_LAT_deg], DEFAULT_LAT_deg);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LAT_min],DEFAULT_LAT_min);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LAT_second],DEFAULT_LAT_second);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LAT_direction],DEFAULT_LAT_direction);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_LONG_deg], DEFAULT_LONG_deg);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LONG_min],DEFAULT_LONG_min);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LONG_second],DEFAULT_LONG_second);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_LONG_direction],DEFAULT_LONG_direction);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_UTC_OFF],DEFAULT_UTC_OFF);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_TYPE], DEFAULT_S1_LED_TYPE);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_DAY_BRIGHTNESS],DEFAULT_S1_LED_DAY_BRIGHTNESS);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_DAY_BLINK],DEFAULT_S1_LED_DAY_BLINK);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BRIGHTNESS],DEFAULT_S1_LED_NIGHT_BRIGHTNESS);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BLINK], DEFAULT_S1_LED_NIGHT_BLINK);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNRISE],DEFAULT_S1_LED_ADD_SUNRISE);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNSET],DEFAULT_S1_LED_ADD_SUNSET);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_TYPE], DEFAULT_S2_LED_TYPE);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_DAY_BRIGHTNESS],DEFAULT_S2_LED_DAY_BRIGHTNESS);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_DAY_BLINK],DEFAULT_S2_LED_DAY_BLINK);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BRIGHTNESS],DEFAULT_S2_LED_NIGHT_BRIGHTNESS);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BLINK], DEFAULT_S2_LED_NIGHT_BLINK);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNRISE],DEFAULT_S2_LED_ADD_SUNRISE);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNSET],DEFAULT_S2_LED_ADD_SUNSET);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_TEC_STATE], DEFAULT_TEC_STATE);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Temperature0],DEFAULT_RELAY1_Temperature0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Edge0],DEFAULT_RELAY1_Edge0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_active0],DEFAULT_RELAY1_active0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Temperature1], DEFAULT_RELAY1_Temperature1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Edge1],DEFAULT_RELAY1_Edge1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_active1],DEFAULT_RELAY1_active1);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Temperature0],DEFAULT_RELAY2_Temperature0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Edge0],DEFAULT_RELAY2_Edge0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_active0],DEFAULT_RELAY2_active0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Temperature1], DEFAULT_RELAY2_Temperature1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Edge1],DEFAULT_RELAY2_Edge1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_active1],DEFAULT_RELAY2_active1);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_0],DEFAULT_PASSWORD_ADMIN_0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_1],DEFAULT_PASSWORD_ADMIN_1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_2],DEFAULT_PASSWORD_ADMIN_2);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_3], DEFAULT_PASSWORD_ADMIN_3);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_0],DEFAULT_PASSWORD_USER_0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_1],DEFAULT_PASSWORD_USER_1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_2],DEFAULT_PASSWORD_USER_2);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_3], DEFAULT_PASSWORD_USER_3);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_DOOR],DEFAULT_DOOR);HAL_Delay(100);

}
