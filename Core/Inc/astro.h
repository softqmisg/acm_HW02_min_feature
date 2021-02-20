/*
 * astro.h
 *
 *  Created on: Jul 4, 2020
 *      Author: mehdi
 */

#ifndef INC_ASTRO_H_
#define INC_ASTRO_H_

#include "stm32f4xx_hal.h"

#define PI 3.141592653589793
#define DEG2RAD (double)(2.0 * PI / 360.0)
#define UTCOEFF_TEHRAN 3.5
enum { ASTRO_DAY= 0, ASTRO_NIGHT=1};

typedef struct
{
	int year;
	int month;
	int day;
} Date_t;
typedef struct
{
	int hr;
	int min;
	int sec;
} Time_t;
typedef struct
{
	uint8_t deg;
	uint8_t min;
	double  second;
	char direction;
} POS_t;
double POS2double(POS_t pos);
POS_t longdouble2POS(double lon);
POS_t latdouble2POS(double lat);
void  Astro_sunRiseSet( double lat, double lng, double UTCoff, Date_t date,Time_t *sunrise_t,Time_t *noon_t,Time_t *sunset_t,uint8_t daylightsave);
void ShowTime_t(Time_t cur_time,char *str);
uint8_t leap(int year);
uint8_t Astro_daylighsaving(Date_t date);
uint8_t Astro_CheckDayNight(RTC_TimeTypeDef cur_time, Time_t sunrise_t,Time_t sunset_t);

extern unsigned char  gDaysInMonth[12];
extern unsigned char  gDaysInMonthLeap[12];

#endif /* INC_ASTRO_H_ */
