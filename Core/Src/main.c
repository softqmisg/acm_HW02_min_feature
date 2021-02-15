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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
///////////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////////////////////

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t als, white;
uint8_t buffer[2];
HAL_StatusTypeDef status;
/////////////////////////////////////////////////////////////////////////////////////////////////////
double voltage, current, temperature[8];
////////////////////////////////////////////////////////////////////////////////////////////////////
//float TL_Value;
//float TH_Value;
//float DELTAT_Value;
//float HL_Value;
//float HH_Value;
//uint16_t BrightW_Value;
//float BlinkW_Value;
//uint16_t BrightIR_Value;
//uint16_t Password_Value;
//float NTCTL_Value;
//float NTCTH_Value;
double LAT_Value;
double LONG_Value;
uint16_t DAY_BRIGHTNESS_Value;
double  DAY_BLINK_Value;
uint16_t NIGHT_BRIGHTNESS_Value;
double NIGHT_BLINK_Value;

/////////////////////////////////////read value of parameter from eeprom///////////////////////////////
void update_values(void)
{
	LAT_Value=-19.264706;
	LONG_Value=-83.328684;
	DAY_BRIGHTNESS_Value=0;
	DAY_BLINK_Value=0.5;
	NIGHT_BRIGHTNESS_Value=80;
	NIGHT_BLINK_Value=0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
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
			draw_box(xpos, ypos, xpos + step_c - 1, ypos + step_r - 1, colour);
			xpos = xpos + step_c - 1;
		}
		ypos = ypos + step_r - 1;
	}

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
enum {
	LEFT_ALIGN = 0, RIGHT_ALIGN, CENTER_ALIGN
};
void text_cell(bounding_box_t *pos, uint8_t index, char *str,
		unsigned char *font, uint8_t align,unsigned char inv) {
	uint8_t x, y = pos[index].y1 + 1;
	uint8_t length_str = text_width(str, font, 1);
	switch (align) {
	case LEFT_ALIGN:
		x = pos[index].x1 + 1;
		draw_fill(x , y, x + length_str + 1, y + text_height(str, font), 0);
		break;
	case RIGHT_ALIGN:
		x = pos[index].x2 - length_str - 1;
		if(x==0) x=1;
		draw_fill(x - 1, y, x + length_str , y + text_height(str, font), 0);
		break;
	case CENTER_ALIGN:
		x = pos[index].x1 + (pos[index].x2 - pos[index].x1 - length_str) / 2;
		if(x==0) x=1;
		draw_fill(x - 1, y, x + length_str + 1, y + text_height(str, font), 0);
		break;
	}
	if(inv)
		draw_fill(pos[index].x1,pos[index].y1,pos[index].x2,pos[index].y2,1);
	draw_text(str, x, y, font, 1, inv);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
#define MENU_ITEMS	7
char *menu[] = { "SET Position", "SET Time", "SET LED", "SET Relay", "SET Door",
		"SET PASS", "Exit" };
void create_menu(uint8_t selected, uint8_t clear) {
	if (clear)
		glcd_blank();
	bounding_box_t *pos_ = (bounding_box_t*) malloc(
			sizeof(bounding_box_t) * 1 * 2);
	create_cell(0, 0, 128, 64, 1, 2, 1, pos_);
	for (uint8_t op = 0; op < MENU_ITEMS; op++) {
		draw_text(menu[op], (op / 5) * 66 + 2, (op % 5) * 12 + 1, Tahoma8, 1,
				0);
	}
	if (selected < MENU_ITEMS)
		draw_text(menu[selected], (selected / 5) * 66 + 2,
				(selected % 5) * 12 + 1, Tahoma8, 1, 1);
	free(pos_);
	glcd_refresh();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void clock_cell(bounding_box_t *pos_) {
	char tmp_str[40];
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
	draw_fill(1, 1, 126, 1 + text_height(tmp_str, Tahoma8), 0);

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	sprintf(tmp_str, "  %02d:%02d:%02d", sTime.Hours,
			sTime.Minutes, sTime.Seconds);
	draw_text(tmp_str, 1, 1, Tahoma8, 1, 0);
	sprintf(tmp_str, "%4d-%02d-%02d",  sDate.Year + 2000, sDate.Month,
			sDate.Date);
	draw_text(tmp_str, 64, 1, Tahoma8, 1, 0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form1(uint8_t clear) {
	HAL_StatusTypeDef result;
	char tmp_str[40];
	bounding_box_t *pos_ = (bounding_box_t*) malloc(
			sizeof(bounding_box_t) * 2 * 5);
	/////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	//////////////////////////////////////////////////////////
	clock_cell(pos_);
	/////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2 + pos_[0].y1, 4, 2, 1,
			pos_);

	for (uint8_t i = 0; i < 8; i++) {
		if (tmp275_readTemperature(i, &temperature[i]) == HAL_OK) {
			sprintf(tmp_str, "T(%d)=%4.1f", i + 1, temperature[i]);
		} else
			sprintf(tmp_str, "T(%d)=---", i + 1);
		text_cell(pos_, i, tmp_str, Tahoma8, CENTER_ALIGN,0);
	}
	//////////////////////////////////////////////////////////

	free(pos_);
	glcd_refresh();

}
////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form2(uint8_t clear) {
	HAL_StatusTypeDef result;
	char tmp_str[40];
	bounding_box_t *pos_ = (bounding_box_t*) malloc(
			sizeof(bounding_box_t) * 1 * 5);
	/////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	//////////////////////////////////////////////////////////
	clock_cell(pos_);
	/////////////////////////////////////////////////////
	create_cell(25, pos_[0].y2, 128-25, 64 - pos_[0].y2 + pos_[0].y1, 4, 1, 1,
			pos_);
	///////////CH1,7V	//////////////////////////////////////////////////////////
	if (ina3221_readfloat((uint8_t) VOLTAGE_7V, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1f", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readfloat((uint8_t) CURRENT_7V, &current) == HAL_OK)
		sprintf(tmp_str, "%s       C=%4.3f", tmp_str, current);
	else
		sprintf(tmp_str, "%s       C=---", tmp_str);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN,0);
	///////////CH2,12V	//////////////////////////////////////////////////////////
	if (ina3221_readfloat((uint8_t) VOLTAGE_12V, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1f", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readfloat((uint8_t) CURRENT_12V, &current) == HAL_OK)
		sprintf(tmp_str, "%s       C=%4.3f", tmp_str, current);
	else
		sprintf(tmp_str, "%s       C=---", tmp_str);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN,0);
	///////////CH3,3.3V	//////////////////////////////////////////////////////////
	if (ina3221_readfloat((uint8_t) VOLTAGE_3V3, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1f", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readfloat((uint8_t) CURRENT_3V3, &current) == HAL_OK)
		sprintf(tmp_str, "%s       C=%4.3f", tmp_str, current);
	else
		sprintf(tmp_str, "%s       C=---", tmp_str);
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN,0);
	///////////TEC,12V	//////////////////////////////////////////////////////////
	if (ina3221_readfloat((uint8_t) VOLTAGE_TEC, &voltage) == HAL_OK)
		sprintf(tmp_str, "V=%3.1f", voltage);
	else
		sprintf(tmp_str, "V=---");

	if (ina3221_readfloat((uint8_t) CURRENT_TEC, &current) == HAL_OK)
		sprintf(tmp_str, "%s       C=%4.3f", tmp_str, current);
	else
		sprintf(tmp_str, "%s       C=---", tmp_str);
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN,0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y1, 25, (pos_[3].y2 - pos_[0].y1)+1, 4, 1, 1, pos_);
	text_cell(pos_, 0, "7.0", Tahoma8, CENTER_ALIGN,1);
	text_cell(pos_, 1, "12", Tahoma8, CENTER_ALIGN,1);
	text_cell(pos_, 2, "3.3", Tahoma8, CENTER_ALIGN,1);
	text_cell(pos_, 3, "TEC", Tahoma8, CENTER_ALIGN,1);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	free(pos_);
	glcd_refresh();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form3(uint8_t clear)
{
	char tmp_str[40];
	bounding_box_t *pos_ = (bounding_box_t*) malloc(
			sizeof(bounding_box_t) * 1 * 5);
	/////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	/////////////////////////////////////////////////////////
	clock_cell(pos_);
	/////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128,2*((pos_[0].y2 - pos_[0].y1)+1), 2, 1, 1,
			pos_);
	POS_t lat_pos=latdouble2POS(LAT_Value);
	sprintf(tmp_str,"Latitude: %d %d\' %.2f\"%c",lat_pos.deg,lat_pos.min,lat_pos.second,lat_pos.direction);
	pos_[0].x1=1;
	text_cell(pos_, 0, tmp_str, Tahoma8, LEFT_ALIGN,0);
	POS_t long_pos=longdouble2POS(LONG_Value);
	sprintf(tmp_str,"Longitude: %d %d\' %.2f\"%c",long_pos.deg,long_pos.min,long_pos.second,long_pos.direction);
	pos_[1].x1=1;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN,0);
	////////////////////////////////////////////////////////////////////////////////////
	create_cell(30, pos_[1].y2, 128-30, 64 - pos_[1].y2 , 2, 1, 1,
			pos_);
	sprintf(tmp_str, "Bri:%2d%% Blnk:%3.1fHz", DAY_BRIGHTNESS_Value,DAY_BLINK_Value);
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN,0);
	sprintf(tmp_str, "Bri:%2d%% Blnk:%3.1fHz", NIGHT_BRIGHTNESS_Value,NIGHT_BLINK_Value);
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN,0);

	//////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y1, 30,  (pos_[1].y2 - pos_[0].y1)+1 , 2, 1, 1,
			pos_);
	text_cell(pos_, 0, "Day", Tahoma8, CENTER_ALIGN,1);
	text_cell(pos_, 1, "Night", Tahoma8, CENTER_ALIGN,1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	free(pos_);
	glcd_refresh();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

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
#ifdef __LWIP__
  MX_LWIP_Init();
#endif
#ifndef __DEBUG__
  MX_IWDG_Init();
#endif
	/* USER CODE BEGIN 2 */
	/////////////////////////
	bounding_box_t pos_[10];
	uint8_t backlight = 100;
	char tmp_str[100], tmp_str1[100];
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	cur_time.Hours = 12;
	cur_time.Minutes = 59;
	cur_time.Seconds = 0;
	cur_Date.Year = 21;
	cur_Date.Month = 2;
	cur_Date.Date = 15;
	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;
	uint8_t y = 0, x = 0;
	//////////////////////retarget////////////////
	RetargetInit(&huart3);
	//////////////////////init LCD//////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);

//	//////////////////////////load Logo/////////////////////////////////////////
#ifndef __DEBUG__
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
	///////////////////////setting RTC/////////////////////////////////////////////////////////
	if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x5050) {
		HAL_RTC_SetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
		HAL_RTC_SetDate(&hrtc, &cur_Date, RTC_FORMAT_BIN);
	}
	///////////////////////initialize & checking sensors///////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 2, 1, pos_);
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2 + pos_[0].y1, 4, 2, 1,
			&pos_[2]);
	glcd_refresh();
	uint8_t ch,inv;
	for ( ch = TMP_CH0; ch <= TMP_CH7; ch++) {
		if ((status = tmp275_init(ch)) != TMP275_OK) {
			printf("tmp275 sensor (#%d) error\n\r", ch);
			sprintf(tmp_str1, "tmp(%d)ERR", ch);inv=1;

		} else {
			printf("tmp275 sensor (#%d) OK\n\r", ch);
			sprintf(tmp_str1, "tmp(%d)OK", ch);inv=0;
		}
		text_cell(pos_, ch, tmp_str1, Tahoma8, CENTER_ALIGN,inv);
		HAL_Delay(50);
	}

	status = ina3221_init();

	if ((status = vcnl4200_init()) != VCNL4200_OK) {
		printf("vcnl4200 sensor error\n\r");
		sprintf(tmp_str1,"vcnl ERR");inv=1;
	} else {
		printf("vcnl4200 sensor OK\n\r");
		sprintf(tmp_str1,"vcnl OK");inv=0;
	}
	text_cell(pos_, ch, tmp_str1, Tahoma8, CENTER_ALIGN,inv);
	ch++;
	if ((status = veml6030_init()) != VEML6030_OK) {
		printf("veml6030 sensor error\n\r");
		sprintf(tmp_str1,"veml ERR");inv=1;

	} else {
		printf("veml6030 sensor OK\n\r");
		sprintf(tmp_str1,"vcnl OK");inv=0;
	}
	text_cell(pos_, ch, tmp_str1, Tahoma8, CENTER_ALIGN,inv);
	glcd_refresh();
	HAL_Delay(5000);

	//////////////////////////////////////////////////////////////////////////////
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS)
			!= HAL_OK) {
		Error_Handler();
	}
	///////////////////////////////////////////////////////////////////////////////////
	if((HAL_RTCEx_BKUPRead (&hrtc,RTC_BKP_DR1) !=0x5050))
	{
#ifndef __DEBUG__
		HAL_IWDG_Refresh(&hiwdg);
#endif
		 HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5050);
	}
	update_values();
	//////////////////////////////////////////////////////////////////////////////////
	create_menu(5, 1);
	HAL_Delay(2000);
	create_form1(1);
	HAL_Delay(2000);
//	create_form2();
	create_form2(1);
	//
	create_form3(1);
	while (1) {
		if (flag_rtc_1s) {
			flag_rtc_1s = 0;
			create_form3(0);
		}
#ifndef __DEBUG__
	HAL_IWDG_Refresh(&hiwdg);
#endif
	}
	////////////////////////keypad check//////////////////////////////////////////
	glcd_init(64, 128);
	glcd_flip_screen(XTB_YRL);
	joystick_init(Key_ALL, Short_press | Long_press);
//	uint8_t select = 0;
//	while (!select) {
//		joystick_init(Key_ALL, Long_press);
//		if (joystick_read(Key_DOWN, Short_press)) {
//			joystick_init(Key_DOWN, Short_press);
//			draw_text("D", 10, 0, Tahoma8, 0, 0);
//
//		} else {
//			draw_text("D", 10, 0, Tahoma8, 0, 1);
//		}
//
//		if (joystick_read(Key_TOP, Short_press)) {
//			joystick_init(Key_TOP, Short_press);
//			draw_text("T", 20, 0, Tahoma8, 0, 0);
//
//		} else {
//			draw_text("T", 20, 0, Tahoma8, 0, 1);
//
//		}
//
//		if (joystick_read(Key_LEFT, Short_press)) {
//			joystick_init(Key_LEFT, Short_press);
//			draw_text("L", 30, 0, Tahoma8, 0, 0);
//
//		} else {
//			draw_text("L", 30, 0, Tahoma8, 0, 1);
//		}
//
//		if (joystick_read(Key_RIGHT, Short_press)) {
//			joystick_init(Key_RIGHT, Short_press);
//			draw_text("R", 40, 0, Tahoma8, 0, 0);
//
//		} else {
//			draw_text("R", 40, 0, Tahoma8, 0, 1);
//
//		}
//
//		if (joystick_read(Key_ENTER, Short_press)) {
//			joystick_init(Key_ENTER, Short_press);
//			draw_text("E", 50, 0, Tahoma8, 0, 0);
//			select = 1;
//
//		} else {
//			draw_text("E", 50, 0, Tahoma8, 0, 1);
//
//		}
//		glcd_refresh();
//		HAL_Delay(500);
//
//	}
	///////////////////////generate menu/////////////////////////////////////////////
	glcd_blank();

	for (uint8_t op = 0; op < 5; op++) {
		draw_text(menu[op], 0, op * 11, Tahoma8, 1, 0);
	}
	uint8_t cur_op = 0;
	draw_text(menu[cur_op], 0, cur_op * 11, Tahoma8, 1, 1);
	cur_op++;
	glcd_refresh();
	joystick_init(Key_ALL, Both_press);
	do {
		joystick_init(Key_ALL, Long_press);
		if (joystick_read(Key_DOWN, Short_press)) {
			joystick_init(Key_DOWN, Short_press);
			if (cur_op > 0)
				draw_text(menu[cur_op - 1], 0, (cur_op - 1) * 11, Tahoma8, 1,
						0);
			if (cur_op > 4)
				cur_op = 0;
			draw_text(menu[cur_op], 0, cur_op * 11, Tahoma8, 1, 1);
			cur_op++;
			glcd_refresh();

		}
	} while (1);
	//	glcd_blank();
	/////////////////////////RTC_Sensor test////////////////////////////////////////////////////
	HAL_RTC_SetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);

	for (uint8_t ch = TMP_CH0; ch <= TMP_CH7; ch++) {
		status = tmp275_init(ch);
		HAL_Delay(100);
	}
	status = ina3221_init();
	status = vcnl4200_init();
	status = veml6030_init();

	HAL_GPIO_WritePin(TEC_ONOFF_GPIO_Port, TEC_ONOFF_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(TEC_CURDIR_GPIO_Port, TEC_CURDIR_Pin, GPIO_PIN_SET);

	status = ina3221_readreg(INA3221_LED_BASEADDRESS, INA3221_MANUFACTURE_ID,
			buffer);
	sprintf(tmp_str, "MANUFACTURE INA321(1):%c%c\n\r", buffer[0], buffer[1]);
	if (file_write("test.txt", tmp_str) != FR_OK) {
		HAL_Delay(1000);
	}
	status = ina3221_readreg(INA3221_LED_BASEADDRESS, INA3221_DIE_ID, buffer);
	sprintf(tmp_str, "DIE INA321(1):%c%c\n\r", buffer[0], buffer[1]);
	if (file_write("test.txt", tmp_str) != FR_OK) {
		HAL_Delay(1000);
	}
	status = ina3221_readreg(INA3221_TEC_BASEADDRESS, INA3221_MANUFACTURE_ID,
			buffer);
	sprintf(tmp_str, "MANUFACTURE INA321(2):%c%c\n\r", buffer[0], buffer[1]);
	if (file_write("test.txt", tmp_str) != FR_OK) {
		HAL_Delay(1000);
	}
	status = ina3221_readreg(INA3221_TEC_BASEADDRESS, INA3221_DIE_ID, buffer);
	sprintf(tmp_str, "DIE INA321(2):%c%c\n\r", buffer[0], buffer[1]);
	if (file_write("test.txt", tmp_str) != FR_OK) {
		HAL_Delay(1000);
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		HAL_RTC_GetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
		///////////temperatures//////////////////////////////////
		for (uint8_t ch = TMP_CH0; ch <= TMP_CH7; ch++) {
			status = tmp275_readTemperature(ch, &temperature[ch]);
			if (status == HAL_OK) {
				sprintf(tmp_str, "tmp(0x%x):%f\n\r", ch + TMP275_BASEADDRESS,
						temperature[ch]);

			} else {
				sprintf(tmp_str, "tmp(0x%x):ERROR\n\r",
						ch + TMP275_BASEADDRESS);

			}
			if (file_write("test.txt", tmp_str) != FR_OK) {
				HAL_Delay(1000);
			}
			HAL_Delay(100);
		}
		//////////////////////////////////////////////////////////
		status = vcnl4200_read((uint8_t) VCNL4200_ALS_Data, &als);
		if (status == VCNL4200_OK) {
			sprintf(tmp_str, "als:%d\n\r", als);

		} else {
			sprintf(tmp_str, "als:ERROR\n\r");

		}
		if (file_write("test.txt", tmp_str) != FR_OK) {
			HAL_Delay(1000);
		}
		status = vcnl4200_read((uint8_t) VCNL4200_WHITE_Data, &white);
		if (status == VCNL4200_OK) {
			sprintf(tmp_str, "white:%d\n\r", white);

		} else {
			sprintf(tmp_str, "white:ERROR\n\r");

		}
		if (file_write("test.txt", tmp_str) != FR_OK) {
			HAL_Delay(1000);
		}
		///////////CH1,7V
		status = ina3221_readfloat((uint8_t) VOLTAGE_7V, &voltage);
		status = ina3221_readfloat((uint8_t) CURRENT_7V, &current);

		///////////CH2,12V
		status = ina3221_readfloat((uint8_t) VOLTAGE_12V, &voltage);
		status = ina3221_readfloat((uint8_t) CURRENT_12V, &current);
		///////////CH3,3.3V
		status = ina3221_readfloat((uint8_t) VOLTAGE_3V3, &voltage);
		status = ina3221_readfloat((uint8_t) CURRENT_3V3, &current);
		///////////TEC,12V
		status = ina3221_readfloat((uint8_t) VOLTAGE_TEC, &voltage);
		status = ina3221_readfloat((uint8_t) CURRENT_TEC, &current);
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		HAL_Delay(1000);
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
