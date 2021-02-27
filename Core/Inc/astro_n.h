#ifndef _ASTRO_H
#define _ASTRO_H
#define M_PI 3.1415926535897932384626433832795
double calcJD(int year,int month,int day);
double calcSunriseUTC(double JD, double latitude, double longitude);
double calcSunsetUTC(double JD, double latitude, double longitude);
#endif
