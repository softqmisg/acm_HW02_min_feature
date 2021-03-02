/*
 * log_file.h
 *
 *  Created on: Mar 2, 2021
 *      Author: Administrator
 */

#ifndef INC_LOG_FILE_H_
#define INC_LOG_FILE_H_
#include "main.h"
#include "fatfs.h"

#define MAX_LINE_IN_FILE	7*24*360 //7 day,24 hr, every 10 s
#define MAX_BYTE_IN_LINE	500
#define MAX_FILE_SIZE		MAX_BYTE_IN_LINE*MAX_LINE_IN_FILE //~28M

#define TEMPERATURE_FILE	0
#define VOLTAMPERE_FILE		1
#define LIGHT_FILE			2
#define DOORSTATE_FILE		3
#define LEDPARAMETER_FILE	4
#define RELAYPAREMETER_FILE	5
#define POSITIONPARAMETER_FILE		6

#define SDCARD_DRIVE		0
#define USB_DRIVE			1
FRESULT Log_file(uint8_t drv,uint8_t filetype,char *str);


#endif /* INC_LOG_FILE_H_ */
