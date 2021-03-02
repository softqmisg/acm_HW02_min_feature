/*
 * log_file.c
 *
 *  Created on: Mar 2, 2021
 *      Author: Administrator
 */

#include "log_file.h"
#include "rtc.h"
#include <string.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRESULT file_write(char *path, char *wstr,uint8_t create) {

	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;

    if((fr = f_open(&myfile, (const TCHAR*) path, FA_WRITE | FA_OPEN_ALWAYS))!=FR_OK)
    {
        /* Seek to end of the file to append data */
    	return fr;
    }
    if((fr = f_lseek(&myfile, f_size(&myfile)))!=FR_OK)
    {
        f_close(&myfile);
        return fr;
    }
	if ((fr = f_write(&myfile, wstr, (UINT)strlen(wstr), (UINT*) &byteswritten))
			!= FR_OK) {
		return fr;
	}
	f_close(&myfile);
	return fr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void generate_filename(RTC_DateTypeDef date, RTC_TimeTypeDef time,
		char *filename) {
	sprintf(filename, "%s %04d_%02d_%02d %02d-%02d-%02d.CSV", filename,
			date.Year + 2000, date.Month, date.Date, time.Hours, time.Minutes,
			time.Seconds);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
char filename_temperature[50] = { 0 };
char filename_volampere[50] = { 0 };
char filename_light[50] = { 0 };
char filename_doorstate[50] = { 0 };
char filename_ledparameter[50] = { 0 };
char filename_relayparameter[50] = { 0 };
char filename_positionparameter[50] = { 0 };
FRESULT Log_file(uint8_t drv, uint8_t filetype, char *str_write) {
	FRESULT fr;
	FATFS *fs;
	FILINFO fno;
	char str_drive[3];
	char str_tmp[100];
	char filename_tmp[50];
	//////////////////////mounting drive//////////////////////////////////
	switch (drv) {
	case SDCARD_DRIVE:
		if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
			printf("mounting sdcard error=%d\n\r", fr);
			return fr;
		}
		sprintf(str_drive, "0:");
		break;
	case USB_DRIVE:
		if ((fr = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1)) != FR_OK) {
			printf("mounting usbh error=%d\n\r", fr);
			return fr;
		}
		sprintf(str_drive, "1:");
		break;
	}
	//////////////////////check log folder exist?//////////////////////////////////
	sprintf(str_tmp, "%s/log", str_drive);
	fr = f_stat(str_tmp, &fno);
	switch (fr) {
	case FR_OK:
		if (fno.fattrib & AM_RDO) {
			printf("directory is read only\n\r");
			return FR_LOCKED;
		}
		break;
	case FR_NO_FILE:
		if ((fr = f_mkdir(str_tmp)) != HAL_OK) {
			printf("f_mkdir error (%d)\n\r", fr);
			return fr;
		}
		break;
	default:
		printf("f_stat(%s) error=%d\n\r", str_tmp,fr);
		return fr;
	}
	/////////////////////////////check file exist or large or need generate ///////////////////////////////////////////
	RTC_DateTypeDef sDate;
	RTC_TimeTypeDef sTime;
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	uint8_t create_new=0;
	switch (filetype) {
	case TEMPERATURE_FILE:
		strcpy(filename_tmp, filename_temperature);
		break;
	case VOLTAMPERE_FILE:
		strcpy(filename_tmp, filename_volampere);
		break;
	case LIGHT_FILE:
		strcpy(filename_tmp, filename_light);
		break;
	case DOORSTATE_FILE:
		strcpy(filename_tmp, filename_doorstate);
		break;
	case LEDPARAMETER_FILE:
		strcpy(filename_tmp, filename_ledparameter);
		break;
	case RELAYPAREMETER_FILE:
		strcpy(filename_tmp, filename_relayparameter);
		break;
	case POSITIONPARAMETER_FILE:
		strcpy(filename_tmp, filename_positionparameter);
		break;
	}
	fr = f_stat(filename_tmp, &fno);
	if((fr==FR_NO_FILE) || (fno.fsize >(FSIZE_t)MAX_FILE_SIZE)|| strlen(filename_tmp)==0)
	{
		create_new=1;
		switch (filetype) {
		case TEMPERATURE_FILE:
			sprintf(filename_temperature,"%s/log/Temperature",str_drive);
			generate_filename(sDate, sTime, filename_temperature);
			strcpy(filename_tmp,filename_temperature);
			break;
		case VOLTAMPERE_FILE:
			sprintf(filename_volampere,"%s/log/Voltampere",str_drive);
			generate_filename(sDate, sTime, filename_volampere);
			strcpy(filename_tmp,filename_volampere);
			break;
		case LIGHT_FILE:
			sprintf(filename_light,"%s/log/light",str_drive);
			generate_filename(sDate, sTime, filename_light);
			strcpy(filename_tmp,filename_light);
			break;
		case DOORSTATE_FILE:
			sprintf(filename_doorstate,"%s/log/doorstate",str_drive);
			generate_filename(sDate, sTime, filename_doorstate);
			strcpy(filename_tmp,filename_doorstate);
			break;
		case LEDPARAMETER_FILE:
			sprintf(filename_ledparameter,"%s/log/led_param",str_drive);
			generate_filename(sDate, sTime, filename_ledparameter);
			strcpy(filename_tmp,filename_ledparameter);
			break;
		case RELAYPAREMETER_FILE:
			sprintf(filename_relayparameter,"%s/log/relay_param",str_drive);
			generate_filename(sDate, sTime, filename_relayparameter);
			strcpy(filename_tmp,filename_relayparameter);
			break;
		case POSITIONPARAMETER_FILE:
			sprintf(filename_positionparameter,"%s/log/position_param",str_drive);
			generate_filename(sDate, sTime, filename_positionparameter);
			strcpy(filename_tmp,filename_positionparameter);
			break;
		}
	}
	else if(fr !=FR_OK)
	{
		printf("f_stat(%s) error=%d\n\r", filename_tmp,fr);
		return fr;
	}
	///////////////////////save data in the file///////////////////////////////
	if((fr=file_write(filename_tmp, str_write,create_new))!=FR_OK)
	{
		printf("writing file error(%d)\n\r",fr);
		return fr;
	}
	if((fr=file_write(filename_tmp, "",0))!=FR_OK)
	{
		printf("writing file error(%d)\n\r",fr);
		return fr;
	}
	//////////////////////unmounting drive//////////////////////////////////
	switch (drv) {
	case SDCARD_DRIVE:
		f_mount(&SDFatFS, "", 1);
		break;
	case USB_DRIVE:
		f_mount(&USBHFatFS, "", 1);
		break;
	}
	return fr;
}
