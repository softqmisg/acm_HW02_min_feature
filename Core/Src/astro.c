/*
 * astro.c
 *
 *  Created on: Jul 4, 2020
 *      Author: mehdi
 */
#include "astro.h"
#include "rtc.h"
////////////////////////////////////////////////////////////////////////////////////////
double POS2double(POS_t pos)
{
	double r=(double)pos.deg+(double)pos.min/60.0+(double)pos.second/3600.0;
	if(pos.direction=='S' || pos.direction=='W')
		r=-r;
	return r;
}
////////////////////////////////////////////////////////////////////////////////////////
POS_t latdouble2POS(double lat)
{
	POS_t pos;
	if(lat<0) { pos.direction='S'; lat=-lat;}
	else pos.direction='N';
	pos.deg=(uint8_t)lat;
	pos.second=(double)(lat-(double)pos.deg)*3600.0;
	pos.min=(uint8_t)((double)pos.second/60);
	pos.second=pos.second-(double)pos.min*60;
	return pos;
}
////////////////////////////////////////////////////////////////////////////////////////
POS_t longdouble2POS(double lon)
{
	POS_t pos;
	if(lon<0) { pos.direction='W'; lon=-lon;}
	else pos.direction='E';
	pos.deg=(uint8_t)lon;
	pos.second=(double)(lon-(double)pos.deg)*3600.0;
	pos.min=(uint8_t)((double)pos.second/60);
	pos.second=pos.second-(double)pos.min*60;
	return pos;
}

////////////////////////////////////////////////////////////////////////////////////////
uint8_t Astro_daylighsaving(Date_t date)
{
	uint8_t r=0;
	if(date.month>RTC_MONTH_MARCH && date.month<RTC_MONTH_SEPTEMBER) r=1;
	else if(date.month==RTC_MONTH_MARCH && date.day>=22)	r=1;
	else if(date.month==RTC_MONTH_SEPTEMBER && date.day<22)	r=1;
	return r;
}
////////////////////////////////////////////////////////////////////////////////////////
Time_t addtime(Time_t t1,Time_t t2)
{
	Time_t tmp=t1;
	tmp.sec+=t2.sec;
	if(tmp.sec>59)
	{
		tmp.sec-=60;
		tmp.min++;
		if(tmp.min>59)
			tmp.hr++;
	}
	if(tmp.sec<0)
	{
		tmp.sec+=60;
		tmp.min--;
		if(tmp.min<0)
			tmp.min+=60;
	}
	tmp.min+=t2.min;
	if(tmp.min>59)
	{
		tmp.min-=60;
		tmp.hr++;
		if(tmp.hr>=23)
			tmp.hr=0;
	}
	if(tmp.min<0)
	{
		tmp.min+=60;
		tmp.hr--;
		if(tmp.hr<0)
			tmp.hr=0;
	}
	tmp.hr+=t2.hr;
	return tmp;
}
////////////////////////////////////////////////////////////////////////////////////////
uint8_t Astro_CheckDayNight(RTC_TimeTypeDef cur_time, Time_t sunrise_t,Time_t sunset_t,double add_sunrise,double add_sunset)
{

	Time_t sunrise_addtime;
	sunrise_addtime.hr=(int8_t) add_sunrise;sunrise_addtime.min=(int8_t)((double)add_sunrise-(double)sunrise_addtime.hr)*60;	sunrise_addtime.sec=0;
	Time_t sunset_addtime;
	sunset_addtime.hr=(int8_t) add_sunset;sunset_addtime.min=(int8_t)((double)add_sunset-(double)sunset_addtime.hr)*60;	sunset_addtime.sec=0;

	sunrise_t=addtime(sunrise_t,sunrise_addtime);
	sunset_t=addtime(sunset_t,sunset_addtime);

	uint8_t r=ASTRO_NIGHT;
	if(cur_time.Hours>sunrise_t.hr && cur_time.Hours<sunset_t.hr) r=ASTRO_DAY;
	else if(cur_time.Hours==sunrise_t.hr && cur_time.Minutes>=sunrise_t.min) r=ASTRO_DAY;
	else if(cur_time.Hours==sunset_t.hr && cur_time.Minutes<=sunset_t.min) r=ASTRO_DAY;
	return r;
}
//////////////////////////////////////////
double wrap(double x)
{
	while(x>360.0) x=x-360.0;
	return x;
}
Time_t seconds2Time_t(int seconds)
{
	Time_t cur_time;
	cur_time.hr=(int)seconds/3600;
	cur_time.min=((int)seconds-(cur_time.hr*3600))/60;
	cur_time.sec=(int)seconds-cur_time.hr*3600-cur_time.min*60;
	return cur_time;
}
unsigned char  gDaysInMonth[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30,31};
unsigned char  gDaysInMonthLeap[12]={31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30,31};
uint8_t leap(int year)
{
	return (year %4 == 0 && ((year %100 != 0) || (year % 400 == 0))) ? 1 : 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
double datenum(Date_t date)
{
	int yg=date.year;
	int mg=date.month;
	int dg=date.day;
	double days=0.0;
	for( int i=0;i<yg;i++)
		if(leap(i))
			days += (double)366;
		else
			days += (double)365;
 	 for(int i=0;i<(mg-1);i++)
		if(leap(yg))
			days+=(double)gDaysInMonthLeap[i];
		else
			days+=(double)gDaysInMonth[i];
	 days+=(double)dg;

	return (double)days;
}

double daysact(Date_t startdate,Date_t enddate)
{
	double BB=datenum(startdate);//cout<<B<<endl;
	double AA=datenum(enddate);//cout<<A<<endl;
	return (double)AA-(double)BB;
}
///////////////////////////////////////////////////////////////////////////////////////////
double  sind(double degree)
{
	return (double)sin(DEG2RAD*degree);
}
double  tand(double degree)
{
	return (double)tan(DEG2RAD*degree);
}
double  cosd(double degree)
{
	return (double)cos(DEG2RAD*degree);
}
double  asind(double num)
{
	return (double)asin(num)/DEG2RAD;
}
double  acosd(double num)
{
	return (double)acos(num)/DEG2RAD;
}
double  atand(double num)
{
	return (double)atan(num)/DEG2RAD;
}
double rad2deg(double rad)
{
	return (double)rad/DEG2RAD;
}
///////////////////////////////////////////////////////////////////////////////////////////
///lat:arz
///lng:tol
void  Astro_sunRiseSet( double lat, double lng, double UTCoff, Date_t date,Time_t *sunrise_t,Time_t *noon_t,Time_t *sunset_t,uint8_t daylightsave)
{
/*
% Reverse engineered from the NOAA Excel:
% (https://www.esrl.noaa.gov/gmd/grad/solcalc/calcdetails.html)
%
% The formulas are from:
% Meeus, Jean H. Astronomical algorithms. Willmann-Bell, Incorporated, 1991.
% Process input
*/
	Date_t startdate;startdate.year=1899;startdate.month=12;startdate.day=30;

	double nDays=(double)daysact(startdate,  date);

	double nTime_ts = 86400.0;                      // % Number of seconds in the day
	double steps=60.0/(nTime_ts-1);
	double min_noon=0.0,min_sunset=0.0,min_sunrise=0.0;
	double temp=0.0;
	int index=0,min_index_noon=0,min_index_sunrise=0,min_index_sunset=0;
	double E,F,G,I,J,K,L,M,P,Q,R,T,U,V,W,X,KSin_J;
	char tmp_str[20];
	//double AD,AB,AC;
	for(double tArray=0.0;tArray<=1;tArray+=((double)steps),index+=60)
	{
		//% Compute
		//% Letters correspond to colums in the NOAA Excel
		 E=tArray;
		double F = (double)nDays+(double)2415018.5+(double)E-(double)UTCoff/24.0;
		 //F = (double)nDays+(double)2415018.3541666666666666666666667+(double)E;
		 G = (double)(F-2451545.0)/36525.0;
		 I = wrap(280.46646+(double)G*(36000.76983+(double)G*0.0003032));
		 J = 357.52911+G*(35999.05029-0.0001537*G);
		 K = 0.016708634-G*(0.000042037+0.0000001267*G);
		 L = sind(J)*(1.914602-G*(0.004817+0.000014*G))+sind(2.0*J)*(0.019993-0.000101*G)+sind(3.0*J)*0.000289;
		 M = I+L;
		 P = M-0.00569-0.00478*sind(125.04-1934.136*G);
		 Q = 23.0+(26+((21.448-G*(46.815+G*(0.00059-G*0.001813))))/60.0)/60.0;
		 R = Q+0.00256*cosd(125.04-1934.136*G);
		 T = asind(sind(R)*sind(P));
		 U = tand(R/2.0)*tand(R/2.0);
		 KSin_J=K*sind(J);
		 V = 4.0*rad2deg(U*sind(2.0*I)-2.0*KSin_J+4.0*U*KSin_J*cosd(2.0*I)-0.5*U*U*sind(4.0*I)-1.25*K*K*sind(2*J));


		 W = acosd(cosd(90.833)/(cosd(lat)*cosd(T))-tand(lat)*tand(T));
		 X = (720.0-4.0*lng-V+UTCoff*60.0)*60.0;
		// X = (930.0-4.0*lng-V)*60.0;

		//% Results in seconds
		double nTimes_tArray=(double)nTime_ts*tArray;
		temp= fabs(X - nTimes_tArray);
		if((temp<min_noon) || (tArray==0.0))
		{
			min_noon=temp;
			min_index_noon=index;
		}
		temp= fabs(X-round(W*240.0) - nTimes_tArray);
		if(temp<min_sunrise|| (tArray==0.0))
		{
			min_sunrise=temp;
			min_index_sunrise=index;
		}
		temp= fabs(X+round(W*240.0) - nTimes_tArray);
		if(temp<min_sunset|| (tArray==0.0))
		{
			min_sunset=temp;
			min_index_sunset=index;
		}
	}
	uint8_t daylightsaving=0;
	if(daylightsave)
		daylightsaving=Astro_daylighsaving(date);

	*noon_t=seconds2Time_t(min_index_noon); noon_t->hr=noon_t->hr+daylightsaving;
	*sunrise_t=seconds2Time_t(min_index_sunrise); sunrise_t->hr=sunrise_t->hr+daylightsaving;
	*sunset_t=seconds2Time_t(min_index_sunset);		sunset_t->hr=sunset_t->hr+daylightsaving;
}
///////////////////////////////////////////////////////////////////////////////////////
void ShowTime_t(Time_t cur_time,char *str)
{
	sprintf(str,"%02d.%02d\n\r",cur_time.hr,cur_time.min);
}


