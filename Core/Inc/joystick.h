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
#define LIMIT_T1_L 1500/MS_COUNTER///1.5s
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
 *		  O	1
 *		/	\
 *		3	2
 */

enum {
	Key_RIGHT = 0,
	Key_ENTER = 1,
	Key_DOWN = 2,
	Key_LEFT = 3,
	Key_TOP = 4,
	Key_ALL = 5,
	Key_NO = 6,
};

enum press_t {
	no_press, Short_press = 1, Long_press = 2
};

enum {
	jostick_initializing = 0, jostick_initialized = 1
};
void joystick_init(uint8_t key);
uint8_t joystick_state();
uint8_t joystick_read(uint8_t key, uint8_t presstime);

#endif /* INC_JOYSTICK_H_ */
