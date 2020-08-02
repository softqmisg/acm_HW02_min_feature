#ifndef __VCNL4200_H__
	#define __VCNL4200_H__
//	#include "main.h"

	#define VCNL4200_BASEADDRESS	0x51 //7bit address
	
	#define VCNL4200_ALS_CONF					0x00
	#define	VCNL4200_ALS_THDH					0x01
	#define	VCNL4200_ALS_THDL					0x02
	#define VCNL4200_PS_CONF1_CONF2		0x03
	#define VCNL4200_PS_CONF3_MS			0x04	
	#define	VCNL4200_PS_CANC					0x05
	#define VCNL4200_PS_THDL					0x06
	#define VCNL4200_PS_THDH					0x07
	#define VCNL4200_PS_Data					0x08
	#define VCNL4200_ALS_Data					0x09
	#define VCNL4200_WHITE_Data				0x0A
	#define VCNL4200_INT_Flag					0x0D
	#define VCNL4200_ID								0x0E
	///////////////////////////////////////ALS_CONF//////////////////////////////////////////////////////////////////////
	//Ambient Light Sensor Shut Down
enum {VCNL4200_ALS_Shutdown_on = 0, VCNL4200_ALS_Shutdown_off = 1};
//ALS Sensor Interrupt
enum {VCNL4200_ALS_Interrupt_disable = 0, VCNL4200_ALS_Interrupt_enable = 2} ;
//ALS Persistence Setting
 enum {VCNL4200_ALS_Pers_one = 0, VCNL4200_ALS_Pers_two = 4, VCNL4200_ALS_Pers_four = 8, VCNL4200_ALS_Pers_eight = 12} ;
//ALS Interrupt
 enum {VCNL4200_ALS_INT_SWITCH_als = 0, VCNL4200_ALS_INT_SWITCH_white = 32} ;
//ALS Integration Time in Milliseconds
 enum {VCNL4200_ALS_IT_ms50 = 0, VCNL4200_ALS_IT_ms100 = 64, VCNL4200_ALS_IT_ms200 = 128, VCNL4200_ALS_IT_ms400 = 192} ;
//////////////////////////////////////////PS_CONF1///////////////////////////////////////////////////////////////////
 //Proximity sensor Shutdown
 enum {VCNL4200_PS_Shutdown_on = 0, VCNL4200_PS_Shutdown_off = 1} ;
 //Proximity  Snesor Integeration time
enum {VCNL4200_PS_1T = 0, VCNL4200_PS_1_5T = 2,VCNL4200_PS_2T=4,VCNL4200_PS_4T=6,VCNL4200_PS_8T=8,VCNL4200_PS_9T=10} ;
//Proximity sensor inetrrupt persistance Setting
enum {VCNL4200_PS_Pers_1 = 0, VCNL4200_PS_Pers_2 = 16,VCNL4200_PS_Pers_3=32,VCNL4200_PS_Pers_4=48} ;
//Proximity sensor duty on/off
enum {VCNL4200_PS_1_160 = 0, VCNL4200_PS_1_320 = 64,VCNL4200_PS_1_640=128,VCNL4200_PS_1_1280=192} ;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	#define VCNL4200_OK	HAL_OK
	#define VCNL4200_ERROR	HAL_ERROR
	HAL_StatusTypeDef vcnl4200_init(void);
	HAL_StatusTypeDef vcnl4200_read(uint8_t cmd,uint16_t *value);
	
#endif