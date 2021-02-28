/*
 * joystick.h
 *
 *  Created on: Aug 15, 2020
 *      Author: mehdi
 */

#ifndef INC_JOYSTICK_H_
#define INC_JOYSTICK_H_
#include "tim.h"
#define JOYSTICK_TIMER		TIM1
#define KEYS_NUM			5
/*
 * timing
 */
#define KEY_REFRESH_HZ		1000 //Hz
#define MS_COUNTER			1//1000/KEY_REFRESH_HZ
//#define GOMAINMENU_DELAY	10*FREQ_TIMER

////short press ~0.01s->0.4s
#define LIMIT_SHORT_L	10/MS_COUNTER//10ms
#define LIMIT_SHORT_H	400/MS_COUNTER//400ms
/////about 3sec
#define LIMIT_T1_L 800/MS_COUNTER///1.5s
#define LIMIT_T1_H 3500/MS_COUNTER//3.5
//////about >7s
#define LIMIT_T2_L 5000/MS_COUNTER//5s
#define LIMIT_T2_H 10000/MS_COUNTER//10s
/*
 * 	BTN number pinmap
 * 		topleft==TOP
 * 		topright=RIGHT
 * 		downleft=LEFT
 * 		downright=DOWN
 * 		4	0
 *		\	/
 *		  +	1
 *		/	\
 *		3	2
 */

enum {
	Key_No=0x00,
	Key_RIGHT = 0x01,
	Key_ENTER = 0x02,
	Key_DOWN = 0x04,
	Key_LEFT = 0x08,
	Key_TOP = 0x10,
	Key_ALL = Key_RIGHT|Key_ENTER|Key_DOWN|Key_LEFT|Key_TOP
};
enum{
	KNum_RIGHT=0,
	KNum_ENTER=1,
	KNum_DOWN=2,
	KNum_LEFT=3,
	KNum_TOP=4

};
enum press_t {
	no_press=0x00, Short_press = 0x01, Long_press = 0x02,Both_press=0x03
};

enum {
	jostick_initializing = 0, jostick_initialized = 1
};
void joystick_init(uint8_t key,uint8_t presstime);
uint8_t joystick_state();
uint8_t joystick_read(uint8_t key, uint8_t presstime);

#endif /* INC_JOYSTICK_H_ */
