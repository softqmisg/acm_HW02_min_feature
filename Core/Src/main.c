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
#include "eeprom.h"
#include "eeprom_usage.h"
#include "log_file.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define FORM_DELAY_SHOW	15	//second
#define LOG_DATA_DELAY	10 //second
////////////////send  code to  bootloader////////////////////////////////
#define WRITE_FROM_SD				1// write from SD with never downgrade
#define WRITE_FROM_USB				2//write from USB with never downgrade
#define FORCE_WRITE_FROM_SD			3//write from SD with forced to flash
#define FORCE_WRITE_FROM_USB		4//write from USB with force to flash

///////////////return codes to application from bootloader/////////////////////////////
#define	MEM_NOT_PRESENT				1
#define	MEM_MOUNT_FAIL				2
#define	FILE_OPEN_FAIL				3
#define	FILE_READ_FAIL				4
#define	FLASH_ERASE_FAIL			5
#define	FLASH_WRITE_FAIL			6
#define	FLASH_CHECKVERSION_FAIL		7
#define	FLASH_CHECKSIZE_FAIL		8
#define	FLASH_VERIFY_FAIL			9
#define	FLASH_CHECKSUM_FAIL			10
#define	FLASH_WRITE_OK				11
//////////////////////////////////////////////////////////////////////////////////////////////
uint32_t __attribute__((section(".newsection"))) sharedmem;

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
uint8_t flag_rtc_1s_general = 0;
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	flag_rtc_1s = 1;
	flag_rtc_1s_general = 1;
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
	DISP_FORM6,
	DISP_FORM7
} DISP_state = DISP_IDLE;

#define MENU_ITEMS	10
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
	COPY_MENU,
	UPGRADE_MENU,
	CHANGEPASS_MENU,
	EXIT_MENU
} MENU_state = MAIN_MENU;
char *menu[] = { "SET Position", "SET Time", "SET LED S1", "SET LED S2",
		"SET Relay", "SET Door", "Copy USB", "Upgrade", "SET PASS", "Exit" };
char *menu_upgrade[] = { "Upgrade from SD", "Upgrade from USB",
		"Force upgrade from SD", "Force upgrade from  USB" };
/////////////////////////////////////////////////parameter variable///////////////////////////////////////////////////////////
LED_t S1_LED_Value, S2_LED_Value;
RELAY_t RELAY1_Value, RELAY2_Value;
uint8_t TEC_STATE_Value;
POS_t LAT_Value;
POS_t LONG_Value;
char PASSWORD_Value[5];
uint16_t DOOR_Value;
int8_t UTC_OFF_Value;
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
	bounding_box_t pos_[2];

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
	change_daylightsaving(&sDate, &sTime, 1);

	sprintf(tmp_str, "  %02d:%02d:%02d", sTime.Hours, sTime.Minutes,
			sTime.Seconds);
	draw_text(tmp_str, 2, 1, Tahoma8, 1, 0);
	sprintf(tmp_str, "%4d-%02d-%02d", sDate.Year + 2000, sDate.Month,
			sDate.Date);
	draw_text(tmp_str, 64, 1, Tahoma8, 1, 0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form1(uint8_t clear, int16_t *temperature) {
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
		if (temperature[i] != (int16_t) 0x8fff) {
			sprintf(tmp_str, "T(%d)=%+4.1f", i + 1, temperature[i] / 10.0);
		} else
			sprintf(tmp_str, "T(%d)=---", i + 1);
		pos_[i].x1++;
		text_cell(pos_, i, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form2(uint8_t clear, double *voltage, double *current) {
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

	if (voltage[0] > 0.0)
		sprintf(tmp_str, "V=%3.1fv", voltage[0]);
	else
		sprintf(tmp_str, "V=---");

	if (voltage[0] > 0.0)
		sprintf(tmp_str, "%s     C=%4.2fA", tmp_str, current[0]);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////CH2,12V	//////////////////////////////////////////////////////////
	if (voltage[1] > 0.0)
		sprintf(tmp_str, "V=%3.1fv", voltage[1]);
	else
		sprintf(tmp_str, "V=---");

	if (voltage[1] > 0.0)
		sprintf(tmp_str, "%s     C=%4.2fA", tmp_str, current[1]);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////CH3,3.3V	//////////////////////////////////////////////////////////
	if (voltage[2] > 0.0)
		sprintf(tmp_str, "V=%3.1fv", voltage[2]);
	else
		sprintf(tmp_str, "V=---");

	if (voltage[2] > 0.0)
		sprintf(tmp_str, "%s     C=%4.3fA", tmp_str, current[2]);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////TEC,12V	//////////////////////////////////////////////////////////
	if (voltage[3] > 0.0)
		sprintf(tmp_str, "V=%3.1fv", voltage[3]);
	else
		sprintf(tmp_str, "V=---");

	if (voltage[3] > 0.0)
		sprintf(tmp_str, "%s     C=%4.1fA", tmp_str, current[3]);
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
			S1_LED_Value.DAY_BLINK_Value / 10.0);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%  %4.1fS", S1_LED_Value.NIGHT_BRIGHTNESS_Value,
			S1_LED_Value.NIGHT_BLINK_Value / 10.0);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%   %4.1fS", S2_LED_Value.DAY_BRIGHTNESS_Value,
			S2_LED_Value.DAY_BLINK_Value / 10.0);
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, " %2d%%  %4.1fS", S2_LED_Value.NIGHT_BRIGHTNESS_Value,
			S2_LED_Value.NIGHT_BLINK_Value / 10.0);
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
void create_form4(uint8_t clear, Time_t cur_sunrise, Time_t cur_sunset) {
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

	sprintf(tmp_str, " %d %d\' %05.2f\"%c", LAT_Value.deg, LAT_Value.min,
			LAT_Value.second / 100.0, LAT_Value.direction);
	pos_[0].x1 = 52;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = 1;
	pos_[0].x2 = 50;
	text_cell(pos_, 0, "Latitude:", Tahoma8, LEFT_ALIGN, 1, 1);

	sprintf(tmp_str, " %d %d\' %05.2f\"%c", LONG_Value.deg, LONG_Value.min,
			LONG_Value.second / 100.0, LONG_Value.direction);
	pos_[1].x1 = 52;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = 1;
	pos_[1].x2 = 50;
	text_cell(pos_, 1, "Longitude:", Tahoma8, LEFT_ALIGN, 1, 1);

	sprintf(tmp_str, "%02d:%02d(%+3.1f)", cur_sunrise.hr, cur_sunrise.min,
			S1_LED_Value.ADD_SUNRISE_Value / 10.0);
	pos_[2].x1 = 42;
	text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[2].x1 = 1;
	pos_[2].x2 = 40;
	text_cell(pos_, 2, "sunrise:", Tahoma8, LEFT_ALIGN, 1, 1);

	sprintf(tmp_str, "%02d:%02d(%+3.1f)", cur_sunset.hr, cur_sunset.min,
			S1_LED_Value.ADD_SUNSET_Value / 10.0);
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
	text_cell(pos_, 0, "TEMPH", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 1, "TEMPL", Tahoma8, CENTER_ALIGN, 1, 1);
	for (uint8_t i = 0; i < 2; i++) {
		if (RELAY1_Value.active[i]) {
			sprintf(tmp_str, "%c %+4.1f", RELAY1_Value.Edge[i],
					RELAY1_Value.Temperature[i] / 10.0);
			text_cell(pos_, 2 + i * 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
		} else {
			text_cell(pos_, 2 + i * 2, "- ------", Tahoma8, CENTER_ALIGN, 0, 0);
		}
		if (RELAY2_Value.active[i]) {
			sprintf(tmp_str, "%c %+4.1f", RELAY2_Value.Edge[i],
					RELAY2_Value.Temperature[i] / 10.0);
			text_cell(pos_, 3 + i * 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
		} else {
			text_cell(pos_, 3 + i * 2, "- ------", Tahoma8, CENTER_ALIGN, 0, 0);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form6(uint8_t clear, uint16_t inside_light, uint16_t outside_light) {
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
	if (inside_light != 0xffff)
		sprintf(tmp_str, "%05d", inside_light);
	else
		sprintf(tmp_str, "----");
	pos_[0].x1 = 90;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = 1;
	pos_[0].x2 = 70;
	text_cell(pos_, 0, "Inside Light:", Tahoma8, LEFT_ALIGN, 1, 1);

	if (outside_light != 0xffff)
		sprintf(tmp_str, "%05d", outside_light);
	else
		sprintf(tmp_str, "-----");
	pos_[1].x1 = 90;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = 1;
	pos_[1].x2 = 70;
	text_cell(pos_, 1, "Outside Light:", Tahoma8, LEFT_ALIGN, 1, 1);
	////
	DWORD fre_clust, fre_sect, tot_sect;
	FRESULT res;

//	if ((res = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {

	if (SDFatFS.fs_type == 0 || BSP_SD_IsDetected() == SD_NOT_PRESENT) {
		sprintf(tmp_str, "--/--");
	} else {
		tot_sect = (SDFatFS.n_fatent - 2) * SDFatFS.csize;
		fre_sect = SDFatFS.free_clst * SDFatFS.csize;
		sprintf(tmp_str, "%5lu/%5lu", fre_sect / 2048, tot_sect / 2048);
	}
	pos_[2].x1 = 67;
	text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = 1;
	pos_[2].x2 = 65;
	text_cell(pos_, 2, "Sd Free|MB", Tahoma8, LEFT_ALIGN, 1, 1);
//	f_mount(NULL, "0:", 0);
	//////
//	if ((res = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1)) != FR_OK) {
	if (USBHFatFS.fs_type == 0) {

		sprintf(tmp_str, "--/--");

	} else {
		tot_sect = (USBHFatFS.n_fatent - 2) * USBHFatFS.csize;
		fre_sect = USBHFatFS.free_clst * USBHFatFS.csize;
		sprintf(tmp_str, "%5lu/%5lu", fre_sect / 2048, tot_sect / 2048);
	}
	pos_[3].x1 = 67;
	text_cell(pos_, 3, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[3].x1 = 1;
	pos_[3].x2 = 65;
	text_cell(pos_, 3, "Usb Free|MB", Tahoma8, LEFT_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form7(uint8_t clear, FRESULT rlog1, FRESULT rlog2, FRESULT rlog3,
		FRESULT rlog4, FRESULT rlog5) {
	char tmp_str[40];
	bounding_box_t pos_[8];

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, pos_[0].y2 - pos_[0].y1, 1, 1, 1, pos_);
	text_cell(pos_, 0, "LOG status", Tahoma8, CENTER_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 2, 1, pos_);

	if (rlog1 != FR_OK)
		sprintf(tmp_str, "Temp Err");
	else
		sprintf(tmp_str, "Temp Ok");
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (rlog2 != FR_OK)
		sprintf(tmp_str, "Volt Err");
	else
		sprintf(tmp_str, "Volt Ok");
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (rlog3 != FR_OK)
		sprintf(tmp_str, "Door Err");
	else
		sprintf(tmp_str, "Door Ok");
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (rlog4 != FR_OK)
		sprintf(tmp_str, "Llight Err");
	else
		sprintf(tmp_str, "Light Ok");
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (rlog5 != FR_OK)
		sprintf(tmp_str, "Param Err");
	else
		sprintf(tmp_str, "Param Ok");
	text_cell(pos_, 4, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

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
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 1, 1, 1, pos_);
	for (uint8_t i = 0; i < 4; i++) {
		pos_[0].x1 = i * 32;
		pos_[0].x2 = (i + 1) * 32 - 1;
		text_pos[i + 2] = text_cell(pos_, 0, "*", Tahoma16, CENTER_ALIGN, 0, 0);
	}

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formposition(uint8_t clear, bounding_box_t *text_pos, POS_t tmp_lat,
		POS_t tmp_long, int16_t utc_off) {
	char tmp_str[40];
	bounding_box_t pos_[3];
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
	create_cell(0, pos_[0].y2, 50, 64 - pos_[0].y2, 3, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 50;
	text_cell(pos_, 0, "Latitude:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 50;
	text_cell(pos_, 1, "Longitude:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[2].x1 = 0;
	pos_[2].x2 = 68;
	text_cell(pos_, 2, "UTC Offset:", Tahoma8, LEFT_ALIGN, 1, 1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(50, pos_[0].y1, 128 - 50, 64 - pos_[0].y1, 3, 1, 1, pos_);

	sprintf(tmp_str, "%02d", tmp_lat.deg);
	pos_[0].x1 = 52;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[2].x1=pos_[0].x1;text_pos[2].x2=pos_[0].x2;text_pos[2].y1=pos_[0].y1;text_pos[2].y2=pos_[0].y2;

	sprintf(tmp_str, "%02d\'", tmp_lat.min);
	pos_[0].x1 = pos_[0].x2 + 4;
	pos_[0].x2 = pos_[0].x1 + text_width("55\'", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[3].x1=pos_[0].x1;text_pos[3].x2=pos_[0].x2;text_pos[3].y1=pos_[0].y1;text_pos[3].y2=pos_[0].y2;

	sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
	pos_[0].x1 = pos_[0].x2 + 3;
	pos_[0].x2 = pos_[0].x1 + text_width("55.55\"", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[4].x1=pos_[0].x1;text_pos[4].x2=pos_[0].x2;text_pos[4].y1=pos_[0].y1;text_pos[4].y2=pos_[0].y2;

	sprintf(tmp_str, "%c", tmp_lat.direction);
	pos_[0].x1 = pos_[0].x2 + 2;
	pos_[0].x2 = pos_[0].x1 + text_width("N", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[5].x1=pos_[0].x1;text_pos[5].x2=pos_[0].x2;text_pos[5].y1=pos_[0].y1;text_pos[5].y2=pos_[0].y2;

	sprintf(tmp_str, "%02d", tmp_long.deg);
	pos_[1].x1 = 52;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[6].x1=pos_[1].x1;text_pos[6].x2=pos_[1].x2;text_pos[6].y1=pos_[1].y1;text_pos[6].y2=pos_[1].y2;

	sprintf(tmp_str, "%02d\'", tmp_long.min);
	pos_[1].x1 = pos_[1].x2 + 4;
	pos_[1].x2 = pos_[1].x1 + text_width("55\'", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[7].x1=pos_[1].x1;text_pos[7].x2=pos_[1].x2;text_pos[7].y1=pos_[1].y1;text_pos[7].y2=pos_[1].y2;

	sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
	pos_[1].x1 = pos_[1].x2 + 3;
	pos_[1].x2 = pos_[1].x1 + text_width("55.55\"", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[8].x1=pos_[1].x1;text_pos[8].x2=pos_[1].x2;text_pos[8].y1=pos_[1].y1;text_pos[8].y2=pos_[1].y2;

	sprintf(tmp_str, "%c", tmp_long.direction);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("W", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
//    text_pos[9].x1=pos_[1].x1;text_pos[9].x2=pos_[1].x2;text_pos[9].y1=pos_[1].y1;text_pos[9].y2=pos_[1].y2;

	sprintf(tmp_str, "%+4.1f", utc_off / 10.0);
	pos_[2].x1 = 70;
	pos_[2].x2 = pos_[2].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[10] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formTime(uint8_t clear, bounding_box_t *text_pos,
		RTC_TimeTypeDef tmp_time, RTC_DateTypeDef tmp_date) {
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

	create_cell(40, pos_[0].y1, 128 - 40, 64 - pos_[0].y1, 2, 1, 1, pos_);

	sprintf(tmp_str, "%02d", tmp_time.Hours);
	pos_[0].x1 = 42;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 3;
	text_cell(pos_, 0, ":", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_time.Minutes);
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 3;
	text_cell(pos_, 0, ":", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_time.Seconds);
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = pos_[0].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%04d", tmp_date.Year + 2000);
	pos_[1].x1 = 42;
	pos_[1].x2 = pos_[1].x1 + text_width("5555", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 4;
	text_cell(pos_, 1, "-", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_date.Month);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 4;
	text_cell(pos_, 1, "-", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%02d", tmp_date.Date);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formLEDS1(uint8_t clear, bounding_box_t *text_pos, LED_t *tmp_led) {

	char tmp_str[40];
	bounding_box_t pos_[3];
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

	sprintf(tmp_str, "%4.1f", tmp_led->DAY_BLINK_Value / 10.0);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 6;
	text_cell(pos_, 1, "s", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNRISE_Value / 10.0);
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

	sprintf(tmp_str, "%4.1f", tmp_led->NIGHT_BLINK_Value / 10.0);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 6;
	text_cell(pos_, 2, "s", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNSET_Value / 10.0);
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
	bounding_box_t pos_[3];
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

	sprintf(tmp_str, "%4.1f", tmp_led->DAY_BLINK_Value / 10.0);
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = pos_[1].x2;
	pos_[1].x2 = pos_[1].x1 + 6;
	text_cell(pos_, 1, "s", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNRISE_Value / 10.0);
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

	sprintf(tmp_str, "%4.1f", tmp_led->NIGHT_BLINK_Value / 10.0);
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[2].x1 = pos_[2].x2;
	pos_[2].x2 = pos_[2].x1 + 6;
	text_cell(pos_, 2, "s", Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%+3.1f", tmp_led->ADD_SUNSET_Value / 10.0);
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

void create_formRelay(uint8_t clear, bounding_box_t *text_pos,
		RELAY_t tmp_Relay1, RELAY_t tmp_Relay2, uint8_t tmp_tec) {
	char tmp_str[40];
	bounding_box_t pos_[3];
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
	text_cell(pos_, 1, "TEMPH:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x1 = 0;
	pos_[2].x2 = 39;
	text_cell(pos_, 2, "TEMPL:", Tahoma8, LEFT_ALIGN, 1, 1);
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
	if (tmp_Relay1.active[0])
		sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
	else
		sprintf(tmp_str, "-");

	pos_[1].x1 = 42;
	pos_[1].x2 = pos_[1].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if (tmp_Relay1.active[0])
		sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[0] / 10.0);
	else
		sprintf(tmp_str, "------");
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	if (tmp_Relay1.active[1])
		sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
	else
		sprintf(tmp_str, "-");
	pos_[1].x1 = pos_[1].x2 + 12;
	pos_[1].x2 = pos_[1].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if (tmp_Relay1.active[1])
		sprintf(tmp_str, "%+4.1f", tmp_Relay1.Temperature[1] / 10.0);
	else
		sprintf(tmp_str, "------");
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = pos_[1].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	////////////////////////////////////////
	if (tmp_Relay2.active[0])
		sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
	else
		sprintf(tmp_str, "-");

	pos_[2].x1 = 42;
	pos_[2].x2 = pos_[2].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if (tmp_Relay2.active[0])
		sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[0] / 10.0);
	else
		sprintf(tmp_str, "------");
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	if (tmp_Relay2.active[1])
		sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
	else
		sprintf(tmp_str, "-");
	pos_[2].x1 = pos_[2].x2 + 12;
	pos_[2].x2 = pos_[2].x1 + text_width("D", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	if (tmp_Relay2.active[1])
		sprintf(tmp_str, "%+4.1f", tmp_Relay2.Temperature[1] / 10.0);
	else
		sprintf(tmp_str, "------");
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = pos_[2].x1 + text_width("+55.5", Tahoma8, 1) + 1;
	text_pos[10] = text_cell(pos_, 2, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formDoor(uint8_t clear, bounding_box_t *text_pos, uint16_t tmp_door) {
	char tmp_str[40];
	bounding_box_t pos_[2];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "DOOR SET", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 60, 64 - pos_[0].y2, 2, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 59;
	text_cell(pos_, 0, "Inside Light:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 59;
	text_cell(pos_, 1, "Limit Light:", Tahoma8, LEFT_ALIGN, 1, 1);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(60, pos_[0].y1, 128 - 60, 64 - pos_[0].y1, 2, 1, 1, pos_);

	uint16_t vcnl_als;
	if (vcnl4200_ps(&vcnl_als) == HAL_OK)
		sprintf(tmp_str, "%04d", vcnl_als);
	else
		sprintf(tmp_str, "-----");
	pos_[0].x1 = 90;
	pos_[0].x2 = pos_[0].x1 + text_width("55555", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);

	sprintf(tmp_str, "%05d", tmp_door);
	pos_[1].x1 = 90;
	pos_[1].x2 = pos_[1].x1 + text_width("55555", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formChangepass(uint8_t clear, bounding_box_t *text_pos,
		char *tmp_pass, char *tmp_confirm) {
	char tmp_str[40];
	bounding_box_t pos_[3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "Pass Change", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 40, 64 - pos_[0].y2, 2, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 39;
	text_cell(pos_, 0, "New:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 39;
	text_cell(pos_, 1, "Confirm:", Tahoma8, LEFT_ALIGN, 1, 1);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(40, pos_[0].y1, 128 - 40, 64 - pos_[0].y1, 2, 1, 1, pos_);
	glcd_refresh();
	pos_[0].x1 = 52;
	for (uint8_t i = 0; i < 4; i++) {
		pos_[0].x2 = pos_[0].x1 + text_width("W", Tahoma8, 1) + 1;
		text_pos[i + 2] = text_cell(pos_, 0, "*", Tahoma8, CENTER_ALIGN, 0, 0);
		pos_[0].x1 += 15;
		tmp_pass[i] = '0';
	}
	pos_[1].x1 = 52;
	for (uint8_t i = 0; i < 4; i++) {
		pos_[1].x2 = pos_[1].x1 + text_width("W", Tahoma8, 1) + 1;
		text_pos[i + 6] = text_cell(pos_, 1, "*", Tahoma8, CENTER_ALIGN, 0, 0);
		pos_[1].x1 += 15;
		tmp_confirm[i] = '0';
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formUpgrade(uint8_t clear, bounding_box_t *text_pos) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "Upgrade", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
//	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 1, 1, 1, pos_);//menu_upgrade[op]
	for (uint8_t op = 0; op < 4; op++) {
		pos_[0].y2 = pos_[0].y1 + text_height(menu_upgrade[op], Tahoma8);
		text_pos[op + 2] = text_cell(pos_, 0, menu_upgrade[op], Tahoma8,
				CENTER_ALIGN, 0, 0);
		pos_[0].y1 = pos_[0].y2 + 1;

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Peripherials_DeInit(void) {
	MX_FATFS_DeInit();

	HAL_UART_DeInit(&huart2);
	HAL_UART_MspDeInit(&huart2);

	HAL_UART_DeInit(&huart3);
	HAL_UART_MspDeInit(&huart3);

	HAL_I2C_DeInit(&hi2c3);
	HAL_I2C_MspDeInit(&hi2c3);

	//HAL_RTC_DeInit(&hrtc);
	//HAL_RTC_MspDeInit(&hrtc);

	HAL_SD_DeInit(&hsd);
	HAL_SD_MspDeInit(&hsd);

	f_mount(NULL, "0:", 1);
	f_mount(NULL, "1:", 1);

	MX_USB_DEVICE_DeInit();

	MX_USB_HOST_DeInit();

	__HAL_RCC_DMA2_CLK_DISABLE();

	MX_GPIO_DeInit();

	__HAL_IWDG_RELOAD_COUNTER(&hiwdg);
	HAL_FLASH_Lock();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int app_main(void) {
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
	uint8_t flag_change_form = 0;
	uint8_t counter_change_form = 0;
	uint8_t flag_log_data = 0;
	uint8_t counter_log_data = 0;

	bounding_box_t pos_[10];
	char tmp_str[100], tmp_str1[100], tmp_str2[100];
	uint8_t algorithm_temp_state = 0;

	FRESULT fr;

	uint8_t index_option = 0;
	char tmp_pass[7] = { '*', '*', '*', '*', '*', '*', 0 };
	char tmp_confirmpass[7] = { '*', '*', '*', '*', '*', '*', 0 };
	POS_t tmp_lat, tmp_long;
	int16_t tmp_utcoff;
	double tmp_dlat, tmp_dlong;
	RTC_TimeTypeDef tmp_time;
	RTC_DateTypeDef tmp_Date;
	LED_t tmp_LED;
	RELAY_t tmp_Relay1, tmp_Relay2;
	uint16_t tmp_door;
	uint8_t tmp_TEC_STATE;
	bounding_box_t text_pos[12];
	HAL_StatusTypeDef status;

	FRESULT r_logtemp = FR_OK, r_logvolt = FR_OK, r_loglight = FR_OK,
			r_logdoor = FR_OK, r_logparam = FR_OK;

	uint8_t pre_daylightsaving = 0, cur_daylightsaving = 0;
	Time_t cur_time_t, cur_sunrise, cur_sunset, cur_noon;
	Date_t cur_date_t;
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	int16_t cur_temperature[8];
	double cur_voltage[4], cur_current[4];
	uint16_t cur_insidelight, cur_outsidelight;
	uint8_t cur_doorstate = 1, prev_doorstate = 1;
	int16_t Env_temperature, prev_Env_temperature, Delta_T;
	//////////////////////retarget////////////////
	RetargetInit(&huart3);
	/////////////////////Turn power off & on USB flash///////////////
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_SET); //disable
	HAL_Delay(500);
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_RESET); //enable
	//////////////////////init LCD//////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);

//	//////////////////////////load Logo/////////////////////////////////////////
#if !(__DEBUG__)
	HAL_IWDG_Refresh(&hiwdg);
#endif

	if (BSP_SD_IsDetected() == SD_PRESENT) {
		HAL_Delay(100);
		if (BSP_SD_IsDetected() == SD_PRESENT) {
			if (SDFatFS.fs_type == 0) {
				HAL_SD_Init(&hsd);
				if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK) {
					printf("mounting SD card\n\r");
					bmp_img img;
					if (bmp_img_read(&img, "logo.bmp") == BMP_OK) {
						draw_bmp_h(0, 0, img.img_header.biWidth,
								img.img_header.biHeight, img.img_pixels, 1);
						bmp_img_free(&img);
					} else
						printf("bmp file error\n\r");

					create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
					glcd_refresh();
					HAL_Delay(1000);

				} else {
					SDFatFS.fs_type = 0;
					HAL_SD_DeInit(&hsd);
					printf("mounting SD card ERROR\n\r");
				}
			}
		}
	} else {
		f_mount(NULL, "0:", 0);
		SDFatFS.fs_type = 0;
		HAL_SD_DeInit(&hsd);
	}

//	if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
//		printf("error mount SD\n\r");
//		f_mount(NULL, "0:", 1);
//		SDFatFS.fs_type=0;
//		HAL_SD_DeInit(&hsd);
//	} else {
//		bmp_img img;
//		if (bmp_img_read(&img, "logo.bmp") == BMP_OK)
//		{
//			draw_bmp_h(0, 0, img.img_header.biWidth, img.img_header.biHeight,
//					img.img_pixels, 1);
//			bmp_img_free(&img);
//		}
//		else
//			printf("bmp file error\n\r");
////		f_mount(NULL, "0:", 0);
//
//		create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
//		glcd_refresh();
//		HAL_Delay(1000);
//	}
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
		sprintf(tmp_str1, "veml OK");
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
	//status = ina3221_init(INA3221_LED_BASEADDRESS);

	if ((status = ina3221_init(INA3221_LED_BASEADDRESS)) != HAL_OK) {
		printf("ina3221 LED sensor error\n\r");
		sprintf(tmp_str1, "ina_1 ERR");
		inv = 1;
	} else {
		printf("ina3221 sensor OK\n\r");
		sprintf(tmp_str1, "ina_1 OK");
		inv = 0;
	}
	text_cell(pos_, 3, tmp_str1, Tahoma8, CENTER_ALIGN, inv, inv);

	if ((status = ina3221_init(INA3221_TEC_BASEADDRESS)) != HAL_OK) {
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
	/////////////////////////////eeprom writing defaults & reading//////////////////////////////////////////////////////
	for (uint8_t i = 0; i < NB_OF_VAR; i++)
		VirtAddVarTab[i] = i * 2 + 20;
	HAL_FLASH_Unlock();
	if (EE_Init() == HAL_OK)
		printf("eeprom_OK\n\r");
	else
		printf("eeprom_error\n\r");
	if ((HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x5050)
			|| HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0) {
#if !(__DEBUG__)
		HAL_IWDG_Refresh(&hiwdg);
#endif
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5050);
		glcd_blank();
		create_cell(0, 0, 128, 64, 1, 1, 1, text_pos);
		text_cell(text_pos, 0, "Loading defaults", Tahoma8, CENTER_ALIGN, 0, 0);
		glcd_refresh();
		Write_defaults();

	}
	update_values();

	//////////////////////////////////////////////////////////////////////////////////////////////////
	joystick_init(Key_ALL, Both_press);
	flag_rtc_1s = 1;
	flag_change_form = 1;
	flag_rtc_1s_general = 1;
	flag_log_data = 1;
	MENU_state = MAIN_MENU;
	DISP_state = DISP_IDLE;
	uint8_t flag_log_param = 1;
	uint8_t Delay_Astro_calculation = 0, CalcAstro = 0;
//	pca9632_setbrighnessblinking(LEDS1,80,0);
//	pca9632_setbrighnessblinking(LEDS2,0,1.0);
//
//	/////////////////////////////////////////////////////////
//	uint8_t readdir=1;
//	FILINFO fno;
//	DIR dir;
//	FIL input, output;
//	BYTE  buf[1024];
//	 UINT br, bw;
//	char filesrc[50],filedes[50];
//
//	while(1)
//	{
//		if(USBHFatFS.fs_type!=0  && readdir)
//		{
//			readdir=0;
//			for(uint8_t counter=0;counter<50;counter++)
//			{
//				sprintf(filedes,"1:log/Temperature %d.txt",counter+1);counter++;
//				printf("%s\n\r",filedes);
//				if ((fr = f_open(&output, (const TCHAR*) filedes, FA_WRITE | FA_CREATE_ALWAYS))
//						!= FR_OK) {
//					printf("open error\n\r");
//					break;
//				}
//				for(uint8_t i=0;i<100;i++)
//				{
//					sprintf(buf,"Hello world  in %d\n",i);
//					if(f_write(&output, buf, sizeof(buf), &bw)!=FR_OK)
//					{
//						printf("write in line %d error\n\r",i);
//						break;
//					}
//				}
//				f_close(&output);
//				MX_USB_HOST_Process();
//			}
////			if ((fr = f_opendir(&dir, "0:/log")) == FR_OK) {
////				for(;;)
////				{
////					fr=f_readdir(&dir, &fno);
////					if (fr != FR_OK || fno.fname[0] == 0)
////					{
////						printf("end of 0:/log directory\n\r");
////						break;
////					}
////					if(!(fno.fattrib & AM_DIR)) //it is file so copy
////					{
////						printf("%s\n\r",fno.fname);
////						sprintf(filesrc,"0:/log/%s",fno.fname);
////						sprintf(filedes,"1:/log/%s",fno.fname);
////						if ((fr = f_open(&input, (const TCHAR*) filesrc, FA_READ)) != FR_OK) {
////							readdir=1;
////							break;
////						}
////
////						if ((fr = f_open(&output, (const TCHAR*) filedes, FA_WRITE | FA_CREATE_ALWAYS))
////								!= FR_OK) {
////							f_close(&input);
////							readdir=1;
////							break;
////						}
////						while (!f_eof(&input)) {
//////							if(f_gets(buf, 500, &input)==NULL)
//////							{
//////								f_close(&input);
//////								f_close(&output);
//////								return FR_DISK_ERR;
//////							}
//////
//////							if(f_puts(buf,&output)<0)
//////							{
//////								f_close(&input);
//////								f_close(&output);
//////								return FR_DISK_ERR;
//////							}
////							if(f_read(&input, buf, sizeof(buf), &br)!=FR_OK)
////							{
////								readdir=1;
////								break;
////							}
////							if(br==0)
////							{
////								break;
////							}
////							if(f_write(&output, buf, br, &bw)!=FR_OK)
////							{
////								readdir=1;
////								break;
////							}
////							if(bw<br)
////							{
////								break;
////							}
////							HAL_Delay(10);
////						}
////						f_close(&input);
////						f_close(&output);
////
////		//				f_unlink(filesrc);
////
////					}
////					MX_USB_HOST_Process();
////					HAL_Delay(100);
////				}
////				f_closedir(&dir);
////			}
//		}
//		MX_USB_HOST_Process();
//	}
	///////////////////////////////////////Start Main Loop////////////////////////////////////////////////////////////
	while (1) {
		///////////////////////////////////////Always run process///////////////////////////////////////
		/////////////////////Read time//////////////////////////////////
		if (flag_rtc_1s_general) {
			flag_rtc_1s_general = 0;
			HAL_RTC_GetDate(&hrtc, &cur_Date, RTC_FORMAT_BIN);
			HAL_RTC_GetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
			change_daylightsaving(&cur_Date, &cur_time, 1);
			cur_date_t.day = cur_Date.Date;
			cur_date_t.month = cur_Date.Month;
			cur_date_t.year = cur_Date.Year;
		}
		/////////////////////check door state & LOG//////////////////////////////////
		if (cur_insidelight > DOOR_Value)
			cur_doorstate = 1;
		else
			cur_doorstate = 0;
		if (cur_doorstate != prev_doorstate) {
			if (cur_doorstate)
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,Opened\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			else
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,Closed\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			r_logdoor = Log_file(SDCARD_DRIVE, DOORSTATE_FILE, tmp_str);
			prev_doorstate = cur_doorstate;
		}
		//////////////////////////Saves parameters in logs file in SDCARD,just one  time////
		if (flag_log_param /*&& USBH_MSC_IsReady(&hUsbHostHS)*/) {
			flag_log_param = 0;
			sprintf(tmp_str,
					"%04d-%02d-%02d,%02d:%02d:%02d,POSITION,%02d %02d' %05.2f\" %c,%02d %02d' %05.2f\" %c\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					LAT_Value.deg, LAT_Value.min, LAT_Value.second / 100.0,
					LAT_Value.direction, LONG_Value.deg, LONG_Value.min,
					LONG_Value.second / 100.0, LONG_Value.direction);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			sprintf(tmp_str,
					"%04d-%02d-%02d,%02d:%02d:%02d,LED SET1,%s,%d,%f,%f,%d,%f,%f\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					(S1_LED_Value.TYPE_Value == IR_LED) ? "IR" : "WHITE",
					S1_LED_Value.DAY_BRIGHTNESS_Value,
					S1_LED_Value.DAY_BLINK_Value / 10.0,
					S1_LED_Value.ADD_SUNRISE_Value / 10.0,
					S1_LED_Value.NIGHT_BRIGHTNESS_Value,
					S1_LED_Value.NIGHT_BLINK_Value / 10.0,
					S1_LED_Value.ADD_SUNSET_Value / 10.0);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			sprintf(tmp_str,
					"%04d-%02d-%02d,%02d:%02d:%02d,LED SET2,%s,%d,%f,%f,%d,%f,%f\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					(S2_LED_Value.TYPE_Value == IR_LED) ? "IR" : "WHITE",
					S2_LED_Value.DAY_BRIGHTNESS_Value,
					S2_LED_Value.DAY_BLINK_Value / 10.0,
					S2_LED_Value.ADD_SUNRISE_Value / 10.0,
					S2_LED_Value.NIGHT_BRIGHTNESS_Value,
					S2_LED_Value.NIGHT_BLINK_Value / 10.0,
					S2_LED_Value.ADD_SUNSET_Value / 10.0);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			if (TEC_STATE_Value)
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,TEC ENABLE\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			else
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,TEC DISABLE\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			if (RELAY1_Value.active[0])
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%c%f",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						RELAY1_Value.Edge[0],
						RELAY1_Value.Temperature[0] / 10.0);
			else
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,--",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			if (RELAY1_Value.active[1])
				sprintf(tmp_str, "%s,%c%f\n", tmp_str, RELAY1_Value.Edge[1],
						RELAY1_Value.Temperature[1] / 10.0);
			else
				sprintf(tmp_str, "%s,--\n", tmp_str);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			if (RELAY2_Value.active[0])
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,%c%f",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						RELAY2_Value.Edge[0],
						RELAY2_Value.Temperature[0] / 10.0);
			else
				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,--",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			if (RELAY2_Value.active[1])
				sprintf(tmp_str, "%s,%c%f\n", tmp_str, RELAY2_Value.Edge[1],
						RELAY2_Value.Temperature[1] / 10.0);
			else
				sprintf(tmp_str, "%s,--\n", tmp_str);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

			sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,Door,%d\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					DOOR_Value);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str);

		}

		/////////////////////////////////////State Machine///////////////////////////////////////////////////////////
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
				counter_log_data++;
				if (counter_log_data >= LOG_DATA_DELAY) {
					counter_log_data = 0;
					flag_log_data = 1;
				}
				if (Delay_Astro_calculation) {
					Delay_Astro_calculation--;
					if (Delay_Astro_calculation == 0)
						CalcAstro = 1;
				}
				/////////////////////read sensors//////////////////////////
				for (uint8_t i = 0; i < 8; i++)
					if (tmp275_readTemperature(i, &cur_temperature[i])
							!= HAL_OK) {
						cur_temperature[i] = (int16_t) 0x8fff;
					}
				if (ina3221_readdouble((uint8_t) VOLTAGE_7V, &cur_voltage[0])
						!= HAL_OK) {
					cur_voltage[0] = -1.0;
				}
				if (ina3221_readdouble((uint8_t) CURRENT_7V, &cur_current[0])
						!= HAL_OK) {
					cur_current[0] = -1.0;
				}
				if (ina3221_readdouble((uint8_t) VOLTAGE_12V, &cur_voltage[1])
						!= HAL_OK) {
					cur_voltage[1] = -1.0;

				}
				if (ina3221_readdouble((uint8_t) CURRENT_12V, &cur_current[1])
						!= HAL_OK) {
					cur_current[1] = -1.0;

				}
				if (ina3221_readdouble((uint8_t) VOLTAGE_3V3, &cur_voltage[2])
						!= HAL_OK) {
					cur_voltage[2] = -1.0;

				}
				if (ina3221_readdouble((uint8_t) CURRENT_3V3, &cur_current[2])
						!= HAL_OK) {
					cur_current[2] = -1.0;

				}
				if (ina3221_readdouble((uint8_t) VOLTAGE_TEC, &cur_voltage[3])
						!= HAL_OK) {
					cur_voltage[3] = -1.0;

				}
				if (ina3221_readdouble((uint8_t) CURRENT_TEC, &cur_current[3])
						!= HAL_OK) {
					cur_current[3] = -1.0;

				}
				if (vcnl4200_ps(&cur_insidelight) != HAL_OK) {
					cur_insidelight = 0xffff;
				}
				if (veml6030_als(&cur_outsidelight) != HAL_OK) {
					cur_outsidelight = 0xffff;
				}

				////////////////////////////LED control////////////////////////////////////
				if ((cur_time.Hours == 0 && cur_time.Minutes == 0
						&& cur_time.Seconds == 0) || (CalcAstro)) {
					CalcAstro = 0;
					tmp_dlat = POS2double(LAT_Value);
					tmp_dlong = POS2double(LONG_Value);
					Astro_sunRiseSet(tmp_dlat, tmp_dlong, UTC_OFF_Value / 10.0,
							cur_date_t, &cur_sunrise, &cur_noon, &cur_sunset,
							1);
				}
				if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
						S1_LED_Value.ADD_SUNRISE_Value / 10.0,
						S1_LED_Value.ADD_SUNSET_Value / 10.0) == ASTRO_DAY) {
					pca9632_setbrighnessblinking(LEDS1,
							S1_LED_Value.DAY_BRIGHTNESS_Value,
							S1_LED_Value.DAY_BLINK_Value / 10.0);
				} else {
					pca9632_setbrighnessblinking(LEDS1,
							S1_LED_Value.NIGHT_BRIGHTNESS_Value,
							S1_LED_Value.NIGHT_BLINK_Value / 10.0);
				}
				if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
						S2_LED_Value.ADD_SUNRISE_Value / 10.0,
						S2_LED_Value.ADD_SUNSET_Value / 10.0) == ASTRO_DAY) {
					pca9632_setbrighnessblinking(LEDS2,
							S2_LED_Value.DAY_BRIGHTNESS_Value,
							S2_LED_Value.DAY_BLINK_Value / 10.0);
				} else {
					pca9632_setbrighnessblinking(LEDS2,
							S2_LED_Value.NIGHT_BRIGHTNESS_Value,
							S2_LED_Value.NIGHT_BLINK_Value / 10.0);

				}
				////////////////////////////DISP Refresh//////////////////////////////////
				switch (DISP_state) {
				case DISP_IDLE:
					break;
				case DISP_FORM1:
					create_form1(0, cur_temperature);
					break;
				case DISP_FORM2:
					create_form2(0, cur_voltage, cur_current);
					break;
				case DISP_FORM3:
					create_form3(0);
					break;
				case DISP_FORM4:
					create_form4(0, cur_sunrise, cur_sunset);
					break;
				case DISP_FORM5:
					create_form5(0);
					break;
				case DISP_FORM6:
					create_form6(0, cur_insidelight, cur_outsidelight);
					break;
				case DISP_FORM7:
					create_form7(0, r_logtemp, r_logvolt, r_logdoor, r_loglight,
							r_logparam);
					break;
				}
			}
			/////////////////////Read & Log Sensors//////////////////////////////////
			if (flag_log_data) {
				flag_log_data = 0;
				counter_log_data = 0;
//				/////////////////////read sensors//////////////////////////
//				for (uint8_t i = 0; i < 8; i++)
//					if (tmp275_readTemperature(i, &cur_temperature[i])
//							!= HAL_OK) {
//						cur_temperature[i] = (int16_t) 0x8fff;
//					}

//				if (ina3221_readdouble((uint8_t) VOLTAGE_7V,
//						&cur_voltage[0]) != HAL_OK) {
//					cur_voltage[0] = -1.0;
//				}
//				if (ina3221_readdouble((uint8_t) CURRENT_7V,
//						&cur_current[0]) != HAL_OK) {
//					cur_current[0] = -1.0;
//				}
//				if (ina3221_readdouble((uint8_t) VOLTAGE_12V,
//						&cur_voltage[1]) != HAL_OK) {
//					cur_voltage[1] = -1.0;
//
//				}
//				if (ina3221_readdouble((uint8_t) CURRENT_12V,
//						&cur_current[1]) != HAL_OK) {
//					cur_current[1] = -1.0;
//
//				}
//				if (ina3221_readdouble((uint8_t) VOLTAGE_3V3,
//						&cur_voltage[2]) != HAL_OK) {
//					cur_voltage[2] = -1.0;
//
//				}
//				if (ina3221_readdouble((uint8_t) CURRENT_3V3,
//						&cur_current[2]) != HAL_OK) {
//					cur_current[2] = -1.0;
//
//				}
//				if (ina3221_readdouble((uint8_t) VOLTAGE_TEC,
//						&cur_voltage[3]) != HAL_OK) {
//					cur_voltage[3] = -1.0;
//
//				}
//				if (ina3221_readdouble((uint8_t) CURRENT_TEC,
//						&cur_current[3]) != HAL_OK) {
//					cur_current[3] = -1.0;
//
//				}
//				if (vcnl4200_ps(&cur_insidelight) != HAL_OK) {
//					cur_insidelight = 0xffff;
//				}
//				if (veml6030_als(&cur_outsidelight) != HAL_OK) {
//					cur_outsidelight = 0xffff;
//				}
				/////////////////////log sensors//////////////////////////

				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
				for (uint8_t i = 0; i < 8; i++) {
					if (cur_temperature[i] == (int16_t) 0x8fff) {
						sprintf(tmp_str, "%s-,", tmp_str);
					} else {
						sprintf(tmp_str, "%s%f,", tmp_str,
								cur_temperature[i] / 10.0);
					}
				}
				sprintf(tmp_str, "%s\n", tmp_str);

				r_logtemp = Log_file(SDCARD_DRIVE, TEMPERATURE_FILE, tmp_str);

				sprintf(tmp_str,
						"%04d-%02d-%02d,%02d:%02d:%02d,%f,%f,%f,%f,%f,%f,%f,%f\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						cur_voltage[0], cur_current[0], cur_voltage[1],
						cur_current[1], cur_voltage[2], cur_current[2],
						cur_voltage[3], cur_current[3]);
				r_logvolt = Log_file(SDCARD_DRIVE, VOLTAMPERE_FILE, tmp_str);

				sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,%d,%d\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						cur_insidelight, cur_outsidelight);

				r_loglight = Log_file(SDCARD_DRIVE, LIGHT_FILE, tmp_str);
			}

			///////////////////////////change display form////////////////////////////////////////////////
			if (flag_change_form) {
				flag_change_form = 0;
				counter_change_form = 0;
				switch (DISP_state) {
				case DISP_IDLE:
					tmp_dlat = POS2double(LAT_Value);
					tmp_dlong = POS2double(LONG_Value);
					Astro_sunRiseSet(tmp_dlat, tmp_dlong, UTC_OFF_Value / 10.0,
							cur_date_t, &cur_sunrise, &cur_noon, &cur_sunset,
							1);
					create_form1(1, cur_temperature);
					DISP_state = DISP_FORM1;
					break;
				case DISP_FORM1:
					create_form2(1, cur_voltage, cur_current);
					DISP_state = DISP_FORM2;
					break;
				case DISP_FORM2:
					create_form3(1);
					DISP_state = DISP_FORM3;
					break;
				case DISP_FORM3:
					create_form4(1, cur_sunrise, cur_sunset);
					DISP_state = DISP_FORM4;
					break;
				case DISP_FORM4:
					create_form5(1);
					DISP_state = DISP_FORM5;
					break;
				case DISP_FORM5:
					create_form6(1, cur_insidelight, cur_outsidelight);
					DISP_state = DISP_FORM6;
					break;
				case DISP_FORM6:
					create_form7(1, r_logtemp, r_logvolt, r_logdoor, r_loglight,
							r_logparam);
					DISP_state = DISP_FORM7;
					break;
				case DISP_FORM7:
					create_form1(1, cur_temperature);
					DISP_state = DISP_FORM1;
					break;
				}
			}
			//////////////////////////////////////////////////////////////////////////////
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)
					|| joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				joystick_init(Key_RIGHT, Short_press);
				flag_change_form = 1;
				counter_change_form = 0;
			}
			if (joystick_read(Key_DOWN, Short_press)
					|| joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				joystick_init(Key_LEFT, Short_press);
				flag_change_form = 1;
				counter_change_form = 0;
				switch (DISP_state) {
				case DISP_IDLE:
					break;
				case DISP_FORM1:
					DISP_state = DISP_FORM6;
					break;
				case DISP_FORM2:
					DISP_state = DISP_FORM7;
					break;
				case DISP_FORM3:
					DISP_state = DISP_FORM1;
					break;
				case DISP_FORM4:
					DISP_state = DISP_FORM2;
					break;
				case DISP_FORM5:
					DISP_state = DISP_FORM3;
					break;
				case DISP_FORM6:
					DISP_state = DISP_FORM4;
					break;
				case DISP_FORM7:
					DISP_state = DISP_FORM5;
					break;

				}
			}
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
				sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
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
					if (tmp_pass[index_option - 2] > '9'
							&& tmp_pass[index_option - 2] < 'A')
						tmp_pass[index_option - 2] = 'A';
					else if (tmp_pass[index_option - 2] > 'Z'
							&& tmp_pass[index_option - 2] < 'a')
						tmp_pass[index_option - 2] = 'a';
					if (tmp_pass[index_option - 2] > 'z')
						tmp_pass[index_option - 2] = '0';
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					draw_fill(text_pos[index_option].x1 + 1,
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
					else if (tmp_pass[index_option - 2] < 'a'
							&& tmp_pass[index_option - 2] > 'Z')
						tmp_pass[index_option - 2] = 'Z';
					else if (tmp_pass[index_option - 2] < 'A'
							&& tmp_pass[index_option - 2] > '9')
						tmp_pass[index_option - 2] = '9';
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);

					draw_fill(text_pos[index_option].x1 + 1,
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
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					index_option = 2;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 5:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				default:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();

			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					index_option = 5;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					index_option = 1;
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					break;
				default:
					index_option--;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				}
				if (index_option > 1) {
					text_cell(text_pos, index_option, tmp_str, Tahoma16,
							CENTER_ALIGN, 1, 0);
				}

				glcd_refresh();

			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
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
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				default:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				}

				if (index_option > 1) {
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
					tmp_lat = LAT_Value;
					tmp_long = LONG_Value;
					tmp_utcoff = UTC_OFF_Value;
					create_formposition(1, text_pos, tmp_lat, tmp_long,
							tmp_utcoff);
					index_option = 2;

					sprintf(tmp_str, "%02d", tmp_lat.deg);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = POSITION_MENU;
					break;
				case TIME_MENU:
					HAL_RTC_GetTime(&hrtc, &tmp_time, RTC_FORMAT_BIN);
					HAL_RTC_GetDate(&hrtc, &tmp_Date, RTC_FORMAT_BIN);
					change_daylightsaving(&tmp_Date, &tmp_time, 1);

					create_formTime(1, text_pos, tmp_time, tmp_Date);
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

					if (tmp_LED.TYPE_Value == WHITE_LED)
						sprintf(tmp_str, "WHITE");
					else
						sprintf(tmp_str, "IR");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = LEDS2_MENU;
					break;
				case RELAY_MENU:
					tmp_Relay1 = RELAY1_Value;
					tmp_Relay2 = RELAY2_Value;
					tmp_TEC_STATE = TEC_STATE_Value;
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					index_option = 2;

					if (tmp_TEC_STATE == 1)
						sprintf(tmp_str, "ENABLE");
					else
						sprintf(tmp_str, "DISABLE");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = RELAY_MENU;
					break;
				case DOOR_MENU:
					tmp_door = DOOR_Value;
					create_formDoor(1, text_pos, tmp_door);
					index_option = 3;
					sprintf(tmp_str, "%05d", tmp_door);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = DOOR_MENU;
					break;
				case CHANGEPASS_MENU:
					sprintf(tmp_pass, "****");
					sprintf(tmp_confirmpass, "****");
					create_formChangepass(1, text_pos, tmp_pass,
							tmp_confirmpass);
					index_option = 2;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = CHANGEPASS_MENU;
					break;
				case COPY_MENU:
					MENU_state = COPY_MENU;
					break;
				case UPGRADE_MENU:
					create_formUpgrade(1, text_pos);
					index_option = 2;
					sprintf(tmp_str, "%s", menu_upgrade[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					glcd_refresh();
					MENU_state = UPGRADE_MENU;
					break;
				case EXIT_MENU:
					MENU_state = EXIT_MENU;
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
					tmp_lat.second += 1;
					if (tmp_lat.second >= 6000)
						tmp_lat.second = 0;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					tmp_long.second += 1;
					if (tmp_long.second >= 6000)
						tmp_long.second = 0;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				case 10:
					tmp_utcoff += 1;
					if (tmp_utcoff > 120)
						tmp_utcoff = -120;
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
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
					tmp_lat.second += 100;
					if (tmp_lat.second >= 6000)
						tmp_lat.second = 0;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					tmp_long.second += 100;
					if (tmp_long.second >= 6000)
						tmp_long.second = 0;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				case 10:
					tmp_utcoff += 10;
					if (tmp_utcoff > 120)
						tmp_utcoff = -120;
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
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
					if (tmp_lat.second == 0)
						tmp_lat.second = 6000;
					tmp_lat.second -= 1;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					if (tmp_long.second == 0)
						tmp_long.second = 6000;
					tmp_long.second -= 1;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				case 10:
					tmp_utcoff -= 1;
					if (tmp_utcoff < -120)
						tmp_utcoff = 120;
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
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
					if (tmp_lat.second < 100)
						tmp_lat.second += 6000;
					tmp_lat.second -= 100;
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					if (tmp_long.second < 100)
						tmp_long.second += 6000;
					tmp_long.second -= 100;
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					break;
				case 9:
					if (tmp_long.direction == 'W')
						tmp_long.direction = 'E';
					else
						tmp_long.direction = 'W';
					sprintf(tmp_str, "%c", tmp_long.direction);
					break;
				case 10:
					tmp_utcoff -= 10;
					if (tmp_utcoff < -120)
						tmp_utcoff = 120;
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
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
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
					index_option = 10;

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
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					index_option = 8;
					break;
				case 10:
					sprintf(tmp_str, "%c", tmp_long.direction);
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
					sprintf(tmp_str, "%02d", tmp_lat.deg);
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%02d\'", tmp_lat.min);
					index_option = 3;
					break;
				case 3:
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					index_option = 8;
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_long.direction);
					index_option = 9;
					break;
				case 9:
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
					index_option = 10;
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
					LAT_Value = tmp_lat;
					LONG_Value = tmp_long;
					UTC_OFF_Value = tmp_utcoff;

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_deg], LAT_Value.deg);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_min], LAT_Value.min);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_second],
							LAT_Value.second);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_direction],
							LAT_Value.direction);

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_deg],
							LONG_Value.deg);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_min],
							LONG_Value.min);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_second],
							LONG_Value.second);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_direction],
							LONG_Value.direction);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_UTC_OFF], UTC_OFF_Value);
					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,POSITION,%02d %02d' %05.2f\" %c,%02d %02d' %05.2f\" %c\n",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							LAT_Value.deg, LAT_Value.min,
							LAT_Value.second / 100.0, LAT_Value.direction,
							LONG_Value.deg, LONG_Value.min,
							LONG_Value.second / 100.0, LONG_Value.direction);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);
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
					sprintf(tmp_str, "%05.2f\"", tmp_lat.second / 100.0);
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
					sprintf(tmp_str, "%05.2f\"", tmp_long.second / 100.0);
					index_option = 8;
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_long.direction);
					index_option = 9;
					break;
				case 9:
					sprintf(tmp_str, "%+4.1f", tmp_utcoff / 10.0);
					index_option = 10;
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
					tmp_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
					tmp_time.StoreOperation = RTC_STOREOPERATION_RESET;
					change_daylightsaving(&tmp_Date, &tmp_time, 0);

					HAL_RTC_SetDate(&hrtc, &tmp_Date, RTC_FORMAT_BIN);
					HAL_Delay(100);

					HAL_RTC_SetTime(&hrtc, &tmp_time, RTC_FORMAT_BIN);
					HAL_Delay(100);
					Delay_Astro_calculation = 3;
					flag_rtc_1s_general = 1;
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
			joystick_init(Key_TOP | Key_DOWN | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value++;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += 1;
					if (tmp_LED.DAY_BLINK_Value > BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += 1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value++;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += 1;
					if (tmp_LED.NIGHT_BLINK_Value > BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += 1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S1_LED_Value.NIGHT_BLINK_Value;
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
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += 10;
					if (tmp_LED.DAY_BLINK_Value > BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += 1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value += 10;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += 10;
					if (tmp_LED.NIGHT_BLINK_Value > BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += 1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S1_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S1_LED_Value.DAY_BLINK_Value;
						create_formLEDS1(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value == 0)
						tmp_LED.DAY_BRIGHTNESS_Value = 101;
					tmp_LED.DAY_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value == BLINK_MIN)
							tmp_LED.DAY_BLINK_Value = BLINK_MAX + 1;

						tmp_LED.DAY_BLINK_Value -= 1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value == ADD_SUNRISE_S1_MIN)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MAX + 1;
					tmp_LED.ADD_SUNRISE_Value -= 1;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value == 0)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 101;
					tmp_LED.NIGHT_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value == BLINK_MIN)
							tmp_LED.NIGHT_BLINK_Value = BLINK_MAX + 1;
						tmp_LED.NIGHT_BLINK_Value -= 1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value == ADD_SUNSET_S1_MIN)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S1_MAX + 1;
					tmp_LED.ADD_SUNSET_Value -= 1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS1(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S1_LED_Value.NIGHT_BLINK_Value;
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
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value < (BLINK_MIN + 10))
							tmp_LED.DAY_BLINK_Value = BLINK_MAX
									+ tmp_LED.DAY_BLINK_Value;
						tmp_LED.DAY_BLINK_Value -= 10;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value < ( ADD_SUNRISE_S1_MIN + 10))
						tmp_LED.ADD_SUNRISE_Value = (ADD_SUNRISE_S1_MAX
								- ADD_SUNRISE_S1_MIN)
								+ tmp_LED.ADD_SUNRISE_Value;
					tmp_LED.ADD_SUNRISE_Value -= 10;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value < 10)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value + 100;
					tmp_LED.NIGHT_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value < ( BLINK_MIN + 10))
							tmp_LED.NIGHT_BLINK_Value = BLINK_MAX
									+ tmp_LED.NIGHT_BLINK_Value;
						tmp_LED.NIGHT_BLINK_Value -= 10;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value < ( ADD_SUNSET_S1_MIN + 10))
						tmp_LED.ADD_SUNSET_Value = ( ADD_SUNSET_S1_MAX
								- ADD_SUNSET_S1_MIN) + tmp_LED.ADD_SUNSET_Value;
					tmp_LED.ADD_SUNSET_Value -= 10;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 7:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
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
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					S1_LED_Value.DAY_BRIGHTNESS_Value =
							tmp_LED.DAY_BRIGHTNESS_Value;
					S1_LED_Value.NIGHT_BLINK_Value = tmp_LED.NIGHT_BLINK_Value;
					S1_LED_Value.NIGHT_BRIGHTNESS_Value =
							tmp_LED.NIGHT_BRIGHTNESS_Value;
					if (S2_LED_Value.TYPE_Value == WHITE_LED
							&& S1_LED_Value.TYPE_Value == WHITE_LED) {
						S2_LED_Value.DAY_BLINK_Value =
								S1_LED_Value.DAY_BLINK_Value;
						S2_LED_Value.NIGHT_BLINK_Value =
								S1_LED_Value.NIGHT_BLINK_Value;
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_DAY_BLINK],
								S2_LED_Value.DAY_BLINK_Value);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BLINK],
								S2_LED_Value.NIGHT_BLINK_Value);
					}

					S2_LED_Value.ADD_SUNRISE_Value =
							S1_LED_Value.ADD_SUNRISE_Value;
					S2_LED_Value.ADD_SUNSET_Value =
							S1_LED_Value.ADD_SUNSET_Value;
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_TYPE],
							S1_LED_Value.TYPE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_DAY_BRIGHTNESS],
							S1_LED_Value.DAY_BRIGHTNESS_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_DAY_BLINK],
							S1_LED_Value.DAY_BLINK_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BRIGHTNESS],
							S1_LED_Value.NIGHT_BRIGHTNESS_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BLINK],
							S1_LED_Value.NIGHT_BLINK_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNRISE],
							S1_LED_Value.ADD_SUNRISE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNSET],
							S1_LED_Value.ADD_SUNSET_Value);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,LED SET1,%s,%d,%f,%f,%d,%f,%f\n",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							(S1_LED_Value.TYPE_Value == IR_LED) ?
									"IR" : "WHITE",
							S1_LED_Value.DAY_BRIGHTNESS_Value,
							S1_LED_Value.DAY_BLINK_Value / 10.0,
							S1_LED_Value.ADD_SUNRISE_Value / 10.0,
							S1_LED_Value.NIGHT_BRIGHTNESS_Value,
							S1_LED_Value.NIGHT_BLINK_Value / 10.0,
							S1_LED_Value.ADD_SUNSET_Value / 10.0);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					////////////////////////////LED control////////////////////////////////////
					if (cur_time.Hours == 0 && cur_time.Minutes == 0
							&& cur_time.Seconds == 0) {
						tmp_dlat = POS2double(LAT_Value);
						tmp_dlong = POS2double(LONG_Value);
						Astro_sunRiseSet(tmp_dlat, tmp_dlong,
								UTC_OFF_Value / 10.0, cur_date_t, &cur_sunrise,
								&cur_noon, &cur_sunset, 1);
					}
					if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
							S1_LED_Value.ADD_SUNRISE_Value / 10.0,
							S1_LED_Value.ADD_SUNSET_Value / 10.0)
							== ASTRO_DAY) {
						pca9632_setbrighnessblinking(LEDS1,
								S1_LED_Value.DAY_BRIGHTNESS_Value,
								S1_LED_Value.DAY_BLINK_Value / 10.0);
					} else {
						pca9632_setbrighnessblinking(LEDS1,
								S1_LED_Value.NIGHT_BRIGHTNESS_Value,
								S1_LED_Value.NIGHT_BLINK_Value / 10.0);
					}
					if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
							S2_LED_Value.ADD_SUNRISE_Value / 10.0,
							S2_LED_Value.ADD_SUNSET_Value / 10.0)
							== ASTRO_DAY) {
						pca9632_setbrighnessblinking(LEDS2,
								S2_LED_Value.DAY_BRIGHTNESS_Value,
								S2_LED_Value.DAY_BLINK_Value / 10.0);
					} else {
						pca9632_setbrighnessblinking(LEDS2,
								S2_LED_Value.NIGHT_BRIGHTNESS_Value,
								S2_LED_Value.NIGHT_BLINK_Value / 10.0);

					}
					////////////////////////////////////////////////////////////////////////
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS1,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
			joystick_init(Key_TOP | Key_DOWN | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					tmp_LED.DAY_BRIGHTNESS_Value++;
					if (tmp_LED.DAY_BRIGHTNESS_Value > 100)
						tmp_LED.DAY_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += 1;
					if (tmp_LED.DAY_BLINK_Value > BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += 1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value++;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 0;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += 1;
					if (tmp_LED.NIGHT_BLINK_Value > BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += 1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNRISE_S2_MAX)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S2_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S2_LED_Value.NIGHT_BLINK_Value;
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
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.DAY_BLINK_Value += 10;
					if (tmp_LED.DAY_BLINK_Value > BLINK_MAX)
						tmp_LED.DAY_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					tmp_LED.ADD_SUNRISE_Value += 1;
					if (tmp_LED.ADD_SUNRISE_Value > ADD_SUNRISE_S1_MAX)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MIN;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					tmp_LED.NIGHT_BRIGHTNESS_Value += 10;
					if (tmp_LED.NIGHT_BRIGHTNESS_Value > 100)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value - 100;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED)
						tmp_LED.NIGHT_BLINK_Value += 10;
					if (tmp_LED.NIGHT_BLINK_Value > BLINK_MAX)
						tmp_LED.NIGHT_BLINK_Value = BLINK_MIN;
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					tmp_LED.ADD_SUNSET_Value += 1;
					if (tmp_LED.ADD_SUNSET_Value > ADD_SUNSET_S1_MAX)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S1_MIN;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S2_LED_Value.NIGHT_BLINK_Value;
						tmp_LED.DAY_BLINK_Value = S2_LED_Value.DAY_BLINK_Value;
						create_formLEDS2(1, text_pos, &tmp_LED);
					}
					break;
				case 3:
					if (tmp_LED.DAY_BRIGHTNESS_Value == 0)
						tmp_LED.DAY_BRIGHTNESS_Value = 101;
					tmp_LED.DAY_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value == BLINK_MIN)
							tmp_LED.DAY_BLINK_Value = BLINK_MAX + 1;
						tmp_LED.DAY_BLINK_Value -= 1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value == ADD_SUNRISE_S2_MIN)
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S2_MAX + 1;
					tmp_LED.ADD_SUNRISE_Value -= 1;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value == 0)
						tmp_LED.NIGHT_BRIGHTNESS_Value = 101;
					tmp_LED.NIGHT_BRIGHTNESS_Value--;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value == BLINK_MIN)
							tmp_LED.NIGHT_BLINK_Value = BLINK_MAX + 1;
						tmp_LED.NIGHT_BLINK_Value -= 1;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value == ADD_SUNSET_S2_MIN)
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S2_MAX + 1;
					tmp_LED.ADD_SUNSET_Value -= 1;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_LED.NIGHT_BLINK_Value = 0;
						tmp_LED.DAY_BLINK_Value = 0;
						create_formLEDS2(1, text_pos, &tmp_LED);
					} else {
						sprintf(tmp_str, "WHITE");
						tmp_LED.TYPE_Value = WHITE_LED;
						tmp_LED.NIGHT_BLINK_Value =
								S2_LED_Value.NIGHT_BLINK_Value;
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
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.DAY_BLINK_Value < (BLINK_MIN + 10))
							tmp_LED.DAY_BLINK_Value = BLINK_MAX
									+ tmp_LED.DAY_BLINK_Value;
						tmp_LED.DAY_BLINK_Value -= 10;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					if (tmp_LED.ADD_SUNRISE_Value < ( ADD_SUNRISE_S2_MIN + 10))
						tmp_LED.ADD_SUNRISE_Value = ADD_SUNRISE_S1_MAX
								- ADD_SUNRISE_S2_MIN
								+ tmp_LED.ADD_SUNRISE_Value;
					tmp_LED.ADD_SUNRISE_Value -= 10;
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					break;
				case 6:
					if (tmp_LED.NIGHT_BRIGHTNESS_Value < 10)
						tmp_LED.NIGHT_BRIGHTNESS_Value =
								tmp_LED.NIGHT_BRIGHTNESS_Value + 100;
					tmp_LED.NIGHT_BRIGHTNESS_Value -= 10;
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 7:
					if (tmp_LED.TYPE_Value == WHITE_LED) {
						if (tmp_LED.NIGHT_BLINK_Value < (BLINK_MIN + 10))
							tmp_LED.NIGHT_BLINK_Value = BLINK_MAX
									+ tmp_LED.NIGHT_BLINK_Value;
						tmp_LED.NIGHT_BLINK_Value -= 10;
					}
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					if (tmp_LED.ADD_SUNSET_Value < ( ADD_SUNSET_S2_MIN + 10))
						tmp_LED.ADD_SUNSET_Value = ADD_SUNSET_S2_MAX
								- ADD_SUNRISE_S2_MIN + tmp_LED.ADD_SUNSET_Value;
					tmp_LED.ADD_SUNSET_Value -= 10;
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 5:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 7:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

					break;
				case 8:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);

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
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
					S2_LED_Value.DAY_BRIGHTNESS_Value =
							tmp_LED.DAY_BRIGHTNESS_Value;
					S2_LED_Value.NIGHT_BLINK_Value = tmp_LED.NIGHT_BLINK_Value;
					S2_LED_Value.NIGHT_BRIGHTNESS_Value =
							tmp_LED.NIGHT_BRIGHTNESS_Value;
					if (S1_LED_Value.TYPE_Value == WHITE_LED
							&& S2_LED_Value.TYPE_Value == WHITE_LED) {
						S1_LED_Value.DAY_BLINK_Value =
								S2_LED_Value.DAY_BLINK_Value;
						S1_LED_Value.NIGHT_BLINK_Value =
								S2_LED_Value.NIGHT_BLINK_Value;
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_DAY_BLINK],
								S1_LED_Value.DAY_BLINK_Value);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_NIGHT_BLINK],
								S1_LED_Value.NIGHT_BLINK_Value);
					}
					S1_LED_Value.ADD_SUNRISE_Value =
							S2_LED_Value.ADD_SUNRISE_Value;
					S1_LED_Value.ADD_SUNSET_Value =
							S2_LED_Value.ADD_SUNSET_Value;

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_TYPE],
							S2_LED_Value.TYPE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_DAY_BRIGHTNESS],
							S2_LED_Value.DAY_BRIGHTNESS_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_DAY_BLINK],
							S2_LED_Value.DAY_BLINK_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BRIGHTNESS],
							S2_LED_Value.NIGHT_BRIGHTNESS_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_NIGHT_BLINK],
							S2_LED_Value.NIGHT_BLINK_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNRISE],
							S2_LED_Value.ADD_SUNRISE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNSET],
							S2_LED_Value.ADD_SUNSET_Value);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,LED SET2,%s,%d,%f,%f,%d,%f,%f\n",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							(S2_LED_Value.TYPE_Value == IR_LED) ?
									"IR" : "WHITE",
							S2_LED_Value.DAY_BRIGHTNESS_Value,
							S2_LED_Value.DAY_BLINK_Value / 10.0,
							S2_LED_Value.ADD_SUNRISE_Value / 10.0,
							S2_LED_Value.NIGHT_BRIGHTNESS_Value,
							S2_LED_Value.NIGHT_BLINK_Value / 10.0,
							S2_LED_Value.ADD_SUNSET_Value / 10.0);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					////////////////////////////LED control////////////////////////////////////
					if (cur_time.Hours == 0 && cur_time.Minutes == 0
							&& cur_time.Seconds == 0) {
						tmp_dlat = POS2double(LAT_Value);
						tmp_dlong = POS2double(LONG_Value);
						Astro_sunRiseSet(tmp_dlat, tmp_dlong,
								UTC_OFF_Value / 10.0, cur_date_t, &cur_sunrise,
								&cur_noon, &cur_sunset, 1);
					}
					if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
							S1_LED_Value.ADD_SUNRISE_Value / 10.0,
							S1_LED_Value.ADD_SUNSET_Value / 10.0)
							== ASTRO_DAY) {
						pca9632_setbrighnessblinking(LEDS1,
								S1_LED_Value.DAY_BRIGHTNESS_Value,
								S1_LED_Value.DAY_BLINK_Value / 10.0);
					} else {
						pca9632_setbrighnessblinking(LEDS1,
								S1_LED_Value.NIGHT_BRIGHTNESS_Value,
								S1_LED_Value.NIGHT_BLINK_Value / 10.0);
					}
					if (Astro_CheckDayNight(cur_time, cur_sunrise, cur_sunset,
							S2_LED_Value.ADD_SUNRISE_Value / 10.0,
							S2_LED_Value.ADD_SUNSET_Value / 10.0)
							== ASTRO_DAY) {
						pca9632_setbrighnessblinking(LEDS2,
								S2_LED_Value.DAY_BRIGHTNESS_Value,
								S2_LED_Value.DAY_BLINK_Value / 10.0);
					} else {
						pca9632_setbrighnessblinking(LEDS2,
								S2_LED_Value.NIGHT_BRIGHTNESS_Value,
								S2_LED_Value.NIGHT_BLINK_Value / 10.0);

					}

					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%03d", tmp_LED.DAY_BRIGHTNESS_Value);
					index_option = 3;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 3:
					sprintf(tmp_str, "%4.1f", tmp_LED.DAY_BLINK_Value / 10.0);
					index_option = 4;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.DAY_BRIGHTNESS_Value,
							tmp_LED.DAY_BLINK_Value / 10.0);
					break;
				case 4:
					sprintf(tmp_str, "%+3.1f",
							tmp_LED.ADD_SUNRISE_Value / 10.0);
					index_option = 5;

					break;
				case 5:
					sprintf(tmp_str, "%03d", tmp_LED.NIGHT_BRIGHTNESS_Value);
					index_option = 6;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 6:
					sprintf(tmp_str, "%4.1f", tmp_LED.NIGHT_BLINK_Value / 10.0);
					index_option = 7;
					pca9632_setbrighnessblinking(LEDS2,
							tmp_LED.NIGHT_BRIGHTNESS_Value,
							tmp_LED.NIGHT_BLINK_Value / 10.0);
					break;
				case 7:
					sprintf(tmp_str, "%+3.1f", tmp_LED.ADD_SUNSET_Value / 10.0);
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
						tmp_TEC_STATE = 0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE = 1;
					}
					break;
				case 3:
					if (tmp_Relay1.Edge[0] == 'U') {
						tmp_Relay1.Edge[0] = 'D';
						tmp_Relay1.active[0] = 1;
					} else if (tmp_Relay1.Edge[0] == 'D') {
						tmp_Relay1.Edge[0] = '-';
						tmp_Relay1.active[0] = 0;
					} else {
						tmp_Relay1.Edge[0] = 'U';
						tmp_Relay1.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0] += 1;
					if (tmp_Relay1.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[0] / 10.0);
					break;

				case 5:
					if (tmp_Relay1.Edge[1] == 'U') {
						tmp_Relay1.Edge[1] = 'D';
						tmp_Relay1.active[1] = 1;
					} else if (tmp_Relay1.Edge[1] == 'D') {
						tmp_Relay1.Edge[1] = '-';
						tmp_Relay1.active[1] = 0;
					} else {
						tmp_Relay1.Edge[1] = 'U';
						tmp_Relay1.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1] += 1;
					if (tmp_Relay1.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[1] / 10.0);

					break;
				case 7:

					if (tmp_Relay2.Edge[0] == 'U') {
						tmp_Relay2.Edge[0] = 'D';
						tmp_Relay2.active[0] = 1;
					} else if (tmp_Relay2.Edge[0] == 'D') {
						tmp_Relay2.Edge[0] = '-';
						tmp_Relay2.active[0] = 0;
					} else {
						tmp_Relay2.Edge[0] = 'U';
						tmp_Relay2.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0] += 1;
					if (tmp_Relay2.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[0] / 10.0);
					break;
				case 9:
					if (tmp_Relay2.Edge[1] == 'U') {
						tmp_Relay2.Edge[1] = 'D';
						tmp_Relay2.active[1] = 1;
					} else if (tmp_Relay2.Edge[1] == 'D') {
						tmp_Relay2.Edge[1] = '-';
						tmp_Relay2.active[1] = 0;
					} else {
						tmp_Relay2.Edge[1] = 'U';
						tmp_Relay2.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1] += 1;
					if (tmp_Relay2.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[1] / 10.0);
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
						tmp_TEC_STATE = 0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE = 1;
					}
					break;
				case 3:
					if (tmp_Relay1.Edge[0] == 'U') {
						tmp_Relay1.Edge[0] = 'D';
						tmp_Relay1.active[0] = 1;
					} else if (tmp_Relay1.Edge[0] == 'D') {
						tmp_Relay1.Edge[0] = '-';
						tmp_Relay1.active[0] = 0;
					} else {
						tmp_Relay1.Edge[0] = 'U';
						tmp_Relay1.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0] += 100;
					if (tmp_Relay1.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[0] / 10.0);
					break;

				case 5:
					if (tmp_Relay1.Edge[1] == 'U') {
						tmp_Relay1.Edge[1] = 'D';
						tmp_Relay1.active[1] = 1;
					} else if (tmp_Relay1.Edge[1] == 'D') {
						tmp_Relay1.Edge[1] = '-';
						tmp_Relay1.active[1] = 0;
					} else {
						tmp_Relay1.Edge[1] = 'U';
						tmp_Relay1.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1] += 100;
					if (tmp_Relay1.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[1] / 10.0);

					break;
				case 7:

					if (tmp_Relay2.Edge[0] == 'U') {
						tmp_Relay2.Edge[0] = 'D';
						tmp_Relay2.active[0] = 1;
					} else if (tmp_Relay2.Edge[0] == 'D') {
						tmp_Relay2.Edge[0] = '-';
						tmp_Relay2.active[0] = 0;
					} else {
						tmp_Relay2.Edge[0] = 'U';
						tmp_Relay2.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0] += 100;
					if (tmp_Relay2.Temperature[0] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[0] / 10.0);
					break;
				case 9:
					if (tmp_Relay2.Edge[1] == 'U') {
						tmp_Relay2.Edge[1] = 'D';
						tmp_Relay2.active[1] = 1;
					} else if (tmp_Relay2.Edge[1] == 'D') {
						tmp_Relay2.Edge[1] = '-';
						tmp_Relay2.active[1] = 0;
					} else {
						tmp_Relay2.Edge[1] = 'U';
						tmp_Relay2.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1] += 100;
					if (tmp_Relay2.Temperature[1] > TEMPERATURE_MAX)
						tmp_Relay2.Temperature[1] = TEMPERATURE_MIN;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[1] / 10.0);
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
						tmp_TEC_STATE = 0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE = 1;
					}
					break;
				case 3:
					if (tmp_Relay1.Edge[0] == 'U') {
						tmp_Relay1.Edge[0] = 'D';
						tmp_Relay1.active[0] = 1;
					} else if (tmp_Relay1.Edge[0] == 'D') {
						tmp_Relay1.Edge[0] = '-';
						tmp_Relay1.active[0] = 0;
					} else {
						tmp_Relay1.Edge[0] = 'U';
						tmp_Relay1.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0] -= 1;
					if (tmp_Relay1.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[0] / 10.0);
					break;

				case 5:
					if (tmp_Relay1.Edge[1] == 'U') {
						tmp_Relay1.Edge[1] = 'D';
						tmp_Relay1.active[1] = 1;
					} else if (tmp_Relay1.Edge[1] == 'D') {
						tmp_Relay1.Edge[1] = '-';
						tmp_Relay1.active[1] = 0;
					} else {
						tmp_Relay1.Edge[1] = 'U';
						tmp_Relay1.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1] -= 1;
					if (tmp_Relay1.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[1] / 10.0);

					break;
				case 7:

					if (tmp_Relay2.Edge[0] == 'U') {
						tmp_Relay2.Edge[0] = 'D';
						tmp_Relay2.active[0] = 1;
					} else if (tmp_Relay2.Edge[0] == 'D') {
						tmp_Relay2.Edge[0] = '-';
						tmp_Relay2.active[0] = 0;
					} else {
						tmp_Relay2.Edge[0] = 'U';
						tmp_Relay2.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0] -= 1;
					if (tmp_Relay2.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[0] / 10.0);
					break;
				case 9:
					if (tmp_Relay2.Edge[1] == 'U') {
						tmp_Relay2.Edge[1] = 'D';
						tmp_Relay2.active[1] = 1;
					} else if (tmp_Relay2.Edge[1] == 'D') {
						tmp_Relay2.Edge[1] = '-';
						tmp_Relay2.active[1] = 0;
					} else {
						tmp_Relay2.Edge[1] = 'U';
						tmp_Relay2.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1] -= 1;
					if (tmp_Relay2.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[1] / 10.0);
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
						tmp_TEC_STATE = 0;
					} else {
						sprintf(tmp_str, "ENABLE");
						tmp_TEC_STATE = 1;
					}
					break;
				case 3:
					if (tmp_Relay1.Edge[0] == 'U') {
						tmp_Relay1.Edge[0] = 'D';
						tmp_Relay1.active[0] = 1;
					} else if (tmp_Relay1.Edge[0] == 'D') {
						tmp_Relay1.Edge[0] = '-';
						tmp_Relay1.active[0] = 0;
					} else {
						tmp_Relay1.Edge[0] = 'U';
						tmp_Relay1.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
					break;
				case 4:
					tmp_Relay1.Temperature[0] -= 100;
					if (tmp_Relay1.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[0] / 10.0);
					break;

				case 5:
					if (tmp_Relay1.Edge[1] == 'U') {
						tmp_Relay1.Edge[1] = 'D';
						tmp_Relay1.active[1] = 1;
					} else if (tmp_Relay1.Edge[1] == 'D') {
						tmp_Relay1.Edge[1] = '-';
						tmp_Relay1.active[1] = 0;
					} else {
						tmp_Relay1.Edge[1] = 'U';
						tmp_Relay1.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					break;
				case 6:
					tmp_Relay1.Temperature[1] -= 100;
					if (tmp_Relay1.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay1.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay1.Temperature[1] / 10.0);

					break;
				case 7:

					if (tmp_Relay2.Edge[0] == 'U') {
						tmp_Relay2.Edge[0] = 'D';
						tmp_Relay2.active[0] = 1;
					} else if (tmp_Relay2.Edge[0] == 'D') {
						tmp_Relay2.Edge[0] = '-';
						tmp_Relay2.active[0] = 0;
					} else {
						tmp_Relay2.Edge[0] = 'U';
						tmp_Relay2.active[0] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);

					break;
				case 8:
					tmp_Relay2.Temperature[0] -= 100;
					if (tmp_Relay2.Temperature[0] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[0] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[0] / 10.0);
					break;
				case 9:
					if (tmp_Relay2.Edge[1] == 'U') {
						tmp_Relay2.Edge[1] = 'D';
						tmp_Relay2.active[1] = 1;
					} else if (tmp_Relay2.Edge[1] == 'D') {
						tmp_Relay2.Edge[1] = '-';
						tmp_Relay2.active[1] = 0;
					} else {
						tmp_Relay2.Edge[1] = 'U';
						tmp_Relay2.active[1] = 1;
					}
					create_formRelay(1, text_pos, tmp_Relay1, tmp_Relay2,
							tmp_TEC_STATE);
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);

					break;
				case 10:

					tmp_Relay2.Temperature[1] -= 100;
					if (tmp_Relay2.Temperature[1] < TEMPERATURE_MIN)
						tmp_Relay2.Temperature[1] = TEMPERATURE_MAX;
					sprintf(tmp_str, "%+4.1f",
							tmp_Relay2.Temperature[1] / 10.0);
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
					if (tmp_Relay2.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[1] / 10.0);
						index_option = 10;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option = 9;
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
					if (tmp_TEC_STATE)
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
					if (tmp_Relay1.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[0] / 10.0);
						index_option = 4;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[0]);
						index_option = 3;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 7:
					if (tmp_Relay1.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[1] / 10.0);
						index_option = 6;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option = 5;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 9:
					if (tmp_Relay2.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[0] / 10.0);
						index_option = 8;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option = 7;
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
					if (tmp_TEC_STATE)
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
					if (tmp_Relay1.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[0] / 10.0);
						index_option = 4;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option = 5;
					}

					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 5:
					if (tmp_Relay1.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[1] / 10.0);
						index_option = 6;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option = 7;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 7:
					if (tmp_Relay1.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[0] / 10.0);
						index_option = 8;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option = 9;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
					index_option = 9;
					break;
				case 9:
					if (tmp_Relay2.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[1] / 10.0);
						index_option = 10;
					} else {
						text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1,
								1);
						index_option = 0;
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
					TEC_STATE_Value = tmp_TEC_STATE;
					RELAY1_Value = tmp_Relay1;
					RELAY2_Value = tmp_Relay2;

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TEC_STATE],
							TEC_STATE_Value);

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Temperature0],
							RELAY1_Value.Temperature[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Edge0],
							RELAY1_Value.Edge[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_active0],
							RELAY1_Value.active[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Temperature1],
							RELAY1_Value.Temperature[1]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_Edge1],
							RELAY1_Value.Edge[1]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY1_active1],
							RELAY1_Value.active[1]);

					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Temperature0],
							RELAY2_Value.Temperature[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Edge0],
							RELAY2_Value.Edge[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_active0],
							RELAY2_Value.active[0]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Temperature1],
							RELAY2_Value.Temperature[1]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_Edge1],
							RELAY2_Value.Edge[1]);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_RELAY2_active1],
							RELAY2_Value.active[1]);

					if (TEC_STATE_Value)
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC ENABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					else
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC DISABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

					if (RELAY1_Value.active[0])
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%c%f",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds, RELAY1_Value.Edge[0],
								RELAY1_Value.Temperature[0]);
					else
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,--",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					if (RELAY1_Value.active[1])
						sprintf(tmp_str, "%s,%c%f\n", tmp_str,
								RELAY1_Value.Edge[1],
								RELAY1_Value.Temperature[1]);
					else
						sprintf(tmp_str, "%s,--\n", tmp_str);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

					if (RELAY2_Value.active[0])
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,%c%f",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds, RELAY2_Value.Edge[0],
								RELAY2_Value.Temperature[0]);
					else
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,--",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					if (RELAY2_Value.active[1])
						sprintf(tmp_str, "%s,%c%f\n", tmp_str,
								RELAY2_Value.Edge[1],
								RELAY2_Value.Temperature[1]);
					else
						sprintf(tmp_str, "%s,--\n", tmp_str);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

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
					if (tmp_Relay1.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[0] / 10.0);
						index_option = 4;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
						index_option = 5;
					}

					break;
				case 4:
					sprintf(tmp_str, "%c", tmp_Relay1.Edge[1]);
					index_option = 5;
					break;
				case 5:
					if (tmp_Relay1.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay1.Temperature[1] / 10.0);
						index_option = 6;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
						index_option = 7;
					}
					break;
				case 6:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[0]);
					index_option = 7;
					break;
				case 7:
					if (tmp_Relay1.active[0]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[0] / 10.0);
						index_option = 8;
					} else {
						sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
						index_option = 9;
					}
					break;
				case 8:
					sprintf(tmp_str, "%c", tmp_Relay2.Edge[1]);
					index_option = 9;
					break;
				case 9:
					if (tmp_Relay2.active[1]) {
						sprintf(tmp_str, "%+4.1f",
								tmp_Relay2.Temperature[1] / 10.0);
						index_option = 10;
					} else {
						text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1,
								1);
						index_option = 0;
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
			if (flag_rtc_1s) {
				flag_rtc_1s = 0;
				draw_fill(text_pos[2].x1 - 1, text_pos[2].y1 + 1,
						text_pos[2].x2 - 1, text_pos[2].y2 - 1, 0);

				if (vcnl4200_ps(&cur_insidelight) == HAL_OK)
					sprintf(tmp_str1, "%05d", cur_insidelight);
				else
					sprintf(tmp_str1, "----");
				text_cell(text_pos, 2, tmp_str1, Tahoma8, CENTER_ALIGN, 0, 0);
				glcd_refresh();
			}
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
				case 3:
					tmp_door++;
					sprintf(tmp_str, "%05d", tmp_door);
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
				case 3:
					tmp_door += 100;
					sprintf(tmp_str, "%05d", tmp_door);
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
				case 3:
					tmp_door--;
					sprintf(tmp_str, "%05d", tmp_door);
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
				case 3:
					tmp_door -= 100;
					sprintf(tmp_str, "%05d", tmp_door);
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
					sprintf(tmp_str, "%05d", tmp_door);
					index_option = 3;
					break;
				case 1:	//CANCEL
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 3:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
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
					sprintf(tmp_str, "%05d", tmp_door);
					index_option = 3;
					break;
				case 3:
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
					DOOR_Value = tmp_door;
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_DOOR], DOOR_Value);
					sprintf(tmp_str, "%04d-%02d-%02d,%02d:%02d:%02d,Door,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							DOOR_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str);

					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 3:
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
			/////////////////////////////////////CHANGEPASS_MENU/////////////////////////////////////////////////
		case CHANGEPASS_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
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
				case 3:
				case 4:
				case 5:
					tmp_pass[index_option - 2] = (char) tmp_pass[index_option
							- 2] + 1;
					if (tmp_pass[index_option - 2] > '9'
							&& tmp_pass[index_option - 2] < 'A')
						tmp_pass[index_option - 2] = 'A';
					else if (tmp_pass[index_option - 2] > 'Z'
							&& tmp_pass[index_option - 2] < 'a')
						tmp_pass[index_option - 2] = 'a';
					if (tmp_pass[index_option - 2] > 'z')
						tmp_pass[index_option - 2] = '0';
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 6:
				case 7:
				case 8:
				case 9:
					tmp_confirmpass[index_option - 6] =
							(char) tmp_confirmpass[index_option - 6] + 1;
					if (tmp_confirmpass[index_option - 6] > '9'
							&& tmp_confirmpass[index_option - 6] < 'A')
						tmp_confirmpass[index_option - 6] = 'A';
					else if (tmp_confirmpass[index_option - 6] > 'Z'
							&& tmp_confirmpass[index_option - 6] < 'a')
						tmp_confirmpass[index_option - 6] = 'a';
					if (tmp_confirmpass[index_option - 6] > 'z')
						tmp_confirmpass[index_option - 6] = '0';
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
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
				case 3:
				case 4:
				case 5:
					tmp_pass[index_option - 2] = (char) tmp_pass[index_option
							- 2] - 1;
					if (tmp_pass[index_option - 2] < '0')
						tmp_pass[index_option - 2] = 'z';
					else if (tmp_pass[index_option - 2] < 'a'
							&& tmp_pass[index_option - 2] > 'Z')
						tmp_pass[index_option - 2] = 'Z';
					else if (tmp_pass[index_option - 2] < 'A'
							&& tmp_pass[index_option - 2] > '9')
						tmp_pass[index_option - 2] = '9';
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);

					break;
				case 6:
				case 7:
				case 8:
				case 9:
					tmp_confirmpass[index_option - 6] =
							(char) tmp_confirmpass[index_option - 6] - 1;
					if (tmp_confirmpass[index_option - 6] < '0')
						tmp_confirmpass[index_option - 6] = 'z';
					else if (tmp_confirmpass[index_option - 6] < 'a'
							&& tmp_confirmpass[index_option - 6] > 'Z')
						tmp_confirmpass[index_option - 6] = 'Z';
					else if (tmp_confirmpass[index_option - 6] < 'A'
							&& tmp_confirmpass[index_option - 6] > '9')
						tmp_confirmpass[index_option - 6] = '9';
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					index_option = 1;
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					index_option = 2;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 2:
				case 3:
				case 4:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 5:
				case 6:
				case 7:
				case 8:
					index_option++;
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
					break;
				case 9:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
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
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					index_option = 9;
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					index_option = 1;
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,
							1);
					break;
				case 3:
				case 4:
				case 5:
				case 6:
					index_option--;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 7:
				case 8:
				case 9:
					index_option--;
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2 - 1,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
					glcd_refresh();
				}
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					if (strcmp(tmp_pass, tmp_confirmpass)) {
						bounding_box_t tmp_box = { .x1 = 50, .x2 = 127,
								.y1 = 51, .y2 = 63 };
						text_cell(&tmp_box, 0, "not same!", Tahoma8,
								CENTER_ALIGN, 0, 0);
						glcd_refresh();
						HAL_Delay(2000);
						sprintf(tmp_pass, "****");
						sprintf(tmp_confirmpass, "****");
						create_formChangepass(1, text_pos, tmp_pass,
								tmp_confirmpass);
						index_option = 2;
						sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);
						glcd_refresh();
					} else {
						strncpy(PASSWORD_Value, tmp_pass, 5);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_0],
								PASSWORD_Value[0]);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_1],
								PASSWORD_Value[1]);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_2],
								PASSWORD_Value[2]);
						HAL_Delay(50);
						EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_3],
								PASSWORD_Value[3]);

						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,PASSWORD CHANGED",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str);

						create_menu(0, 1, text_pos);
						index_option = 0;
						MENU_state = OPTION_MENU;
					}
					break;
				case 1:					//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
				case 3:
				case 4:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 2]);
					break;
				case 5:
				case 6:
				case 7:
				case 8:
					index_option++;
					sprintf(tmp_str, "%c", tmp_confirmpass[index_option - 6]);
					break;
				case 9:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
			break;
			/////////////////////////////////////COPY_MENU/////////////////////////////////////////////////
		case COPY_MENU:
			Copy2USB();
			create_menu(0, 1, text_pos);
			index_option = 0;
			MENU_state = OPTION_MENU;
			break;
			/////////////////////////////////////UPGRADE_MENU/////////////////////////////////////////////////
		case UPGRADE_MENU:

			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				draw_fill(text_pos[index_option].x1 + 1,
						text_pos[index_option].y1 + 1,
						text_pos[index_option].x2 - 1,
						text_pos[index_option].y2 - 1, 0);
				text_cell(text_pos, index_option, tmp_str, Tahoma8,
						CENTER_ALIGN, 0, 0);
				if (index_option == 1)
					index_option = 6;
				index_option--;
				switch (index_option) {
//				case 0:	//OK
//					sprintf(tmp_str,"OK");
//					text_cell(text_pos, index_option, tmp_str, Tahoma8,
//							CENTER_ALIGN, 1, 1);
//					break;
				case 1:	//CANCEL
					sprintf(tmp_str, "CANCEL");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 1);
					break;
				default:
					sprintf(tmp_str, "%s", menu_upgrade[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					break;
				}

				glcd_refresh();
			}

			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				draw_fill(text_pos[index_option].x1 + 1,
						text_pos[index_option].y1 + 1,
						text_pos[index_option].x2 - 1,
						text_pos[index_option].y2 - 1, 0);
				text_cell(text_pos, index_option, tmp_str, Tahoma8,
						CENTER_ALIGN, 0, 0);
				index_option++;
				if (index_option > 5)
					index_option = 1;
				switch (index_option) {
//				case 0:	//OK
//					sprintf(tmp_str,"OK");
//					text_cell(text_pos, index_option, tmp_str, Tahoma8,
//							CENTER_ALIGN, 1, 1);
//					break;
				case 1:	//CANCEL
					sprintf(tmp_str, "CANCEL");
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 1);
					break;
				default:
					sprintf(tmp_str, "%s", menu_upgrade[index_option - 2]);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
					break;
				}

				glcd_refresh();
			}
			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);

				switch (index_option) {
//				case 0:	//OK
//					sprintf(tmp_str,"OK");
//					text_cell(text_pos, index_option, tmp_str, Tahoma8,
//							CENTER_ALIGN, 1, 1);
//					break;
				case 1:	//CANCEL
					create_menu(0, 1, text_pos);
					index_option = 0;
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sharedmem = WRITE_FROM_SD;
					break;
				case 3:
					sharedmem = WRITE_FROM_USB;
					break;
				case 4:
					sharedmem = FORCE_WRITE_FROM_SD;
					break;
				case 5:
					sharedmem = FORCE_WRITE_FROM_USB;
					break;

				}
				if (index_option > 1) {
					Peripherials_DeInit();
					HAL_Delay(100);
					SCB->AIRCR = 0x05FA0000 | (uint32_t) 0x04; //system reset
					while (1)
						;
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////EXIT_MENU/////////////////////////////////////////////////
		case EXIT_MENU:
			MENU_state = MAIN_MENU;
			DISP_state = DISP_IDLE;
			flag_change_form = 1;
			break;
		}
		///////////////////////////////////////////////END SWITCH of state machine//////////////////////////////////////////////////////
		/////////////////////Temperature Control Algorithm//////////////////////////////////
		if (TEC_STATE_Value) {
			if (0)		//(NTC_Centigrade>NTCTH_Value))
			{
				//			FAN2_ON();
				//			sprintf(FAN_chstate,"ON");
				//			FAN_OFF();
				//			sprintf(TEC_chstate,"OFF");
				//			TEC_overtemp	=1;
			} else if (0)//NTC_Centigrade<NTCTH_Value  && NTC_Centigrade>NTCTL_Value && TEC_overtemp)
			{
				//			FAN2_ON();
				//			sprintf(FAN_chstate,"ON");
				//			FAN_OFF();
				//			sprintf(TEC_chstate,"OFF");
			} else //if((NTC_Centigrade<NTCTH_Value &&  TEC_overtemp==0)|| (NTC_Centigrade<NTCTL_Value))
			{
				//			TEC_overtemp=0;
				Env_temperature = cur_temperature[4];
				Delta_T = Env_temperature - prev_Env_temperature;
				if (Env_temperature > RELAY1_Value.Temperature[0]) //s1
						{
					FAN_ON();
					TEC_COLD();
					if (algorithm_temp_state != 1) {
						algorithm_temp_state = 1;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,COLD",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}
				}

				else if (Env_temperature <= RELAY1_Value.Temperature[1]
						&& Env_temperature > RELAY2_Value.Temperature[0]) //s4
								{
					FAN_OFF();
					if (algorithm_temp_state != 2) {
						algorithm_temp_state = 2;
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,OFF",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str);
					}
				} else if ((Delta_T > 2)
						&& (Env_temperature > RELAY1_Value.Temperature[1])) //s3
						{
					FAN_OFF();
					if (algorithm_temp_state != 3) {
						algorithm_temp_state = 3;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,OFF",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}
				} else if ((Delta_T < -2)
						&& (Env_temperature > RELAY1_Value.Temperature[1])) //s2
						{
					FAN_ON();
					TEC_COLD();
					if (algorithm_temp_state != 4) {
						algorithm_temp_state = 4;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,COLD",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}
				} else if (Env_temperature < RELAY2_Value.Temperature[1]) //s7
						{
					FAN_ON();
					TEC_HOT();
					if (algorithm_temp_state != 5) {
						algorithm_temp_state = 5;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,HOT",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}

				} else if ((Env_temperature >= RELAY2_Value.Temperature[0])
						&& (Env_temperature <= RELAY1_Value.Temperature[1])) {
					FAN_OFF();
					if (algorithm_temp_state != 6) {
						algorithm_temp_state = 6;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,OFF",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}

				} else if ((Delta_T > 2)
						&& (Env_temperature > RELAY2_Value.Temperature[1])) {
					FAN_ON();
					TEC_HOT();
					if (algorithm_temp_state != 7) {
						algorithm_temp_state = 7;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,HOT",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}

				} else if ((Delta_T < -2)
						&& (Env_temperature > RELAY2_Value.Temperature[1])) {
					FAN_OFF();
					if (algorithm_temp_state != 8) {
						algorithm_temp_state = 8;
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);
					}
				}
				prev_Env_temperature = Env_temperature;

			}
			////////////////////////////////////////////////////////////////////////////////////
		}
		else
		{
			FAN_OFF();
			if(algorithm_temp_state!=9)
			{
				algorithm_temp_state=9;
				sprintf(tmp_str2,
						"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF",
						cur_Date.Year + 2000, cur_Date.Month,
						cur_Date.Date, cur_time.Hours, cur_time.Minutes,
						cur_time.Seconds);
				r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
						tmp_str2);
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////
#if __LWIP__
		MX_LWIP_Process();
#endif
//		if (HAL_GPIO_ReadPin(USB_OVRCUR_GPIO_Port, USB_OVRCUR_Pin)) //no fault
//			HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin,
//					GPIO_PIN_RESET);
//		else
//			HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin,
//					GPIO_PIN_SET);
////		if(!USBH_MSC_IsReady(&hUsbHostHS))
////		{
////			MX_USB_HOST_Init();
////		}
		if (BSP_SD_IsDetected() == SD_PRESENT) {
			HAL_Delay(100);
			if (BSP_SD_IsDetected() == SD_PRESENT) {
				if (SDFatFS.fs_type == 0) {
					HAL_SD_Init(&hsd);
					if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK) {
						printf("mounting SD card\n\r");
						flag_log_param = 1;
						filename_temperature[0] = '\0';
						filename_volampere[0] = '\0';
						filename_light[0] = '\0';
						filename_doorstate[0] = '\0';
						filename_parameter[0] = '\0';
					} else {
						SDFatFS.fs_type = 0;
						HAL_SD_DeInit(&hsd);
						printf("mounting SD card ERROR\n\r");
					}
				}
			}
		} else {
			f_mount(NULL, "0:", 0);
			SDFatFS.fs_type = 0;
			HAL_SD_DeInit(&hsd);
		}
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
