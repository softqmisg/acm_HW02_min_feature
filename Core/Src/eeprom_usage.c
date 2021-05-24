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
extern RELAY_t TempLimit_Value[6];
extern uint8_t TEC_STATE_Value;
extern uint8_t HYSTERESIS_Value;
extern POS_t LAT_Value;
extern POS_t LONG_Value;
extern char PASSWORD_ADMIN_Value[5];
extern char PASSWORD_USER_Value[5];
extern uint16_t DOOR_Value;
extern int8_t UTC_OFF_Value;
extern uint8_t profile_user_Value;
extern uint8_t profile_admin_Value;
extern uint8_t profile_active[][MENU_TOTAL_ITEMS];
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

	EE_ReadVariable(VirtAddVarTab[ADD_HYS_Temp], &tmp);HYSTERESIS_Value=(uint8_t)tmp;
	HAL_Delay(100);


	EE_ReadVariable(VirtAddVarTab[ADD_ENV_TempH],&tmp);TempLimit_Value[ENVIROMENT_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_ENV_TempL],&tmp);TempLimit_Value[ENVIROMENT_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_ENV_active],&tmp);TempLimit_Value[ENVIROMENT_TEMP].active=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_CAM_TempH],&tmp);TempLimit_Value[CAM_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_CAM_TempL],&tmp);TempLimit_Value[CAM_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_CAM_active],&tmp);TempLimit_Value[CAM_TEMP].active=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_CASE_TempH],&tmp);TempLimit_Value[CASE_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_CASE_TempL],&tmp);TempLimit_Value[CASE_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_CASE_active],&tmp);TempLimit_Value[CASE_TEMP].active=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_MB_TempH],&tmp);TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_MB_TempL],&tmp);TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_MB_active],&tmp);TempLimit_Value[MOTHERBOARD_TEMP].active=(uint8_t)tmp;
	HAL_Delay(100);

	EE_ReadVariable(VirtAddVarTab[ADD_TECIN_TempH],&tmp);TempLimit_Value[TECIN_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_TECIN_TempL],&tmp);TempLimit_Value[TECIN_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_TECIN_active],&tmp);TempLimit_Value[TECIN_TEMP].active=(uint8_t)tmp;
	HAL_Delay(100);


	EE_ReadVariable(VirtAddVarTab[ADD_TECOUT_TempH],&tmp);TempLimit_Value[TECOUT_TEMP].TemperatureH=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_TECOUT_TempL],&tmp);TempLimit_Value[TECOUT_TEMP].TemperatureL=(int16_t)tmp;
	HAL_Delay(100);
	EE_ReadVariable(VirtAddVarTab[ADD_TECOUT_active],&tmp);TempLimit_Value[TECOUT_TEMP].active=(uint8_t)tmp;
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
	EE_ReadVariable(VirtAddVarTab[ADD_PROFILE_USER],&tmp);profile_user_Value=(uint8_t)tmp;
	EE_ReadVariable(VirtAddVarTab[ADD_PROFILE_ADMIN],&tmp);profile_admin_Value=(uint8_t)tmp;
	if(profile_user_Value& 0x02)
	{
		profile_active[USER_PROFILE][2]=1;
		profile_active[USER_PROFILE][3]=1;
	}
	else
	{
		profile_active[USER_PROFILE][2]=0;
		profile_active[USER_PROFILE][3]=0;
	}

	if(profile_user_Value& 0x04)
	{
		profile_active[USER_PROFILE][0]=1;
		profile_active[USER_PROFILE][1]=1;
	}
	else
	{
		profile_active[USER_PROFILE][0]=0;
		profile_active[USER_PROFILE][1]=0;
	}

	if(profile_user_Value& 0x08)
	{
		profile_active[USER_PROFILE][5]=1;
		profile_active[USER_PROFILE][6]=1;
	}
	else
	{
		profile_active[USER_PROFILE][5]=0;
		profile_active[USER_PROFILE][6]=0;
	}

	if(profile_user_Value& 0x20)
	{
		profile_active[USER_PROFILE][11]=1;
		profile_active[USER_PROFILE][12]=1;
	}
	else
	{
		profile_active[USER_PROFILE][11]=0;
		profile_active[USER_PROFILE][12]=0;
	}

	if(profile_user_Value& 0x40)
	{
		profile_active[USER_PROFILE][13]=1;
	}
	else
	{
		profile_active[USER_PROFILE][13]=0;
	}


	if(profile_user_Value& 0x80)
	{
		profile_active[USER_PROFILE][8]=1;
	}
	else
	{
		profile_active[USER_PROFILE][8]=0;
	}
	profile_active[USER_PROFILE][4]=0;
	profile_active[USER_PROFILE][7]=0;
	profile_active[USER_PROFILE][9]=1;
	profile_active[USER_PROFILE][10]=1;
	profile_active[USER_PROFILE][14]=0;
	profile_active[USER_PROFILE][15]=0;
	profile_active[USER_PROFILE][15]=1;

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

	EE_WriteVariable(VirtAddVarTab[ADD_HYS_Temp], DEFAULT_HYS_Temp);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempH], DEFAULT_ENV_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempL], DEFAULT_ENV_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_ENV_active], DEFAULT_ENV_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempH], DEFAULT_CAM_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempL], DEFAULT_CAM_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_CAM_active], DEFAULT_CAM_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempH], DEFAULT_CASE_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempL], DEFAULT_CASE_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_CASE_active], DEFAULT_CASE_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_MB_TempH], DEFAULT_MB_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_MB_TempL], DEFAULT_MB_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_MB_active], DEFAULT_MB_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempH], DEFAULT_TECIN_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempL], DEFAULT_TECIN_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_TECIN_active], DEFAULT_TECIN_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempH], DEFAULT_TECOUT_TempH);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempL], DEFAULT_TECOUT_TempL);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_active], DEFAULT_TECOUT_active);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_0],DEFAULT_PASSWORD_ADMIN_0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_1],DEFAULT_PASSWORD_ADMIN_1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_2],DEFAULT_PASSWORD_ADMIN_2);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_ADMIN_3], DEFAULT_PASSWORD_ADMIN_3);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_0],DEFAULT_PASSWORD_USER_0);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_1],DEFAULT_PASSWORD_USER_1);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_2],DEFAULT_PASSWORD_USER_2);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_3], DEFAULT_PASSWORD_USER_3);HAL_Delay(100);

	EE_WriteVariable(VirtAddVarTab[ADD_DOOR],DEFAULT_DOOR);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PROFILE_USER],DEFAULT_PROFILE_USER);HAL_Delay(100);
	EE_WriteVariable(VirtAddVarTab[ADD_PROFILE_ADMIN],DEFAULT_PROFILE_ADMIN);HAL_Delay(100);

}
