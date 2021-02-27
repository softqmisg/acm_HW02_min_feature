/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "iwdg.h"
#include "lwip.h"
#include "rtc.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "usb"
#include "retarget.h"
#include "vcnl4200.h"
#include "INA3221.h"
#include "veml6030.h"
#include "astro.h"
#include "tmp275.h"
#include "fonts/font_tahoma.h"
#include "fonts/font_verdana.h"
//#include "images/acm_logo.h"
#include "graphics.h"
#include "st7565.h"
#include "libbmp.h"
#include "joystick.h"
#include "pca9632.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	uint8_t TYPE_Value;
	uint16_t DAY_BRIGHTNESS_Value;
	double DAY_BLINK_Value;
	uint16_t NIGHT_BRIGHTNESS_Value;
	double NIGHT_BLINK_Value;
	double ADD_SUNRISE_Value;
	double ADD_SUNSET_Value;

} LED_t;
typedef struct
{
	double Temperature[2];
	char Edge[2];
	uint8_t active[2];
} RELAY_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WHITE_LED	0
#define IR_LED		1

#define FORM_DELAY_SHOW	4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBH_HandleTypeDef hUsbHostHS;
extern ApplicationTypeDef Appli_state;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t flag_rtc_1s = 0;
//uint8_t flag_rtc_showtemp=1;
//uint8_t flag_rtc_blink=0;
//uint8_t counter_rtc_showtemp=0;
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	flag_rtc_1s = 1;
//	flag_rtc_blink=1-flag_rtc_blink;
//	counter_rtc_showtemp++;
//	if(counter_rtc_showtemp>=5)
//	{
//		flag_rtc_showtemp=1;
//		counter_rtc_showtemp=0;
//	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t usart2_datardy = 0, usart3_datardy = 0;
char ESP_data;
char PC_data;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		USART3->DR = USART2->DR;
		HAL_UART_Receive_IT(&huart2, &ESP_data, 1);
	} else if (huart->Instance == USART3) {
		USART2->DR = USART3->DR;
		HAL_UART_Receive_IT(&huart3, &PC_data, 1);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRESULT file_write_USB(char *path, char *wstr) {
	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;
	if ((fr = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1)) != FR_OK) {
		HAL_Delay(1000);
	}
	if ((fr = f_open(&myfile, (const TCHAR*) path, FA_OPEN_APPEND | FA_WRITE))
			!= FR_OK) {
		HAL_Delay(1000);

	}
	if ((fr = f_write(&myfile, wstr, strlen(wstr), (void*) &byteswritten))
			!= FR_OK) {
		HAL_Delay(1000);
	}
	f_close(&myfile);
	f_mount(&USBHFatFS, "", 1);
	return fr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRESULT file_write_SD(char *path, char *wstr) {

	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;
	if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
		HAL_Delay(1000);
	}
	if ((fr = f_open(&myfile, (const TCHAR*) path, FA_OPEN_APPEND | FA_WRITE))
			!= FR_OK) {
		HAL_Delay(1000);

	}
	if ((fr = f_write(&myfile, wstr, strlen(wstr), (void*) &byteswritten))
			!= FR_OK) {
		HAL_Delay(1000);
	}
	f_close(&myfile);
	f_mount(0, "", 1);
	return fr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
enum {
	DISP_IDLE,
	DISP_FORM1,
	DISP_FORM2,
	DISP_FORM3,
	DISP_FORM4,
	DISP_FORM5,
	DISP_FORM6
} DISP_state = DISP_IDLE;
enum {
	MAIN_MENU = 0,
	PASS_MENU,
	OPTION_MENU,
	POSITION_MENU,
	TIME_MENU,
	LEDS1_MENU,
	LEDS2_MENU,
	RELAY_MENU,
	DOOR_MENU,
	CHANGEPASS_MENU,
	EXIT_MENU
} MENU_state = MAIN_MENU;
#define MENU_ITEMS	8
char *menu[] = { "SET Position", "SET Time", "SET LED S1", "SET LED S2",
		"SET Relay", "SET Door", "SET PASS", "Exit" };

//uint16_t als, white;
//uint8_t buffer[2];
//HAL_StatusTypeDef status;
////////////////////////////////////////////////////////////////////////////////////////////////////////////
Time_t cur_sunrise, cur_sunset, cur_noon;
double voltage, current, temperature[8];
uint8_t counter_change_form = 0;
uint8_t flag_change_form;


////////////////////////////////////////////////////////////////////////////////////////////////////////////

LED_t S1_LED_Value, S2_LED_Value;
RELAY_t RELAY1_Value,RELAY2_Value;
uint8_t TEC_STATE_Value;

double LAT_Value;
double LONG_Value;


char PASSWORD_Value[5];
/////////////////////////////////////read value of parameter from eeprom///////////////////////////////
void update_values(void) {
	LAT_Value = 35.719086;
	LONG_Value = 51.398101;

	S1_LED_Value.TYPE_Value = WHITE_LED;
	S1_LED_Value.DAY_BRIGHTNESS_Value = 0;
	S1_LED_Value.DAY_BLINK_Value = 0.5;
	S1_LED_Value.NIGHT_BRIGHTNESS_Value = 80;
	S1_LED_Value.NIGHT_BLINK_Value = 0;
	S1_LED_Value.ADD_SUNRISE_Value = 1.5;
	S1_LED_Value.ADD_SUNSET_Value = -1.0;

	S2_LED_Value.TYPE_Value = IR_LED;
	S2_LED_Value.DAY_BRIGHTNESS_Value = 0;
	S2_LED_Value.DAY_BLINK_Value = 0.0;
	S2_LED_Value.NIGHT_BRIGHTNESS_Value = 80;
	S2_LED_Value.NIGHT_BLINK_Value = 0.0;
	S2_LED_Value.ADD_SUNRISE_Value = 1.5;
	S2_LED_Value.ADD_SUNSET_Value = -1.0;

	TEC_STATE_Value = 1;
	RELAY1_Value.Temperature[0] = 33.1;
	RELAY1_Value.Edge[0] = 'U';
	RELAY1_Value.active[0]=1;
	RELAY1_Value.Temperature[1] = 0.0;
	RELAY1_Value.Edge[1] = '-';
	RELAY1_Value.active[1]=0;

	RELAY2_Value.Temperature[0] = 33.1;
	RELAY2_Value.Edge[0] = 'D';
	RELAY2_Value.active[0]=1;
	RELAY2_Value.Temperature[1] = 35.1;
	RELAY2_Value.Edge[1] = 'U';
	RELAY2_Value.active[1]=1;


	PASSWORD_Value[0] = '0';
	PASSWORD_Value[1] = '0';
	PASSWORD_Value[2] = '0';
	PASSWORD_Value[3] = '0';
	PASSWORD_Value[4] = 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_cell(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
		uint8_t row, uint8_t col, char colour, bounding_box_t *box) {
	uint8_t step_r = floor((double) height / row);
	uint8_t step_c = floor((double) width / col);
	uint8_t xpos = x, ypos = y;
	for (uint8_t i = 0; i < row; i++) {
		if ((ypos + 2 * step_r) > (y + height)) {
			step_r = height / row;
			if ((ypos + 2 * step_r) > (y + height))
				step_r = (y + height) - ypos;
		}
		xpos = x;
		for (uint8_t j = 0; j < col; j++) {
			if ((xpos + 2 * step_c) > (x + width)) {
				step_c = (width / col);
				if ((xpos + 2 * step_c) > (x + width))
					step_c = (x + width) - xpos;
			}
			box[j + i * col].x1 = xpos;
			box[j + i * col].x2 = xpos + step_c - 1;
			box[j + i * col].y1 = ypos;
			box[j + i * col].y2 = ypos + step_r - 1;
			draw_rectangle(xpos, ypos, xpos + step_c - 1, ypos + step_r - 1,
					colour);
			xpos = xpos + step_c - 1;
		}
		ypos = ypos + step_r - 1;
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum {
	LEFT_ALIGN = 0, RIGHT_ALIGN, CENTER_ALIGN
};
bounding_box_t text_cell(bounding_box_t *pos, uint8_t index, char *str,
		unsigned char *font, uint8_t align, unsigned char text_inv,
		unsigned char box_inv) {
	uint8_t x, y = pos[index].y1
			+ (pos[index].y2 - pos[index].y1 - text_height(str, font)) / 2 + 1;
	uint8_t length_str = text_width(str, font, 1);
	switch (align) {
	case LEFT_ALIGN:
		x = pos[index].x1 + 1;
//		draw_fill(x, y, x + length_str + 2, y + text_height(str, font), 0);
		break;
	case RIGHT_ALIGN:
		x = pos[index].x2 - length_str - 2;
		if (x == 0)
			x = 1;
//		draw_fill(x - 1, y, x + length_str, y + text_height(str, font), 0);
		break;
	case CENTER_ALIGN:
		x = pos[index].x1 + (pos[index].x2 - pos[index].x1 - length_str) / 2;
		if (x == 0)
			x = 1;
//		draw_fill(x - 1, y, x + length_str + 1, y + text_height(str, font), 0);
		break;
	}
	if (box_inv)
		draw_fill(pos[index].x1, pos[index].y1 + 1, pos[index].x2,
				pos[index].y2 - 1, box_inv);
	else
		draw_fill(pos[index].x1 + 1, pos[index].y1 + 1, pos[index].x2 - 1,
				pos[index].y2 - 1, box_inv);

	draw_text(str, x, y, font, 1, text_inv);
	//glcd_refresh();

	bounding_box_t box = { .x1 = pos[index].x1, .x2 = pos[index].x2, .y1 =
			pos[index].y1, .y2 = pos[index].y2 };
	return box;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bounding_box_t create_button(bounding_box_t box, char *str, uint8_t text_inv,
		uint8_t box_inv) {
	draw_rectangle(box.x1, box.y1, box.x2, box.y2, 1);
	return text_cell(&box, 0, str, Tahoma8, CENTER_ALIGN, text_inv, box_inv);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_menu(uint8_t selected, uint8_t clear, bounding_box_t *text_pos) {

	if (clear)
		glcd_blank();
	bounding_box_t *pos_ = (bounding_box_t*) malloc(
			sizeof(bounding_box_t) * 1 * 2);
	create_cell(0, 0, 128, 64, 1, 2, 1, pos_);
	for (uint8_t op = 0; op < MENU_ITEMS; op++) {
		draw_text(menu[op], (op / 5) * 66 + 2, (op % 5) * 12 + 1, Tahoma8, 1,
				0);
		text_pos[op].x1 = (op / 5) * 66 + 2;
		text_pos[op].y1 = (op % 5) * 12 + 1;
		text_pos[op].x2 = text_pos[op].x1 + text_width(menu[op], Tahoma8, 1);
		text_pos[op].y2 = text_pos[op].y1 + text_height(menu[op], Tahoma8);

	}
	if (selected < MENU_ITEMS)
		draw_text(menu[selected], (selected / 5) * 66 + 2,
				(selected % 5) * 12 + 1, Tahoma8, 1, 1);
	free(pos_);
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void clock_cell(bounding_box_t *pos_) {
	char tmp_str[40];
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
	draw_fill(1, 1, 126, 1 + text_height(tmp_str, Tahoma8), 0);

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	sprintf(tmp_str, "  %02d:%02d:%02d", sTime.Hours, sTime.Minutes,
			sTime.Seconds);
	draw_text(tmp_str, 2, 1, Tahoma8, 1, 0);
	sprintf(tmp_str, "%4d-%02d-%02d", sDate.Year + 2000, sDate.Month,
			sDate.Date);
	draw_text(tmp_str, 64, 1, Tahoma8, 1, 0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form1(uint8_t clear) {
	HAL_StatusTypeDef result;
	char tmp_str[40];
	bounding_box_t pos_[8];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2 + pos_[0].y1, 4, 2, 1,
			pos_);

	for (uint8_t i = 0; i < 8; i++) {
		if (tmp275_readTemperature(i, &temperature[i]) == HAL_OK) {
			sprintf(tmp_str, "T(%d)=%+4.1f", i + 1, temperature[i]);
		} else
			sprintf(tmp_str, "T(%d)=---", i + 1);
		text_cell(pos_, i, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form2(uint8_t clear) {
	HAL_StatusTypeDef result;
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(25, pos_[0].y2, 128 - 25, 64 - pos_[0].y2 + pos_[0].y1, 4, 1, 1,
			pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////CH1,7V	//////////////////////////////////////////////////////////
	if (ina3221_readdouble((uint8_t) VOLTAGE_7V, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1fv", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readdouble((uint8_t) CURRENT_7V, &current) == HAL_OK)
		sprintf(tmp_str, "%s     C=%4.3fA", tmp_str, current);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////CH2,12V	//////////////////////////////////////////////////////////
	if (ina3221_readdouble((uint8_t) VOLTAGE_12V, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1fv", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readdouble((uint8_t) CURRENT_12V, &current) == HAL_OK)
		sprintf(tmp_str, "%s     C=%4.3fA", tmp_str, current);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////CH3,3.3V	//////////////////////////////////////////////////////////
	if (ina3221_readdouble((uint8_t) VOLTAGE_3V3, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1fv", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readdouble((uint8_t) CURRENT_3V3, &current) == HAL_OK)
		sprintf(tmp_str, "%s     C=%4.3fA", tmp_str, current);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////TEC,12V	//////////////////////////////////////////////////////////
	if (ina3221_readdouble((uint8_t) VOLTAGE_TEC, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1fv", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readdouble((uint8_t) CURRENT_TEC, &current) == HAL_OK)
		sprintf(tmp_str, "%s     C=%4.3fA", tmp_str, current);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y1, 25, (pos_[3].y2 - pos_[0].y1) + 1, 4, 1, 1,
			pos_);
	text_cell(pos_, 0, "7.0", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 1, "12", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 2, "3.3", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 3, "TEC", Tahoma8, CENTER_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form3(uint8_t clear) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(30, pos_[0].y2, 128 - 30, 64 - pos_[0].y2, 4, 1, 1, pos_);

	sprintf(tmp_str, " %2d%%   %4.1fS", S1_LED_Value.DAY_BRIGHTNESS_Value,
			S1_LED_Value.DAY_BLINK_Value);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%  %4.1fS", S1_LED_Value.NIGHT_BRIGHTNESS_Value,
			S1_LED_Value.NIGHT_BLINK_Value);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%   %4.1fS", S2_LED_Value.DAY_BRIGHTNESS_Value,
			S2_LED_Value.DAY_BLINK_Value);
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%  %4.1fS", S2_LED_Value.NIGHT_BRIGHTNESS_Value,
			S2_LED_Value.NIGHT_BLINK_Value);
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(20, pos_[0].y1, 20, 64 - pos_[0].y1, 4, 1, 1, pos_);
	text_cell(pos_, 0, "D", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 1, "N", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 2, "D", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 3, "N", Tahoma8, CENTER_ALIGN, 1, 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y1, 20, 64 - pos_[0].y1, 2, 1, 1, pos_);
	if (S1_LED_Value.TYPE_Value == WHITE_LED)
		text_cell(pos_, 0, "WT", Tahoma8, CENTER_ALIGN, 0, 0);
	else
		text_cell(pos_, 0, "IR", Tahoma8, CENTER_ALIGN, 0, 0);

	if (S2_LED_Value.TYPE_Value == WHITE_LED)
		text_cell(pos_, 1, "WT", Tahoma8, CENTER_ALIGN, 0, 0);
	else
		text_cell(pos_, 1, "IR", Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form4(uint8_t clear) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128 - 0, 64 - pos_[0].y2 + pos_[0].y1, 4, 1, 1,
			pos_);

	POS_t lat_pos = latdouble2POS(LAT_Value);
	sprintf(tmp_str, " %d %d\' %05.2f\"%c", lat_pos.deg, lat_pos.min,
			lat_pos.second, lat_pos.direction);
	pos_[0].x1 = 52;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = 1;
	pos_[0].x2 = 50;
	text_cell(pos_, 0, "Latitude:", Tahoma8, LEFT_ALIGN, 1, 1);

	POS_t long_pos = longdouble2POS(LONG_Value);
	sprintf(tmp_str, " %d %d\' %05.2f\"%c", long_pos.deg, long_pos.min,
			long_pos.second, long_pos.direction);
	pos_[1].x1 = 52;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = 1;
	pos_[1].x2 = 50;
	text_cell(pos_, 1, "Longitude:", Tahoma8, LEFT_ALIGN, 1, 1);

	sprintf(tmp_str, "%02d:%02d(%+3.1f/%+3.1f)", cur_sunrise.hr, cur_sunrise.min,
			S1_LED_Value.ADD_SUNRISE_Value, S1_LED_Value.ADD_SUNRISE_Value);
	pos_[2].x1 = 42;
	text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[2].x1 = 1;
	pos_[2].x2 = 40;
	text_cell(pos_, 2, "sunrise:", Tahoma8, LEFT_ALIGN, 1, 1);

	sprintf(tmp_str, "%02d:%02d(%+3.1f/%+3.1f)", cur_sunset.hr, cur_sunset.min,
			S2_LED_Value.ADD_SUNSET_Value, S2_LED_Value.ADD_SUNSET_Value);
	pos_[3].x1 = 42;
	text_cell(pos_, 3, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[3].x1 = 1;
	pos_[3].x2 = 40;
	text_cell(pos_, 3, "sunset:", Tahoma8, LEFT_ALIGN, 1, 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form5(uint8_t clear) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128 - 0, 64 - pos_[0].y2 + pos_[0].y1, 4, 1, 1,
			pos_);
	uint16_t vcnl_als;
	if (vcnl4200_als(&vcnl_als) == HAL_OK)
		sprintf(tmp_str, "%04d", vcnl_als);
	else
		sprintf(tmp_str, "----");
	pos_[0].x1 = 90;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = 1;
	pos_[0].x2 = 70;
	text_cell(pos_, 0, "Inside Light:", Tahoma8, LEFT_ALIGN, 1, 1);

	uint16_t veml_als;
	if (veml6030_als(&veml_als) == HAL_OK)
		sprintf(tmp_str, "%04d", veml_als);
	else
		sprintf(tmp_str, "-----");
	pos_[1].x1 = 90;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = 1;
	pos_[1].x2 = 70;
	text_cell(pos_, 1, "Outside Light:", Tahoma8, LEFT_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form6(uint8_t clear) {
	char tmp_str[40];
	bounding_box_t pos_[6];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, ((pos_[0].y2 - pos_[0].y1) + 1), 1, 1, 1,
			pos_);
	(TEC_STATE_Value == 1) ?
			sprintf(tmp_str, "ENABLE") : sprintf(tmp_str, "DISABLE");
	pos_[0].x1 = 60;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = 1;
	pos_[0].x2 = 30;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 1, 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 2, 1, pos_);
	text_cell(pos_, 0, "RELAY1", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 1, "RELAY2", Tahoma8, CENTER_ALIGN, 1, 1);
	for (uint8_t i = 0; i < 2; i++) {
		if(RELAY1_Value.active[i])
		{
			sprintf(tmp_str, "%c %+4.1f", RELAY1_Value.Edge[i],
					RELAY1_Value.Temperature[i]);
			text_cell(pos_, 2 + i * 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
		}
		else
		{
			text_cell(pos_, 2 + i * 2, "- ------", Tahoma8, CENTER_ALIGN, 0, 0);
		}
		if(RELAY2_Value.active[i])
		{
			sprintf(tmp_str, "%c %+4.1f", RELAY2_Value.Edge[i],
					RELAY2_Value.Temperature[i]);
			text_cell(pos_,3 + i * 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
		}
		else
		{
			text_cell(pos_, 3 + i * 2, "- ------", Tahoma8, CENTER_ALIGN, 0, 0);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formpass(uint8_t clear, bounding_box_t *text_pos) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
	pos_[0].x2 = 57;
	text_cell(pos_, 0, "PASSWORD", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 1, 4, 1, pos_);
	for(uint8_t i=0;i<4;i++)
		text_pos[i+2]=text_cell(pos_, i, "*", Tahoma16, CENTER_ALIGN, 0, 0);

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formposition(uint8_t clear, bounding_box_t *text_pos,
		POS_t *tmp_lat, POS_t *tmp_long) {
	char tmp_str[40];
	bounding_box_t pos_[2];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
	pos_[0].x2 = 57;
	text_cell(pos_, 0, "POSITION", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 50, 64 - pos_[0].y2, 2, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 50;
	text_cell(pos_, 0, "Latitude:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 50;
	text_cell(pos_, 1, "Longitude:", Tahoma8, LEFT_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	*tmp_lat = latdouble2POS(LAT_Value);
	*tmp_long = longdouble2POS(LONG_Value);
	create_cell(50, pos_[0].y1, 128 - 50, 64 - pos_[0].y1, 2, 1, 1, pos_);

	sprintf(tmp_str, "%02d", tmp_lat->deg);
	pos_[0].x1 = 52;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[2].x1=pos_[0].x1;text_pos[2].x2=pos_[0].x2;text_pos[2].y1=pos_[0].y1;text_pos[2].y2=pos_[0].y2;

	sprintf(tmp_str, "%02d\'", tmp_lat->min);
	pos_[0].x1 = pos_[0].x2 + 4;
	pos_[0].x2 = pos_[0].x1 + text_width("55\'", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[3].x1=pos_[0].x1;text_pos[3].x2=pos_[0].x2;text_pos[3].y1=pos_[0].y1;text_pos[3].y2=pos_[0].y2;

	sprintf(tmp_str, "%05.2f\"", tmp_lat->second);
	pos_[0].x1 = pos_[0].x2 + 3;
	pos_[0].x2 = pos_[0].x1 + text_width("55.55\"", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[4].x1=pos_[0].x1;text_pos[4].x2=pos_[0].x2;text_pos[4].y1=pos_[0].y1;text_pos[4].y2=pos_[0].y2;

	sprintf(tmp_str, "%c", tmp_lat->direction);
	pos_[0].x1 = pos_[0].x2 + 2;
	pos_[0].x2 = pos_[0].x1 + text_width("N", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[5].x1=pos_[0].x1;text_pos[5].x2=pos_[0].x2;text_pos[5].y1=pos_[0].y1;text_pos[5].y2=pos_[0].y2;

	sprintf(tmp_str, "%02d", tmp_long->deg);
	pos_[1].x1 = 52;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[6].x1=pos_[1].x1;text_pos[6].x2=pos_[1].x2;text_pos[6].y1=pos_[1].y1;text_pos[6].y2=pos_[1].y2;

	sprintf(tmp_str, "%02d\'", tmp_long->min);
	pos_[1].x1 = pos_[1].x2 + 4;
	pos_[1].x2 = pos_[1].x1 + text_width("55\'", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[7].x1=pos_[1].x1;text_pos[7].x2=pos_[1].x2;text_pos[7].y1=pos_[1].y1;text_pos[7].y2=pos_[1].y2;

	sprintf(tmp_str, "%05.2f\"", tmp_long->second);
	pos_[1].x1 = pos_[1].x2 + 3;
	pos_[1].x2 = pos_[1].x1 + text_width("55.55\"", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[8].x1=pos_[1].x1;text_pos[8].x2=pos_[1].x2;text_pos[8].y1=pos_[1].y1;text_pos[8].y2=pos_[1].y2;

	sprintf(tmp_str, "%c", tmp_long->direction);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("W", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[9].x1=pos_[1].x1;text_pos[9].x2=pos_[1].x2;text_pos[9].y1=pos_[1].y1;text_pos[9].y2=pos_[1].y2;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formTime(uint8_t clear, bounding_box_t *text_pos,
		RTC_TimeTypeDef *tmp_time, RTC_DateTypeDef *tmp_date) {
	char tmp_str[40];
	bounding_box_t pos_[2];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
	pos_[0].x2 = 57;
	text_cell(pos_, 0, "TIME", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 40, 64 - pos_[0].y2, 2, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 40;
	text_cell(pos_, 0, "CLOCK:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 40;
	text_cell(pos_, 1, "Date:", Tahoma8, LEFT_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HAL_RTC_GetTime(&hrtc, tmp_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, tmp_date, RTC_FORMAT_BIN);

	create_cell(40, pos_[0].y1, 128 - 40, 64 - pos_[0].y1, 2, 1, 1, pos_);

	sprintf(tmp_str, "%02d", tmp_time->Hours);
	pos_[0].x1 = 42;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 3;
	text_cell(pos_, 0, ":", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_time->Minutes);
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 3;
	text_cell(pos_, 0, ":", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_time->Seconds);
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%04d", tmp_date->Year + 2000);
	pos_[1].x1 = 42;
	pos_[1].x2 = pos_[1].x1 + text_width("5555", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 4;
	text_cell(pos_, 1, "-", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_date->Month);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 4;
	text_cell(pos_, 1, "-", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_date->Date);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formLEDS1(uint8_t clear, bounding_box_t *text_pos, LED_t *tmp_led) {
//	tmp_led->TYPE_Value=S1_LED_Value.TYPE_Value;
//	tmp_led->ADD_SUNRISE_Value=S1_LED_Value.ADD_SUNRISE_Value;
//	tmp_led->ADD_SUNSET_Value=S1_LED_Value.ADD_SUNSET_Value;
//	tmp_led->DAY_BLINK_Value=S1_LED_Value.DAY_BLINK_Value;
//	tmp_led->DAY_BRIGHTNESS_Value=S1_LED_Value.DAY_BRIGHTNESS_Value;
//	tmp_led->NIGHT_BLINK_Value=S1_LED_Value.NIGHT_BLINK_Value;
//	tmp_led->NIGHT_BRIGHTNESS_Value=S1_LED_Value.NIGHT_BRIGHTNESS_Value;

	char tmp_str[40];
	bounding_box_t pos_ [3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "LED SET1", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 35, 64 - pos_[0].y2, 3, 1, 1, pos_);
	glcd_refresh();

	pos_[0].x1 = 0;
	pos_[0].x2 = 34;
	text_cell(pos_, 0, "SET1:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[1].x1 = 0;
	pos_[1].x2 = 34;
	text_cell(pos_, 1, "DAY:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x1 = 0;
	pos_[2].x2 = 34;
	text_cell(pos_, 2, "NIGHT:", Tahoma8, LEFT_ALIGN, 1, 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(35, pos_[0].y1, 128 - 35, 64 - pos_[0].y1, 3, 1, 1, pos_);

	if (tmp_led->TYPE_Value == WHITE_LED)
		sprintf(tmp_str, "WHITE");
	else
		sprintf(tmp_str, "IR");

	pos_[0].x1 = 50;
	pos_[0].x2 = pos_[0].x1 + text_width("WHITE", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%03d", tmp_led->DAY_BRIGHTNESS_Value);
	pos_[1].x1 = 37;
	pos_[1].x2 = pos_[1].x1 + text_width("055", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 11;
	text_cell(pos_, 1, "%", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_led->DAY_BLINK_Value);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 6;
	text_cell(pos_, 1, "S", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNRISE_Value);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("+5.5", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 11;
	text_cell(pos_, 1, "hr", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%03d", tmp_led->NIGHT_BRIGHTNESS_Value);
	pos_[2].x1 = 37;
	pos_[2].x2 = pos_[2].x1 + text_width("055", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 11;
	text_cell(pos_, 2, "%", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_led->NIGHT_BLINK_Value);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 6;
	text_cell(pos_, 2, "S", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNSET_Value);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("+5.5", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 11;
	text_cell(pos_, 2, "hr", Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formLEDS2(uint8_t clear, bounding_box_t *text_pos, LED_t *tmp_led) {

	char tmp_str[40];
	bounding_box_t pos_ [3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "LED SET2", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 35, 64 - pos_[0].y2, 3, 1, 1, pos_);

	pos_[0].x1 = 0;
	pos_[0].x2 = 34;
	text_cell(pos_, 0, "SET1:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[1].x1 = 0;
	pos_[1].x2 = 34;
	text_cell(pos_, 1, "DAY:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x1 = 0;
	pos_[2].x2 = 34;
	text_cell(pos_, 2, "NIGHT:", Tahoma8, LEFT_ALIGN, 1, 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(35, pos_[0].y1, 128 - 35, 64 - pos_[0].y1, 3, 1, 1, pos_);

	if (tmp_led->TYPE_Value == WHITE_LED)
		sprintf(tmp_str, "WHITE");
	else
		sprintf(tmp_str, "IR");

	pos_[0].x1 = 50;
	pos_[0].x2 = pos_[0].x1 + text_width("WHITE", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%03d", tmp_led->DAY_BRIGHTNESS_Value);
	pos_[1].x1 = 37;
	pos_[1].x2 = pos_[1].x1 + text_width("055", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 11;
	text_cell(pos_, 1, "%", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_led->DAY_BLINK_Value);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 6;
	text_cell(pos_, 1, "S", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNRISE_Value);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("+5.5", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 11;
	text_cell(pos_, 1, "hr", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%03d", tmp_led->NIGHT_BRIGHTNESS_Value);
	pos_[2].x1 = 37;
	pos_[2].x2 = pos_[2].x1 + text_width("055", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 11;
	text_cell(pos_, 2, "%", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_led->NIGHT_BLINK_Value);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 6;
	text_cell(pos_, 2, "S", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNSET_Value);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("+5.5", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 11;
	text_cell(pos_, 2, "hr", Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_formRelay(uint8_t clear, bounding_box_t *text_pos, RELAY_t tmp_Relay1,RELAY_t tmp_Relay2,uint8_t tmp_tec) {
	char tmp_str[40];
	bounding_box_t pos_ [3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "RELAY", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 40, 64 - pos_[0].y2, 3, 1, 1, pos_);

	pos_[0].x1 = 0;
	pos_[0].x2 = 39;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[1].x1 = 0;
	pos_[1].x2 = 39;
	text_cell(pos_, 1, "RELAY1:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x1 = 0;
	pos_[2].x2 = 39;
	text_cell(pos_, 2, "RELAY2:", Tahoma8, LEFT_ALIGN, 1, 1);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(40, pos_[0].y1, 128 - 40, 64 - pos_[0].y1, 3, 1, 1, pos_);
	if (tmp_tec == 1)
		sprintf(tmp_str, "ENABLE");
	else
		sprintf(tmp_str, "DISABLE");

	pos_[0].x1 = 50;
	pos_[0].x2 = pos_[0].x1 + text_width("DISABLE", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	if(tmp_Relay1.active[0])
		sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
	else
		sprintf(tmp_str, "-");

	pos_[1].x1 = 42;
	pos_[1].x2 = pos_[1].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if(tmp_Relay1.active[0])
		sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
	else
		sprintf(tmp_str, "------");
	pos_[1].x1 = pos_[1].x2+1;
	pos_[1].x2 = pos_[1].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[4]=text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	if(tmp_Relay1.active[1])
		sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
	else
		sprintf(tmp_str, "-");
	pos_[1].x1 = pos_[1].x2 + 12;
	pos_[1].x2 = pos_[1].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if(tmp_Relay1.active[1])
		sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);
	else
		sprintf(tmp_str, "------");
	pos_[1].x1 = pos_[1].x2+1;
	pos_[1].x2 = pos_[1].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[6]=text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	////////////////////////////////////////
	if(tmp_Relay2.active[0])
		sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
	else
		sprintf(tmp_str, "-");

	pos_[2].x1 = 42;
	pos_[2].x2 = pos_[2].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if(tmp_Relay2.active[0])
		sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
	else
		sprintf(tmp_str, "------");
	pos_[2].x1 = pos_[2].x2+1;
	pos_[2].x2 = pos_[2].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[8]=text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	if(tmp_Relay2.active[1])
		sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
	else
		sprintf(tmp_str, "-");
	pos_[2].x1 = pos_[2].x2 + 12;
	pos_[2].x2 = pos_[2].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if(tmp_Relay2.active[1])
		sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
	else
		sprintf(tmp_str, "------");
	pos_[2].x1 = pos_[2].x2+1;
	pos_[2].x2 = pos_[2].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[10]=text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_I2C3_Init();
	MX_TIM4_Init();
	MX_RTC_Init();
	MX_SDIO_SD_Init();
	MX_FATFS_Init();
	MX_USB_DEVICE_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_USB_HOST_Init();
#if __LWIP__
	MX_LWIP_Init();
#endif
#if !(__DEBUG__)
  MX_IWDG_Init();
#endif
	/* USER CODE BEGIN 2 */
	/////////////////////////
	bounding_box_t pos_[10];
	uint8_t backlight = 100;
	char tmp_str[100], tmp_str1[100];
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;
	uint8_t y = 0, x = 0;
	uint8_t pre_daylightsaving = 0, cur_daylightsaving = 0;

	uint8_t index_option = 0;
	char tmp_pass[7] = { '*', '*', '*', '*', '*', '*', 0 };
	POS_t tmp_lat, tmp_long;
	RTC_TimeTypeDef tmp_time;
	RTC_DateTypeDef tmp_Date;
	LED_t tmp_LED;
	RELAY_t tmp_Relay1,tmp_Relay2;
	uint8_t tmp_TEC_STATE;
	bounding_box_t text_pos[12];
	HAL_StatusTypeDef status;
	Time_t cur_time_t;
	Date_t cur_date_t;
	//////////////////////retarget////////////////
	RetargetInit(&huart3);
	//////////////////////init LCD//////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);

//	//////////////////////////load Logo/////////////////////////////////////////
#if !(__DEBUG__)
	HAL_IWDG_Refresh(&hiwdg);
#endif
	if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
		printf("error mount SD\n\r");
	} else {
		bmp_img img;
		if (bmp_img_read(&img, "logo.bmp") == BMP_OK)
			draw_bmp_h(0, 0, img.img_header.biWidth, img.img_header.biHeight,
					img.img_pixels, 1);
		else
			printf("bmp file error\n\r");
		f_mount(&SDFatFS, "", 1);
		bmp_img_free(&img);
		create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
		glcd_refresh();
		HAL_Delay(1000);
	}
	glcd_blank();
/////////////////////////transceiver PC<->ESP32/////////////////////////////
	HAL_UART_Transmit(&huart3, (uint8_t*) "\033[0;0H", strlen("\033[0;0H"),
	HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart3, (uint8_t*) "\033[2J", strlen("\033[2J"),
	HAL_MAX_DELAY);
	HAL_GPIO_WritePin(ESP32_EN_GPIO_Port, ESP32_EN_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(ESP32_EN_GPIO_Port, ESP32_EN_Pin, GPIO_PIN_SET);
	HAL_UART_Receive_IT(&huart3, (uint8_t*) &PC_data, 1);
	HAL_UART_Receive_IT(&huart2, (uint8_t*) &ESP_data, 1);
	///////////////////////initialize & checking sensors///////////////////////////////////////
//	create_cell(0, 0, 128, 13, 1, 2, 1, pos_);
//	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2 + pos_[0].y1, 4, 2, 1,
//			&pos_[2]);
	create_cell(0, 0, 128, 64, 4, 2, 1, pos_);
	uint8_t ch, inv;
	for (ch = TMP_CH0; ch <= TMP_CH7; ch++) {
		if ((status = tmp275_init(ch)) != TMP275_OK) {
			printf("tmp275 sensor (#%d) error\n\r", ch + 1);
			sprintf(tmp_str1, "tmp(%d)ERR", ch + 1);
			inv = 1;

		} else {
			printf("tmp275 sensor (#%d) OK\n\r", ch + 1);
			sprintf(tmp_str1, "tmp(%d)OK", ch + 1);
			inv = 0;
		}
		text_cell(pos_, ch, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);
		HAL_Delay(50);
	}

	glcd_refresh();
	HAL_Delay(3000);

	glcd_blank();
	create_cell(0, 0, 128, 64, 4, 2, 1, pos_);

	if ((status = vcnl4200_init()) != VCNL4200_OK) {
		printf("vcnl4200 sensor error\n\r");
		sprintf(tmp_str1, "vcnl ERR");
		inv = 1;
	} else {
		printf("vcnl4200 sensor OK\n\r");
		sprintf(tmp_str1, "vcnl OK");
		inv = 0;
	}
	text_cell(pos_, 0, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);
	if ((status = veml6030_init()) != VEML6030_OK) {
		printf("veml6030 sensor error\n\r");
		sprintf(tmp_str1, "veml ERR");
		inv = 1;

	} else {
		printf("veml6030 sensor OK\n\r");
		sprintf(tmp_str1, "vcnl OK");
		inv = 0;
	}
	text_cell(pos_, 1, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);

	if ((status = pca9632_init()) != HAL_OK) {
		printf("pca9631 sensor error\n\r");
		sprintf(tmp_str1, "pca9 ERR");
		inv = 1;

	} else {
		printf("pca9631 sensor OK\n\r");
		sprintf(tmp_str1, "pca9 OK");
		inv = 0;
	}
	text_cell(pos_, 2, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);
	status = ina3221_init(INA3221_LED_BASEADDRESS);

	if((status = ina3221_init(INA3221_LED_BASEADDRESS))!=HAL_OK)
	{
		printf("ina3221 LED sensor error\n\r");
		sprintf(tmp_str1, "ina_1 ERR");
		inv = 1;
	} else {
		printf("ina3221 sensor OK\n\r");
		sprintf(tmp_str1, "ina_1 OK");
		inv = 0;
	}
	text_cell(pos_, 3, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);

	if((status = ina3221_init(INA3221_TEC_BASEADDRESS))!=HAL_OK)
	{
		printf("ina3221 TEC sensor error\n\r");
		sprintf(tmp_str1, "ina_2 ERR");
		inv = 1;

	} else {
		printf("ina3221 sensor OK\n\r");
		sprintf(tmp_str1, "ina_2 OK");
		inv = 0;
	}
	text_cell(pos_, 4, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);

	glcd_refresh();
	HAL_Delay(3000);
	///////////////////////////RTC interrupt enable///////////////////////////////////////////////////
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS)
			!= HAL_OK) {
		Error_Handler();
	}
	/////////////////////////////eeprom reading//////////////////////////////////////////////////////
	if ((HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x5050)) {
#if !(__DEBUG__)
		HAL_IWDG_Refresh(&hiwdg);
#endif
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5050);

	}
	update_values();
	//////////////////////////////////////////////////////////////////////////////////
	joystick_init(Key_ALL, Both_press);
	flag_change_form = 1;
	flag_rtc_1s=1;
	while (1) {
		switch (MENU_state) {
		/////////////////////////////////////MAIN_MENU/////////////////////////////////////////////////
		case MAIN_MENU:
			////////////////////////////1S flag//////////////////////////////////////////////////
			if (flag_rtc_1s) {
				flag_rtc_1s = 0;
				counter_change_form++;
				if (counter_change_form > FORM_DELAY_SHOW) {
					counter_change_form = 0;
					flag_change_form = 1;
				}
				////////////////////////////RTC//////////////////////////////////
				HAL_RTC_GetDate(&hrtc, &cur_Date, RTC_FORMAT_BIN);
				 cur_date_t.day = cur_Date.Date; cur_date_t.month =	cur_Date.Month;cur_date_t.year = cur_Date.Year;
				cur_daylightsaving = Astro_daylighsaving(cur_date_t);
				if (cur_daylightsaving && !pre_daylightsaving) {
					if (READ_BIT(hrtc.Instance->CR, RTC_CR_BKP) != RTC_CR_BKP) {
						__HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
						SET_BIT(hrtc.Instance->CR, RTC_CR_ADD1H);
						SET_BIT(hrtc.Instance->CR, RTC_CR_BKP);
						CLEAR_BIT(hrtc.Instance->CR, RTC_CR_SUB1H);
						__HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
					}
					pre_daylightsaving = cur_daylightsaving;
				}
				if (!cur_daylightsaving && pre_daylightsaving) {
					if (READ_BIT(hrtc.Instance->CR, RTC_CR_BKP) != RTC_CR_BKP) {
						__HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
						SET_BIT(hrtc.Instance->CR, RTC_CR_SUB1H);
						SET_BIT(hrtc.Instance->CR, RTC_CR_BKP);
						CLEAR_BIT(hrtc.Instance->CR, RTC_CR_ADD1H);
						__HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
					}
				}
				HAL_RTC_GetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
				cur_time_t.hr=cur_time.Hours;cur_time_t.min=cur_time.Minutes;cur_time_t.sec=cur_time.Seconds;
				////////////////////////////LED control////////////////////////////////////
				if(cur_time_t.hr==0 && cur_time_t.min==0 && cur_time_t.sec==0)
					Astro_sunRiseSet(LAT_Value, LONG_Value, +3.5, cur_date_t, &cur_sunrise, &cur_noon,&cur_sunset, 1);
				if(Astro_CheckDayNight(cur_time,cur_sunrise,cur_sunset,S1_LED_Value.ADD_SUNRISE_Value,S1_LED_Value.ADD_SUNSET_Value)==ASTRO_DAY)
				{
					pca9632_setbrighnessblinking(LEDS1, S1_LED_Value.DAY_BRIGHTNESS_Value, S1_LED_Value.DAY_BLINK_Value);
				}
				else
				{
					pca9632_setbrighnessblinking(LEDS1, S1_LED_Value.NIGHT_BRIGHTNESS_Value, S1_LED_Value.NIGHT_BLINK_Value);
				}
				if(Astro_CheckDayNight(cur_time,cur_sunrise,cur_sunset,S1_LED_Value.ADD_SUNRISE_Value,S1_LED_Value.ADD_SUNSET_Value)==ASTRO_DAY)
				{
					pca9632_setbrighnessblinking(LEDS2, S2_LED_Value.DAY_BRIGHTNESS_Value, S2_LED_Value.DAY_BLINK_Value);
				}
				else
				{
					pca9632_setbrighnessblinking(LEDS2, S2_LED_Value.NIGHT_BRIGHTNESS_Value, S2_LED_Value.NIGHT_BLINK_Value);

				}
				////////////////////////////DISP Refresh//////////////////////////////////
				switch (DISP_state) {
				case DISP_IDLE:
					break;
				case DISP_FORM1:
					create_form1(0);
					break;
				case DISP_FORM2:
					create_form2(0);
					break;
				case DISP_FORM3:
					create_form3(0);
					break;
				case DISP_FORM4:
					create_form4(0);
					break;
				case DISP_FORM5:
					create_form5(0);
					break;
				case DISP_FORM6:
					create_form6(0);
					break;
				}
			}
			if (flag_change_form) {
				flag_change_form = 0;
				switch (DISP_state) {
				case DISP_IDLE:
					Astro_sunRiseSet(LAT_Value, LONG_Value, +3.5, cur_date_t, &cur_sunrise, &cur_noon,&cur_sunset, 1);
					create_form1(1);
					DISP_state = DISP_FORM1;
					break;
				case DISP_FORM1:
					create_form2(1);
					DISP_state = DISP_FORM2;
					break;
				case DISP_FORM2:
					create_form3(1);
					DISP_state = DISP_FORM3;
					break;
				case DISP_FORM3:
					create_form4(1);
					DISP_state = DISP_FORM4;
					break;
				case DISP_FORM4:
					create_form5(1);
					DISP_state = DISP_FORM5;
					break;
				case DISP_FORM5:
					create_form6(1);
					DISP_state = DISP_FORM6;
					break;
				case DISP_FORM6:
					create_form1(1);
					DISP_state = DISP_FORM1;
					break;
				}
			}
			//////////////////////////////////////////////////////////////////////////////
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT,
					Both_press);
			if (joystick_read(Key_ENTER, Long_press)
					|| joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Both_press);
				sprintf(tmp_pass, "0000");
				create_formpass(1, text_pos);
				index_option = 2;
//				draw_fill(text_pos[index_option].x1, text_pos[index_option].y1,
//						text_pos[index_option].x2, text_pos[index_option].y2,
//						0);
//				draw_char(tmp_pass[index_option - 2], text_pos[index_option].x1,
//						text_pos[index_option].y1, Tahoma16, 1);
				sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
				text_cell(text_pos, index_option, tmp_str, Tahoma16,
										CENTER_ALIGN, 1, 0);
				glcd_refresh();
				MENU_state = PASS_MENU;

			}
			break;
			/////////////////////////////////////PASS_MENU/////////////////////////////////////////////////
		case PASS_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {

					tmp_pass[index_option - 2] = (char) tmp_pass[index_option
							- 2] + 1;
					if (tmp_pass[index_option - 2] > '9' && tmp_pass[index_option - 2] < 'A')
						tmp_pass[index_option - 2] = 'A';
					else if (tmp_pass[index_option - 2] > 'Z' && tmp_pass[index_option - 2] < 'a')
						tmp_pass[index_option - 2] = 'a';
					if (tmp_pass[index_option - 2] > 'z')
						tmp_pass[index_option - 2] = '0';
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					draw_fill(text_pos[index_option].x1+1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);

					glcd_refresh();
				}

			}
			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					tmp_pass[index_option - 2] = (char) tmp_pass[index_option
							- 2] - 1;
					if (tmp_pass[index_option - 2] < '0')
						tmp_pass[index_option - 2] = 'z';
					else if (tmp_pass[index_option - 2] < 'a' && tmp_pass[index_option - 2] > 'Z')
						tmp_pass[index_option - 2] = 'Z' ;
					else if (tmp_pass[index_option - 2] < 'A' && tmp_pass[index_option - 2] > '9')
						tmp_pass[index_option - 2] = '9' ;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );

					draw_fill(text_pos[index_option].x1+ 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
				}

			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if(index_option>1)
				{
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 0, 0);
				}
				switch(index_option)
				{
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN,
							1, 1);
					index_option = 1;
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN,
							0, 0);
					index_option = 2;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					break;
				case 5:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN,
							1, 1);
					index_option = 0;
					break;
				default:
					index_option++;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					break;
				}
				if(index_option>1)
				{
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();

			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if(index_option>1)
				{
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 0, 0);
				}
				switch(index_option)
				{
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0,
							0);

					index_option = 5;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN,
							0, 0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 0;
					break;
				case 2:
					index_option = 1;
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN,
							1, 1);
					break;
				default:
					index_option--;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					break;
				}
				if(index_option>1)
				{
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
				}

				glcd_refresh();

			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if(index_option>1)
				{
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					if (strcmp(tmp_pass, PASSWORD_Value)) {
						bounding_box_t tmp_box = { .x1 = 0, .x2 = 127, .y1 = 51,
								.y2 = 63 };
						text_cell(&tmp_box, 0, "Wrong password", Tahoma8,
								CENTER_ALIGN, 0, 0);
						glcd_refresh();
						HAL_Delay(2000);
						MENU_state = MAIN_MENU;
						DISP_state = DISP_IDLE;
						flag_change_form = 1;
					} else {
						create_menu(0, 1, text_pos);
						index_option = 0;
						MENU_state = OPTION_MENU;
					}

					/////////////////////////////////////////////////////////////////////////
					break;
				case 1:
					MENU_state = MAIN_MENU;
					DISP_state = DISP_IDLE;
					flag_change_form = 1;
					break;
				case 5:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN,
							1, 1);
					index_option = 0;
					break;
				default:
					index_option++;
					sprintf(tmp_str,"%c",tmp_pass[index_option - 2] );
					break;
				}

//				default:
//					draw_fill(text_pos[index_option].x1,
//							text_pos[index_option].y1,
//							text_pos[index_option].x2,
//							text_pos[index_option].y2, 0);
//					draw_char(tmp_pass[index_option - 2],
//							text_pos[index_option].x1,
//							text_pos[index_option].y1, Tahoma16, 0);
//					glcd_refresh();
//					index_option++;
//					if (index_option > 5)
//						index_option = 0;
//					if (index_option == 0) {
//						text_cell(&text_pos[0], 0, "OK", Tahoma8, CENTER_ALIGN,
//								1, 1);
//					} else {
//						draw_fill(text_pos[index_option].x1,
//								text_pos[index_option].y1,
//								text_pos[index_option].x2,
//								text_pos[index_option].y2, 0);
//						draw_char(tmp_pass[index_option - 2],
//								text_pos[index_option].x1,
//								text_pos[index_option].y1, Tahoma16, 1);
//
//					}
//					glcd_refresh();
//					break;
//				}
				if(index_option>1)
				{
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();

			}
			break;
			/////////////////////////////////////OPTION_MENU/////////////////////////////////////////////////
		case OPTION_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				index_option++;
				if (index_option >= MENU_ITEMS)
					index_option = 0;
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				if (index_option == 0)
					index_option = MENU_ITEMS;
				index_option--;

				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				if (index_option < 5) {
					index_option += 5;
					if (index_option >= MENU_ITEMS)
						index_option = index_option % 5;
				} else
					index_option -= 5;
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				index_option += 5;
				if (index_option >= MENU_ITEMS)
					index_option = index_option % 5;
				draw_text(menu[index_option], (index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				index_option += (uint8_t) POSITION_MENU;
				switch (index_option) {
				case POSITION_MENU:
					create_formposition(1, text_pos, &tmp_lat, &tmp_long);
					index_option = 2;

					sprintf(tmp_str, "%02d", tmp_lat.deg);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = POSITION_MENU;
					break;
				case TIME_MENU:

					create_formTime(1, text_pos, &tmp_time, &tmp_Date);
					index_option = 2;

					sprintf(tmp_str, "%02d", tmp_time.Hours);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = TIME_MENU;
					break;
				case LEDS1_MENU:
					tmp_LED = S1_LED_Value;
					create_formLEDS1(1, text_pos, &tmp_LED);
					index_option = 2;

					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = LEDS1_MENU;
					break;
				case LEDS2_MENU:
					tmp_LED = S2_LED_Value;
					create_formLEDS2(1, text_pos, &tmp_LED);
					index_option = 2;

					if (tmp_LED.TYPE_Value== WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = LEDS2_MENU;
					break;
				case RELAY_MENU:
					tmp_Relay1=RELAY1_Value;
					tmp_Relay2=RELAY2_Value;
					tmp_TEC_STATE=TEC_STATE_Value;
					create_formRelay(1, text_pos, tmp_Relay1,tmp_Relay2,tmp_TEC_STATE);
					index_option = 2;

					if (tmp_TEC_STATE== 1)
						sprintf(tmp_str, "ENABLE");
					else
						sprintf(tmp_str, "DISABLE");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = RELAY_MENU;
					break;
				case EXIT_MENU:
					MENU_state = MAIN_MENU;
					DISP_state = DISP_IDLE;
					flag_change_form = 1;

					break;
				}
			}
			break;
			/////////////////////////////////////POSITION_MENU/////////////////////////////////////////////////
		case POSITION_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_lat.deg++;
					if (tmp_lat.deg > 89)
						tmp_lat.deg = 0;
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					break;
				case 3:
					tmp_lat.min++;
					if (tmp_lat.min >= 60)
						tmp_lat.min = 0;
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					break;
				case 4:
					tmp_lat.second += 0.01;
					if (tmp_lat.second >= 60.00)
						tmp_lat.second = 0.00;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					break;
				case 5:
					if (tmp_lat.direction == 'N')
						tmp_lat.direction = 'S';
					else
						tmp_lat.direction = 'N';
					sprintf(tmp_str, "%c", tmp_lat.direction);
					break;
				case 6:
					tmp_long.deg++;
					if (tmp_long.deg > 179)
						tmp_long.deg = 0;
					sprintf(tmp_str, "%02d", tmp_long.deg);
					break;
				case 7:
					tmp_long.min++;
					if (tmp_long.min >= 60)
						tmp_long.min = 0;
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					break;
				case 8:
					tmp_long.second += 0.01;
					if (tmp_long.second >= 60.00)
						tmp_long.second = 0.00;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Long_press)) {
				joystick_init(Key_TOP, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}

				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_lat.deg += 10;
					if (tmp_lat.deg > 89)
						tmp_lat.deg = 0;
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					break;
				case 3:
					tmp_lat.min += 10;
					if (tmp_lat.min >= 60)
						tmp_lat.min = 0;
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					break;
				case 4:
					tmp_lat.second += 1.0;
					if (tmp_lat.second >= 60.00)
						tmp_lat.second = 0.00;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					break;
				case 5:
					if (tmp_lat.direction == 'N')
						tmp_lat.direction = 'S';
					else
						tmp_lat.direction = 'N';
					sprintf(tmp_str, "%c", tmp_lat.direction);
					break;
				case 6:
					tmp_long.deg += 10;
					if (tmp_long.deg > 179)
						tmp_long.deg = 0;
					sprintf(tmp_str, "%02d", tmp_long.deg);
					break;
				case 7:
					tmp_long.min += 10;
					if (tmp_long.min >= 60)
						tmp_long.min = 0;
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					break;
				case 8:
					tmp_long.second += 1.0;
					if (tmp_long.second >= 60.00)
						tmp_long.second = 0.00;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}

				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_lat.deg == 0)
						tmp_lat.deg = 90;
					tmp_lat.deg--;
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					break;
				case 3:
					if (tmp_lat.min == 0)
						tmp_lat.min = 60;
					tmp_lat.min--;
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					break;
				case 4:
					if (tmp_lat.second == 0.00)
						tmp_lat.second = 60.00;
					tmp_lat.second -= 0.01;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					break;
				case 5:
					if (tmp_lat.direction == 'N')
						tmp_lat.direction = 'S';
					else
						tmp_lat.direction = 'N';
					sprintf(tmp_str, "%c", tmp_lat.direction);
					break;
				case 6:
					if (tmp_long.deg == 0)
						tmp_long.deg = 180;
					tmp_long.deg--;
					sprintf(tmp_str, "%02d", tmp_long.deg);
					break;
				case 7:
					if (tmp_long.min == 0)
						tmp_long.min = 60;
					tmp_long.min--;
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					break;
				case 8:
					if (tmp_long.second == 0.00)
						tmp_long.second = 60.00;
					tmp_long.second -= 0.01;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}
			if (joystick_read(Key_DOWN, Long_press)) {
				joystick_init(Key_DOWN, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}

				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_lat.deg < 10)
						tmp_lat.deg += 90;
					tmp_lat.deg -= 10;
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					break;
				case 3:
					if (tmp_lat.min < 10)
						tmp_lat.min += 60;
					tmp_lat.min -= 10;
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					break;
				case 4:
					if (tmp_lat.second < 1.00)
						tmp_lat.second += 60.00;
					tmp_lat.second -= 1.00;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					break;
				case 5:
					if (tmp_lat.direction == 'N')
						tmp_lat.direction = 'S';
					else
						tmp_lat.direction = 'N';
					sprintf(tmp_str, "%c", tmp_lat.direction);
					break;
				case 6:
					if (tmp_long.deg < 10)
						tmp_long.deg += 90;
					tmp_long.deg -= 10;
					sprintf(tmp_str, "%02d", tmp_long.deg);
					break;
				case 7:
					if (tmp_long.min < 10)
						tmp_long.min += 60;
					tmp_long.min -= 10;
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					break;
				case 8:
					if (tmp_long.second < 1.00)
						tmp_long.second += 60.00;
					tmp_long.second -= 1.0;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}

			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					sprintf(tmp_str, "%c", tmp_long.direction);
					index_option = 9;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 3:
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					index_option = 2;

					break;
				case 4:
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					index_option = 3;

					break;
				case 5:
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					index_option = 4;
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_lat.direction);
					index_option = 5;

					break;
				case 7:

					sprintf(tmp_str, "%02d", tmp_long.deg);
					index_option = 6;

					break;
				case 8:
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					index_option = 7;
					break;
				case 9:
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					index_option = 8;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					index_option = 3;
					break;
				case 3:
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					index_option = 4;
					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_lat.direction);
					index_option = 5;
					break;
				case 5:
					sprintf(tmp_str, "%02d", tmp_long.deg);
					index_option = 6;
					break;
				case 6:
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					index_option = 7;
					break;
				case 7:
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					index_option = 8;
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_long.direction);
					index_option = 9;
					break;
				case 9:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					LAT_Value = POS2double(tmp_lat);
					LONG_Value = POS2double(tmp_long);
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					index_option = 3;
					break;
				case 3:
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second);
					index_option = 4;
					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_lat.direction);
					index_option = 5;
					break;
				case 5:
					sprintf(tmp_str, "%02d", tmp_long.deg);
					index_option = 6;
					break;
				case 6:
					sprintf(tmp_str, "%02d\'", tmp_long.min);
					index_option = 7;
					break;
				case 7:
					sprintf(tmp_str, "%05.2f\"", tmp_long.second);
					index_option = 8;
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_long.direction);
					index_option = 9;
					break;
				case 9:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			break;
			/////////////////////////////////////CLOCK_MENU/////////////////////////////////////////////////
		case TIME_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_time.Hours++;
					if (tmp_time.Hours > 23)
						tmp_time.Hours = 0;
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					break;
				case 3:
					tmp_time.Minutes++;
					if (tmp_time.Minutes >= 60)
						tmp_time.Minutes = 0;
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					break;
				case 4:
					tmp_time.Seconds++;
					if (tmp_time.Seconds >= 60)
						tmp_time.Seconds = 0;
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					break;
				case 5:
					tmp_Date.Year++;
					if (tmp_Date.Year > 99)
						tmp_Date.Year = 0;
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					break;
				case 6:
					tmp_Date.Month++;
					if (tmp_Date.Month > 12)
						tmp_Date.Month = 1;
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					break;
				case 7:
					tmp_Date.Date++;
					if (leap(tmp_Date.Year)) {
						if (tmp_Date.Date > gDaysInMonthLeap[tmp_Date.Date])
							tmp_Date.Date = 1;
					} else {
						if (tmp_Date.Date > gDaysInMonth[tmp_Date.Date])
							tmp_Date.Date = 1;
					}
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Long_press)) {
				joystick_init(Key_TOP, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_time.Hours++;
					if (tmp_time.Hours > 23)
						tmp_time.Hours = 0;
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					break;
				case 3:
					tmp_time.Minutes++;
					if (tmp_time.Minutes >= 60)
						tmp_time.Minutes = 0;
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					break;
				case 4:
					tmp_time.Seconds++;
					if (tmp_time.Seconds >= 60)
						tmp_time.Seconds = 0;
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					break;
				case 5:
					tmp_Date.Year++;
					if (tmp_Date.Year > 99)
						tmp_Date.Year = 0;
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					break;
				case 6:
					tmp_Date.Month++;
					if (tmp_Date.Month > 12)
						tmp_Date.Month = 1;
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					break;
				case 7:
					tmp_Date.Date++;
					if (leap(tmp_Date.Year)) {
						if (tmp_Date.Date > gDaysInMonthLeap[tmp_Date.Month])
							tmp_Date.Date = 1;
					} else {
						if (tmp_Date.Date > gDaysInMonth[tmp_Date.Month])
							tmp_Date.Date = 1;
					}
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_time.Hours == 0)
						tmp_time.Hours = 24;
					tmp_time.Hours--;
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					break;
				case 3:
					if (tmp_time.Minutes == 0)
						tmp_time.Minutes = 60;
					tmp_time.Minutes--;
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					break;
				case 4:
					if (tmp_time.Seconds == 0)
						tmp_time.Seconds = 60;
					tmp_time.Seconds--;
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					break;
				case 5:
					if (tmp_Date.Year == 21)
						tmp_Date.Year = 22;
					tmp_Date.Year--;
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					break;
				case 6:
					if (tmp_Date.Month == 1)
						tmp_Date.Month = 13;
					tmp_Date.Month--;
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					break;
				case 7:
					if (tmp_Date.Date == 1) {
						if (leap(tmp_Date.Year)) {
							tmp_Date.Date = gDaysInMonthLeap[tmp_Date.Month]
									+ 1;
						} else {
							tmp_Date.Date = gDaysInMonth[tmp_Date.Month] + 1;
						}
					}
					tmp_Date.Date--;

					sprintf(tmp_str, "%02d", tmp_Date.Date);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_DOWN, Long_press)) {
				joystick_init(Key_DOWN, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_time.Hours == 0)
						tmp_time.Hours = 24;
					tmp_time.Hours--;
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					break;
				case 3:
					if (tmp_time.Minutes == 0)
						tmp_time.Minutes = 60;
					tmp_time.Minutes--;
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					break;
				case 4:
					if (tmp_time.Seconds == 0)
						tmp_time.Seconds = 60;
					tmp_time.Seconds--;
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					break;
				case 5:
					if (tmp_Date.Year == 21)
						tmp_Date.Year = 22;
					tmp_Date.Year--;
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					break;
				case 6:
					if (tmp_Date.Month == 1)
						tmp_Date.Month = 13;
					tmp_Date.Month--;
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					break;
				case 7:
					if (tmp_Date.Date == 1) {
						if (leap(tmp_Date.Year)) {
							tmp_Date.Date = gDaysInMonthLeap[tmp_Date.Month]
									+ 1;
						} else {
							tmp_Date.Date = gDaysInMonth[tmp_Date.Month] + 1;
						}
					}
					tmp_Date.Date--;
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					index_option = 7;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 3:
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					index_option = 2;

					break;
				case 4:
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					index_option = 3;

					break;
				case 5:
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					index_option = 4;
					break;
				case 6:
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					index_option = 5;

					break;
				case 7:
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					index_option = 6;

					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;

					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					sprintf(tmp_str, "%02d", tmp_time.Hours);
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					index_option = 3;
					break;
				case 3:
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					index_option = 4;
					break;
				case 4:
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					index_option = 6;
					break;
				case 6:
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					index_option = 7;

					break;
				case 7:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					HAL_RTC_SetTime(&hrtc, &tmp_time, RTC_FORMAT_BIN);
					HAL_RTC_SetDate(&hrtc, &tmp_Date, RTC_FORMAT_BIN);

					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%02d", tmp_time.Minutes);
					index_option = 3;
					break;
				case 3:
					sprintf(tmp_str, "%02d", tmp_time.Seconds);
					index_option = 4;
					break;
				case 4:
					sprintf(tmp_str, "%04d", tmp_Date.Year + 2000);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%02d", tmp_Date.Month);
					index_option = 6;
					break;
				case 6:
					sprintf(tmp_str, "%02d", tmp_Date.Date);
					index_option = 7;

					break;
				case 7:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////LEDS1_MENU/////////////////////////////////////////////////
		case LEDS1_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value++;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += (double)0.1;
					if (tmp_LED.DAY_BLINK_Value > (double) BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = (double) BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value +=(double) 0.1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value++;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += (double)0.1;
					if (tmp_LED.NIGHT_BLINK_Value > (double) BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = (double) BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += (double)0.1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = (double)ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Long_press)) {
				joystick_init(Key_TOP, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value += 10;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value =
								tmp_LED.DAY_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += (double)1.0;
					if (tmp_LED.DAY_BLINK_Value > (double) BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = (double) BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += (double)0.1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value += 10;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += (double)1.0;
					if (tmp_LED.NIGHT_BLINK_Value > (double) BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = (double) BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += (double)0.1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = (double)ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED)

					{
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value == 0)
						tmp_LED.DAY_BRIGHTNESS_Value = 101;
					tmp_LED.DAY_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value == (double) BLINK_MIN)
							tmp_LED.DAY_BLINK_Value = (double) BLINK_MAX + (double)0.1;

						tmp_LED.DAY_BLINK_Value -= (double)0.1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value == ADD_SUNRISE_S1_MIN)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MAX + (double)0.1;
					tmp_LED.ADD_SUNRISE_Value -= (double)0.1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value == 0)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 101;
					tmp_LED.NIGHT_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value == (double) BLINK_MIN)
							tmp_LED.NIGHT_BLINK_Value = (double) BLINK_MAX
									+ (double)0.1;
						tmp_LED.NIGHT_BLINK_Value -= (double)0.1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value == ADD_SUNSET_S1_MIN)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S1_MAX + (double)0.1;
					tmp_LED.ADD_SUNSET_Value -= (double)0.1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_DOWN, Long_press)) {
				joystick_init(Key_DOWN, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value < 10)
						tmp_LED.DAY_BRIGHTNESS_Value =
								tmp_LED.DAY_BRIGHTNESS_Value + 100;
					tmp_LED.DAY_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value
								< ((double) BLINK_MIN + (double)1.0))
							tmp_LED.DAY_BLINK_Value = (double) BLINK_MAX
									+ tmp_LED.DAY_BLINK_Value;
						tmp_LED.DAY_BLINK_Value -= (double)1.0;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value
							< ((double) ADD_SUNRISE_S1_MIN +(double) 1.0))
						tmp_LED.ADD_SUNRISE_Value =((double) ADD_SUNRISE_S1_MAX-ADD_SUNRISE_S1_MIN)
								+ tmp_LED.ADD_SUNRISE_Value;
					tmp_LED.ADD_SUNRISE_Value -= (double)1.0;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value < 10)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value + 100;
					tmp_LED.NIGHT_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value
								< ((double) BLINK_MIN + (double)1.0))
							tmp_LED.NIGHT_BLINK_Value = (double) BLINK_MAX
									+ tmp_LED.DAY_BLINK_Value;
						tmp_LED.NIGHT_BLINK_Value -=(double) 1.0;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value
							< ((double) ADD_SUNSET_S1_MIN + (double)1.0))
						tmp_LED.ADD_SUNSET_Value = ((double) ADD_SUNSET_S1_MAX-ADD_SUNSET_S1_MIN)
								+ tmp_LED.ADD_SUNSET_Value;
					tmp_LED.ADD_SUNSET_Value -= (double)1.0;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 3:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					index_option = 2;

					break;
				case 4:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 7:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;

					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 8:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					S1_LED_Value.TYPE_Value = tmp_LED.TYPE_Value;
					S1_LED_Value.ADD_SUNRISE_Value = tmp_LED.ADD_SUNRISE_Value;
					S1_LED_Value.ADD_SUNSET_Value = tmp_LED.ADD_SUNSET_Value;
					S1_LED_Value.DAY_BLINK_Value = tmp_LED.DAY_BLINK_Value;
					S1_LED_Value.DAY_BRIGHTNESS_Value = tmp_LED.DAY_BRIGHTNESS_Value;
					S1_LED_Value.NIGHT_BLINK_Value = tmp_LED.NIGHT_BLINK_Value;
					S1_LED_Value.NIGHT_BRIGHTNESS_Value =
							tmp_LED.NIGHT_BRIGHTNESS_Value;
					if(S2_LED_Value.TYPE_Value==WHITE_LED && S1_LED_Value.TYPE_Value==WHITE_LED)
					{
						S2_LED_Value.DAY_BLINK_Value=S1_LED_Value.DAY_BLINK_Value;
						S2_LED_Value.NIGHT_BLINK_Value=S1_LED_Value.NIGHT_BLINK_Value;
					}
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 8:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////LEDIR_MENU/////////////////////////////////////////////////
		case LEDS2_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value++;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if(tmp_LED.TYPE_Value==WHITE_LED)
						tmp_LED.DAY_BLINK_Value+=(double)0.1;
					if (tmp_LED.DAY_BLINK_Value > (double)BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = (double)BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += (double)0.1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value++;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					if(tmp_LED.TYPE_Value==WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value+=(double)0.1;
					if (tmp_LED.NIGHT_BLINK_Value > (double)BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = (double)BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value +=(double) 0.1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = (double)ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Long_press)) {
				joystick_init(Key_TOP, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value += 10;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value =
								tmp_LED.DAY_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if(tmp_LED.TYPE_Value==WHITE_LED)
						tmp_LED.DAY_BLINK_Value+=(double)1.0;
					if (tmp_LED.DAY_BLINK_Value > (double)BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = (double)BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += (double)0.1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value += 10;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 7:
					if(tmp_LED.TYPE_Value==WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value+=(double)1.0;
					if (tmp_LED.NIGHT_BLINK_Value > (double)BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = (double)BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += (double)0.1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value =(double) ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value == 0)
						tmp_LED.DAY_BRIGHTNESS_Value = 101;
					tmp_LED.DAY_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if(tmp_LED.TYPE_Value==WHITE_LED)
					{
						if (tmp_LED.DAY_BLINK_Value == (double)BLINK_MIN)
							tmp_LED.DAY_BLINK_Value = (double)BLINK_MAX+(double)0.1;
						tmp_LED.DAY_BLINK_Value-=(double)0.1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value == ADD_SUNRISE_S2_MIN)
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S2_MAX +(double) 0.1;
					tmp_LED.ADD_SUNRISE_Value -= (double)0.1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value == 0)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 101;
					tmp_LED.NIGHT_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 7:
					if(tmp_LED.TYPE_Value==WHITE_LED)
					{
					if (tmp_LED.NIGHT_BLINK_Value == (double)BLINK_MIN)
						tmp_LED.NIGHT_BLINK_Value = (double)BLINK_MAX+(double)0.1;
					tmp_LED.NIGHT_BLINK_Value-=(double)0.1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value == ADD_SUNSET_S2_MIN)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S2_MAX + (double)0.1;
					tmp_LED.ADD_SUNSET_Value -= (double)0.1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}
			if (joystick_read(Key_DOWN, Long_press)) {
				joystick_init(Key_DOWN, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						sprintf(tmp_str, "IR");
						tmp_LED.TYPE_Value = IR_LED;
						tmp_LED.NIGHT_BLINK_Value = 0.0;
						tmp_LED.DAY_BLINK_Value = 0.0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value = S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value < 10)
						tmp_LED.DAY_BRIGHTNESS_Value =
								tmp_LED.DAY_BRIGHTNESS_Value + 100;
					tmp_LED.DAY_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					if(tmp_LED.TYPE_Value==WHITE_LED)
					{
					if (tmp_LED.DAY_BLINK_Value < ((double)BLINK_MIN+(double)1.0))
						tmp_LED.DAY_BLINK_Value = (double)BLINK_MAX+tmp_LED.DAY_BLINK_Value;
					tmp_LED.DAY_BLINK_Value-=(double)1.0;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value
							< ((double) ADD_SUNRISE_S2_MIN + (double)1.0))
						tmp_LED.ADD_SUNRISE_Value = (double)ADD_SUNRISE_S1_MAX-(double)ADD_SUNRISE_S2_MIN
								+ (double)tmp_LED.ADD_SUNRISE_Value;
					tmp_LED.ADD_SUNRISE_Value -= (double)1.0;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value < 10)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value + 100;
					tmp_LED.NIGHT_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 7:
					if(tmp_LED.TYPE_Value==WHITE_LED)
					{
					if (tmp_LED.NIGHT_BLINK_Value < ((double)BLINK_MIN+(double)1.0))
						tmp_LED.NIGHT_BLINK_Value = (double)BLINK_MAX+tmp_LED.DAY_BLINK_Value;
					tmp_LED.NIGHT_BLINK_Value-=(double)1.0;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value
							< ((double) ADD_SUNSET_S2_MIN + (double)1.0))
						tmp_LED.ADD_SUNSET_Value = (double)ADD_SUNSET_S2_MAX-(double)ADD_SUNRISE_S2_MIN
								+ (double)tmp_LED.ADD_SUNSET_Value;
					tmp_LED.ADD_SUNSET_Value -= (double)1.0;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 3:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					index_option = 2;

					break;
				case 4:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 5:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 7:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				case 8:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);

					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;

					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 8:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					S2_LED_Value.TYPE_Value = tmp_LED.TYPE_Value;
					S2_LED_Value.ADD_SUNRISE_Value = tmp_LED.ADD_SUNRISE_Value;
					S2_LED_Value.ADD_SUNSET_Value = tmp_LED.ADD_SUNSET_Value;
					S2_LED_Value.DAY_BLINK_Value = tmp_LED.DAY_BLINK_Value;
					S2_LED_Value.DAY_BRIGHTNESS_Value = tmp_LED.DAY_BRIGHTNESS_Value;
					S2_LED_Value.NIGHT_BLINK_Value = tmp_LED.NIGHT_BLINK_Value;
					S2_LED_Value.NIGHT_BRIGHTNESS_Value =
							tmp_LED.NIGHT_BRIGHTNESS_Value;
					if(S1_LED_Value.TYPE_Value==WHITE_LED && S2_LED_Value.TYPE_Value==WHITE_LED)
					{
						S1_LED_Value.DAY_BLINK_Value=S2_LED_Value.DAY_BLINK_Value;
						S1_LED_Value.NIGHT_BLINK_Value=S2_LED_Value.NIGHT_BLINK_Value;
					}
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.DAY_BRIGHTNESS_Value, tmp_LED.DAY_BLINK_Value);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNRISE_Value);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2, tmp_LED.NIGHT_BRIGHTNESS_Value, tmp_LED.NIGHT_BLINK_Value);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value);
					index_option = 8;
					break;
				case 8:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			break;
			/////////////////////////////////////RELAY_MENU/////////////////////////////////////////////////
		case RELAY_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_TEC_STATE) {
						sprintf(tmp_str, "DISBALE");
						tmp_TEC_STATE=0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE=1;
					}
					break;
				case 3:
					if(tmp_Relay1.Edge[0]=='U')
					{
						tmp_Relay1.Edge[0]='D';
						tmp_Relay1.active[0]=1;
					}
					else if(tmp_Relay1.Edge[0]=='D')
					{
						tmp_Relay1.Edge[0]='-';
						tmp_Relay1.active[0]=0;
					}
					else
					{
						tmp_Relay1.Edge[0]='U';
						tmp_Relay1.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0]+=0.1;
					if (tmp_Relay1.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
					break;

				case 5:
					if(tmp_Relay1.Edge[1]=='U')
					{
						tmp_Relay1.Edge[1]='D';
						tmp_Relay1.active[1]=1;
					}
					else if(tmp_Relay1.Edge[1]=='D')
					{
						tmp_Relay1.Edge[1]='-';
						tmp_Relay1.active[1]=0;
					}
					else
					{
						tmp_Relay1.Edge[1]='U';
						tmp_Relay1.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1]+=0.1;
					if (tmp_Relay1.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);

					break;
				case 7:

					if(tmp_Relay2.Edge[0]=='U')
					{
						tmp_Relay2.Edge[0]='D';
						tmp_Relay2.active[0]=1;
					}
					else if(tmp_Relay2.Edge[0]=='D')
					{
						tmp_Relay2.Edge[0]='-';
						tmp_Relay2.active[0]=0;
					}
					else
					{
						tmp_Relay2.Edge[0]='U';
						tmp_Relay2.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0]+=0.1;
					if (tmp_Relay2.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
					break;
				case 9:
					if(tmp_Relay2.Edge[1]=='U')
					{
						tmp_Relay2.Edge[1]='D';
						tmp_Relay2.active[1]=1;
					}
					else if(tmp_Relay2.Edge[1]=='D')
					{
						tmp_Relay2.Edge[1]='-';
						tmp_Relay2.active[1]=0;
					}
					else
					{
						tmp_Relay2.Edge[1]='U';
						tmp_Relay2.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1]+=0.1;
					if (tmp_Relay2.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[10] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_TOP, Long_press)) {
				joystick_init(Key_TOP, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_TEC_STATE) {
						sprintf(tmp_str, "DISBALE");
						tmp_TEC_STATE=0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE=1;
					}
					break;
				case 3:
					if(tmp_Relay1.Edge[0]=='U')
					{
						tmp_Relay1.Edge[0]='D';
						tmp_Relay1.active[0]=1;
					}
					else if(tmp_Relay1.Edge[0]=='D')
					{
						tmp_Relay1.Edge[0]='-';
						tmp_Relay1.active[0]=0;
					}
					else
					{
						tmp_Relay1.Edge[0]='U';
						tmp_Relay1.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0]+=10.0;
					if (tmp_Relay1.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
					break;

				case 5:
					if(tmp_Relay1.Edge[1]=='U')
					{
						tmp_Relay1.Edge[1]='D';
						tmp_Relay1.active[1]=1;
					}
					else if(tmp_Relay1.Edge[1]=='D')
					{
						tmp_Relay1.Edge[1]='-';
						tmp_Relay1.active[1]=0;
					}
					else
					{
						tmp_Relay1.Edge[1]='U';
						tmp_Relay1.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1]+=10.0;
					if (tmp_Relay1.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);

					break;
				case 7:

					if(tmp_Relay2.Edge[0]=='U')
					{
						tmp_Relay2.Edge[0]='D';
						tmp_Relay2.active[0]=1;
					}
					else if(tmp_Relay2.Edge[0]=='D')
					{
						tmp_Relay2.Edge[0]='-';
						tmp_Relay2.active[0]=0;
					}
					else
					{
						tmp_Relay2.Edge[0]='U';
						tmp_Relay2.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0]+=10.0;
					if (tmp_Relay2.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
					break;
				case 9:
					if(tmp_Relay2.Edge[1]=='U')
					{
						tmp_Relay2.Edge[1]='D';
						tmp_Relay2.active[1]=1;
					}
					else if(tmp_Relay2.Edge[1]=='D')
					{
						tmp_Relay2.Edge[1]='-';
						tmp_Relay2.active[1]=0;
					}
					else
					{
						tmp_Relay2.Edge[1]='U';
						tmp_Relay2.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1]+=10.0;
					if (tmp_Relay2.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[10] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_TEC_STATE) {
						sprintf(tmp_str, "DISBALE");
						tmp_TEC_STATE=0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE=1;
					}
					break;
				case 3:
					if(tmp_Relay1.Edge[0]=='U')
					{
						tmp_Relay1.Edge[0]='D';
						tmp_Relay1.active[0]=1;
					}
					else if(tmp_Relay1.Edge[0]=='D')
					{
						tmp_Relay1.Edge[0]='-';
						tmp_Relay1.active[0]=0;
					}
					else
					{
						tmp_Relay1.Edge[0]='U';
						tmp_Relay1.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0]-=0.1;
					if (tmp_Relay1.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
					break;

				case 5:
					if(tmp_Relay1.Edge[1]=='U')
					{
						tmp_Relay1.Edge[1]='D';
						tmp_Relay1.active[1]=1;
					}
					else if(tmp_Relay1.Edge[1]=='D')
					{
						tmp_Relay1.Edge[1]='-';
						tmp_Relay1.active[1]=0;
					}
					else
					{
						tmp_Relay1.Edge[1]='U';
						tmp_Relay1.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1]-=0.1;
					if (tmp_Relay1.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);

					break;
				case 7:

					if(tmp_Relay2.Edge[0]=='U')
					{
						tmp_Relay2.Edge[0]='D';
						tmp_Relay2.active[0]=1;
					}
					else if(tmp_Relay2.Edge[0]=='D')
					{
						tmp_Relay2.Edge[0]='-';
						tmp_Relay2.active[0]=0;
					}
					else
					{
						tmp_Relay2.Edge[0]='U';
						tmp_Relay2.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0]-=0.1;
					if (tmp_Relay2.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
					break;
				case 9:
					if(tmp_Relay2.Edge[1]=='U')
					{
						tmp_Relay2.Edge[1]='D';
						tmp_Relay2.active[1]=1;
					}
					else if(tmp_Relay2.Edge[1]=='D')
					{
						tmp_Relay2.Edge[1]='-';
						tmp_Relay2.active[1]=0;
					}
					else
					{
						tmp_Relay2.Edge[1]='U';
						tmp_Relay2.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1]-=0.1;
					if (tmp_Relay2.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[10] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Long_press)) {
				joystick_init(Key_DOWN, Long_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (tmp_TEC_STATE) {
						sprintf(tmp_str, "DISBALE");
						tmp_TEC_STATE=0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE=1;
					}
					break;
				case 3:
					if(tmp_Relay1.Edge[0]=='U')
					{
						tmp_Relay1.Edge[0]='D';
						tmp_Relay1.active[0]=1;
					}
					else if(tmp_Relay1.Edge[0]=='D')
					{
						tmp_Relay1.Edge[0]='-';
						tmp_Relay1.active[0]=0;
					}
					else
					{
						tmp_Relay1.Edge[0]='U';
						tmp_Relay1.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0]-=10.0;
					if (tmp_Relay1.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
					break;

				case 5:
					if(tmp_Relay1.Edge[1]=='U')
					{
						tmp_Relay1.Edge[1]='D';
						tmp_Relay1.active[1]=1;
					}
					else if(tmp_Relay1.Edge[1]=='D')
					{
						tmp_Relay1.Edge[1]='-';
						tmp_Relay1.active[1]=0;
					}
					else
					{
						tmp_Relay1.Edge[1]='U';
						tmp_Relay1.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1]-=10.0;
					if (tmp_Relay1.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);

					break;
				case 7:

					if(tmp_Relay2.Edge[0]=='U')
					{
						tmp_Relay2.Edge[0]='D';
						tmp_Relay2.active[0]=1;
					}
					else if(tmp_Relay2.Edge[0]=='D')
					{
						tmp_Relay2.Edge[0]='-';
						tmp_Relay2.active[0]=0;
					}
					else
					{
						tmp_Relay2.Edge[0]='U';
						tmp_Relay2.active[0]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0]-=10.0;
					if (tmp_Relay2.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
					break;
				case 9:
					if(tmp_Relay2.Edge[1]=='U')
					{
						tmp_Relay2.Edge[1]='D';
						tmp_Relay2.active[1]=1;
					}
					else if(tmp_Relay2.Edge[1]=='D')
					{
						tmp_Relay2.Edge[1]='-';
						tmp_Relay2.active[1]=0;
					}
					else
					{
						tmp_Relay2.Edge[1]='U';
						tmp_Relay2.active[1]=1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2, tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1]-=10.0;
					if (tmp_Relay2.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[10] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();
			}

			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					if(tmp_Relay2.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
						index_option = 10;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option=9;
					}
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 3:
					if (tmp_TEC_STATE )
						sprintf(tmp_str, "ENABLE");
					else
						sprintf(tmp_str, "DISABLE");
					index_option = 2;

					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					index_option = 3;
					break;
				case 5:
					if(tmp_Relay1.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
						index_option = 4;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
						index_option=3;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 7:
					if(tmp_Relay1.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);
						index_option = 6;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option=5;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 9:
					if(tmp_Relay2.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
						index_option = 8;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option=7;
					}
					break;
				case 10:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
					index_option = 9;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;

					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					if (tmp_TEC_STATE )
						sprintf(tmp_str, "ENABLE");
					else
						sprintf(tmp_str, "DISABLE");
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					index_option = 3;
					break;
				case 3:
					if(tmp_Relay1.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
						index_option = 4;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option=5;
					}

					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 5:
					if(tmp_Relay1.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);
						index_option = 6;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option=7;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 7:
					if(tmp_Relay1.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
						index_option = 8;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option=9;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
					index_option = 9;
					break;
				case 9:
					if(tmp_Relay2.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
						index_option = 10;
					}
					else
					{
						text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
						index_option=0;
					}
					break;
				case 10:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					TEC_STATE_Value=tmp_TEC_STATE;
					RELAY1_Value=tmp_Relay1;
					RELAY2_Value=tmp_Relay2;
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					index_option = 3;
					break;
				case 3:
					if(tmp_Relay1.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0]);
						index_option = 4;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option=5;
					}

					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 5:
					if(tmp_Relay1.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1]);
						index_option = 6;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option=7;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 7:
					if(tmp_Relay1.active[0])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0]);
						index_option = 8;
					}
					else
					{
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option=9;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
					index_option = 9;
					break;
				case 9:
					if(tmp_Relay2.active[1])
					{
						sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1]);
						index_option = 10;
					}
					else
					{
						text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
						index_option=0;
					}
					break;
				case 10:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			break;
			/////////////////////////////////////DOOR_MENU/////////////////////////////////////////////////
		case DOOR_MENU:
			break;
			/////////////////////////////////////CHANGEPASS_MENU/////////////////////////////////////////////////
		case CHANGEPASS_MENU:
			break;
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
#if __LWIP__
		MX_LWIP_Process();
#endif
		MX_USB_HOST_Process();
#if !(__DEBUG__)
	HAL_IWDG_Refresh(&hiwdg);
#endif
	}
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		MX_USB_HOST_Process();

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */
	if (htim->Instance == TIM1 && joystick_state() == jostick_initialized) {
		joystick_read(Key_ALL, no_press);
	}
	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
