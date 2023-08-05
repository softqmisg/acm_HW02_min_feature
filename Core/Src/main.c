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
#include "images/acm_logo.h"
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
uint8_t reinit = 0;
void reinit_i2c(I2C_HandleTypeDef *instance) {

	GPIO_InitTypeDef GPIO_InitStruct;
	int timeout = 100;
	int timeout_cnt = 0;

	// 1. Clear PE bit.
	instance->Instance->CR1 &= ~(0x0001);

	//  2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	GPIO_InitStruct.Pin = GPIO_PIN_7; //scl
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = GPIO_PIN_8; //sda
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);

	// 3. Check SCL and SDA High level in GPIOx_IDR.
	while (GPIO_PIN_SET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_7)) {
		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	while (GPIO_PIN_SET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_8)) {
		//Move clock to release I2C
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_RESET);
		asm("nop");
		HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);

		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	// 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_RESET);

	//  5. Check SDA Low level in GPIOx_IDR.
	while (GPIO_PIN_RESET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_8)) {
		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	// 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_RESET);

	//  7. Check SCL Low level in GPIOx_IDR.
	while (GPIO_PIN_RESET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_7)) {
		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	// 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);

	// 9. Check SCL High level in GPIOx_IDR.
	while (GPIO_PIN_SET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_7)) {
		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	// 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);

	// 11. Check SDA High level in GPIOx_IDR.
	while (GPIO_PIN_SET != HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_8)) {
		timeout_cnt++;
		if (timeout_cnt > timeout)
			return;
	}

	// 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_8;
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);

	// 13. Set SWRST bit in I2Cx_CR1 register.
	instance->Instance->CR1 |= 0x8000;

	asm("nop");

	// 14. Clear SWRST bit in I2Cx_CR1 register.
	instance->Instance->CR1 &= ~0x8000;

	asm("nop");

	// 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register
	instance->Instance->CR1 |= 0x0001;

	// Call initialization function.
	HAL_I2C_Init(instance);

//
//
//	HAL_I2C_MspDeInit(&hi2c3);
//	HAL_Delay(100);
//	MX_I2C3_Init();
	for (uint8_t ch = TMP_CH0; ch <= TMP_CH7; ch++) {
		tmp275_init(ch);
		HAL_Delay(20);
	}
	ina3221_init(INA3221_LED_BASEADDRESS);
	ina3221_init(INA3221_TEC_BASEADDRESS);
	pca9632_init();
	vcnl4200_init();
	veml6030_init();

	reinit = 1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t flag_rtc_1s = 0;
uint8_t flag_rtc_1s_general = 0;
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	flag_rtc_1s = 1;
	flag_rtc_1s_general = 1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DashboardActivePage			0
#define LEDActivePage				1
#define TimePositionActivePage		2
#define TemperatureActivePage		3
#define UserOptionActivePage		4
#define WifiActivePage				5
#define UpgradeActivePage			6
#define PasswordActivePage			7

#define MAX_BROWSER_PAGE			8
uint8_t ActiveBrowserPage[MAX_BROWSER_PAGE] = { 0 };
/////////////////////////////////////////////////////////////////////////////////
typedef enum {
	cmd_none = 0x80,
	cmd_submit = 0x81,
	cmd_current = 0x82,
	cmd_default = 0x83,
	cmd_event = 0x84,
	cmd_error = 0x85
} cmd_type_t;

////////////////cmd_code
#define DashboardEvent        0x90
#define TemperatureEvent      0xB0
#define SDCardEvent           0xB1
#define UsbMemEvent           0xB2
#define DateEvent             0xB3
#define TimeEvent             0xB4
#define ClientsEvent          0xB5
#define DoorEvent             0xB6
#define LightoutsideEvent     0xB7
#define LatitudeEvent         0xB8
#define LongitudeEvent        0xB9
#define CVEvent               0xBA
#define LEDS1PageEvent        0xBB
#define LEDS2PageEvent        0xBC
#define TimePositionPageEvent 0xBD
#define TemperaturePageEvent  0xBE
#define UseroptionPageEvent   0xBF
#define WifiPageEvent         0xC0
#define UpgradePageEvent      0xC1
#define PasswordPageEvent     0xC2
#define IamreadyEvent         0xC3
#define ThanksEvent           0xC4
#define StartUploadEvent      0xC5
#define DataUploadEvent       0xC6
#define EndUploadEvent        0xC7

#define readDashboard     0x90
#define LEDS1page         0x91
#define LEDS2page         0x92
#define TimePositionpage  0x93
#define Temperaturepage   0x94
#define Useroptionpage    0x95
#define Wifipage          0x96
#define Upgradepage       0x97
#define Passwordpage      0x98

#define UploadError   0xC4
/////////////////////////////////////UART2->ESP32/////////////////////////////////////////
#if 0
#define MAX_SIZE_FRAME  3000
cmd_type_t received_cmd_type = cmd_none;
uint8_t received_tmp_frame[MAX_SIZE_FRAME];
uint8_t received_frame[MAX_SIZE_FRAME];
uint8_t received_cmd_code;
uint8_t received_state = 0;
uint16_t received_index = 0;
uint8_t received_crc = 0;
uint8_t received_valid = 0;
uint8_t received_byte;
uint16_t received_frame_length = 0;
uint16_t received_msg_length = 0;
uint8_t frame_error=0;
uint8_t usart2_datardy = 0, usart3_datardy = 0;
size_t total_firmwaresize=0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	uint16_t size_msg=0;
	if (huart->Instance == USART2) {
		received_byte = USART2->DR;
//		USART3->DR=USART2->DR;
		switch (received_state) {
		case 0:
			if (received_byte == 0xaa) {
				received_tmp_frame[received_index++] = received_byte;
				received_state = 1;
			}
			break;
		case 1:
			if (received_byte == 0x55) {
				received_tmp_frame[received_index++] = received_byte;
				received_state = 2;
			} else {
				received_index = 0;
				received_state = 0;
				received_valid = 0;
				received_cmd_type = cmd_none;
				frame_error=1;
			}
			break;
		case 2:
			received_tmp_frame[received_index++] = received_byte;
			if (received_index >= MAX_SIZE_FRAME) {
				received_state = 0;
				received_index = 0;
				received_valid = 0;
				received_cmd_type = cmd_none;
				frame_error=1;
			}
			else if(received_index>5)
			{
				size_msg=((uint16_t)received_tmp_frame[4]<<8)+(uint16_t)received_tmp_frame[5];
				if (received_index == size_msg + 8) {

					received_crc = 0;
					for (uint16_t i = 0; i < size_msg + 6; i++) {
						received_crc += received_tmp_frame[i];
					}

					if (received_crc == received_tmp_frame[size_msg+6]) {
						received_valid = 1;
						received_cmd_type = (cmd_type_t) received_tmp_frame[2];
						received_cmd_code = received_tmp_frame[3];
						received_frame_length = size_msg+8;
						received_msg_length=size_msg;
						frame_error=0;
					} else {
						received_valid = 0;
						received_cmd_type = cmd_none;
						frame_error=1;
					}
					received_index = 0;
					received_state = 0;
				}
			}
			break;
		}
		HAL_UART_Receive_IT(&huart2, &received_byte, 1);
	}
//	else if (huart->Instance == USART3) {
//		USART2->DR = USART3->DR;
//		HAL_UART_Receive_IT(&huart3, &PC_data, 1);
//	}
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t uart_transmit_frame(char *msg, cmd_type_t cmd_type, uint8_t cmd_code) {
#if 0
	uint8_t transmit_frame[120];
	transmit_frame[0] = 0xaa;
	transmit_frame[1] = 0x55;
	transmit_frame[2] = (uint8_t) cmd_type;
	transmit_frame[3] = cmd_code;
	transmit_frame[4] = strlen(msg);
	uint8_t index = 0;
	while (msg[index]) {
		transmit_frame[5 + index] = msg[index];
		index++;
	}

	uint8_t crc = 0;
	for (uint8_t i = 0; i < 5 + index; i++) {
		crc += transmit_frame[i];
	}
	transmit_frame[5 + index] = crc;
	index++;
	transmit_frame[5 + index] = '\n';
	if (HAL_UART_Transmit(&huart2, transmit_frame, 6 + index, 3000) != HAL_OK)
	{
		printf("send to esp32 fail!\n\r");
		return 0;
	}
	return 1;
#endif
}
/////////////////////////////////////UART3<->MotherBoard/////////////////////////////////////////
#define INDEX_START_BYTE	0
#define SIZE_START_BYTE		2

#define INDEX_SIZE_BYTE		2
#define SIZE_SIZE_BYTE		1

#define INDEX_TYPE_BYTE		3
#define SIZE_TYPE_BYTE		1

#define INDEX_CHANNEL_BYTE	4
#define SIZE_CHANNEL_BYTE	1

#define INDEX_VALUE_BYTE	5


#define TYPE_MB_TEMPERATURE	0
#define TYPE_MB_LIGHT		1
#define TYPE_MB_VOLTAGE		2
#define TYPE_MB_CURRENT		3
#define TYPE_MB_LED			4
#define TYPE_MB_FAN			5
#define TYPE_MB_ERROR		0xFF

#define MAX_SIZE_FRAME_MB  20


uint8_t received_byte_MB;
uint8_t received_state_MB = 0;
uint16_t received_index_MB = 0;
uint8_t received_tmp_frame_MB[MAX_SIZE_FRAME_MB];
uint8_t received_frame_MB[MAX_SIZE_FRAME_MB];
uint8_t received_valid_MB = 0;
uint16_t size_msg=0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	uint8_t received_crc = 0;

	if (huart->Instance == USART3) {
		received_byte_MB = USART3->DR;
		switch (received_state_MB) {
		case 0://get first start byte
			if (received_byte_MB == 0xaa) {
				received_tmp_frame_MB[received_index_MB++] = received_byte_MB;
				received_state_MB = 1;
			}
			break;
		case 1: //get second start byte
			if (received_byte_MB == 0x55) {
				received_tmp_frame_MB[received_index_MB++] = received_byte_MB;
				received_state_MB = 2;
			} else {
				received_index_MB = 0;
				received_state_MB = 0;
				received_valid_MB = 0;
			}
			break;
		case 2: //get size
			received_tmp_frame_MB[received_index_MB++] = received_byte_MB;
			if(received_index_MB>=INDEX_SIZE_BYTE+SIZE_SIZE_BYTE)
			{
				size_msg=0;
				for(int i=0;i<SIZE_SIZE_BYTE;i++)
				{
					size_msg<<=8;
					size_msg+=(uint16_t)received_tmp_frame_MB[i+INDEX_SIZE_BYTE];
				}
				received_state_MB = 3;
			}
		break;
		case 3://get other values
			received_tmp_frame_MB[received_index_MB++] = received_byte_MB;
			if (received_index_MB >= MAX_SIZE_FRAME_MB) {
				received_state_MB = 0;
				received_index_MB = 0;
				received_valid_MB = 0;
			}
			else if(received_index_MB>INDEX_VALUE_BYTE+size_msg)
			{
					received_crc = 0;
					for (uint16_t i = 0; i < size_msg + INDEX_VALUE_BYTE; i++) {
						received_crc += received_tmp_frame_MB[i];
					}

					if (received_crc == received_tmp_frame_MB[size_msg+INDEX_VALUE_BYTE]) {
						received_valid_MB = 1;
					} else {
						received_valid_MB = 0;
					}
					received_index_MB = 0;
					received_state_MB = 0;
			}
			break;
		}
		HAL_UART_Receive_IT(&huart3, &received_byte_MB, 1);
	}

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t uart_transmit_frame_MB(UART_HandleTypeDef *huart,int8_t size,uint8_t type,uint8_t channel, uint8_t *data) {

	uint8_t transmit_frame[20];
	transmit_frame[0] = 0xaa;
	transmit_frame[1] = 0x55;
	transmit_frame[2] =  size;
	transmit_frame[3] = type;
	transmit_frame[4] = channel;
	for (uint16_t i = 0; i < size; i++) {
		transmit_frame[INDEX_VALUE_BYTE + i] = data[i];
	}

	uint8_t crc = 0;
	for (uint16_t i = 0; i < INDEX_VALUE_BYTE+size  ; i++) {
		crc += transmit_frame[i];
	}

	transmit_frame[INDEX_VALUE_BYTE+size] = crc;
	if (HAL_UART_Transmit(huart, transmit_frame, INDEX_VALUE_BYTE + size+1, 3000) != HAL_OK)
	{
		printf("send to esp32 fail!\n\r");
		return 0;
	}
	return 1;
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
	DISP_FORM5_5,
	DISP_FORM6,
	DISP_FORM7
} DISP_state = DISP_IDLE;
enum {
	MAIN_MENU = 0,
	PASS_MENU,
	OPTION_MENU,
	POSITION_MENU,
	TIME_MENU,
	LEDS1_MENU,
	LEDS2_MENU,
	DOOR_MENU,
	TEMP1_MENU,
	TEMP2_MENU,
	USEROPTION_MENU,
	CHANGEPASS_MENU,
	NEXT_MENU,
	PREV_MENU,
	WIFI1_MENU,
	WIFI2_MENU,
	UPGRADE_MENU,
	COPY_MENU,
	FACTORYRESET_MENU,
	EXIT_MENU
} MENU_state = MAIN_MENU;

char *menu[] = { "SET Position", "SET Time", "SET LED S1", "SET LED S2",
		"SET Door", "SET Temp 1", "SET Temp 2", "User Option", "SET PASS",
		"Next->", "<-Prev", "SET WIFI 1", "SET WIFI 2", "Upgrade", "Copy USB",
		"Factory rst", "Exit" };

char *menu_upgrade[] = { "Upgrade from SD", "Upgrade from USB",
		"Force upgrade from SD", "Force upgrade from  USB" };

uint8_t profile_active[][MENU_TOTAL_ITEMS] = { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1,
		1 } };

typedef struct {
	char ssid[12];
	char pass[12];
	char ip[16];
	char gateway[16];
	char netmask[16];
	uint8_t txpower;
	uint8_t ssidhidden;
	uint8_t maxclients;
} WiFi_t;
WiFi_t cur_wifi = { 0 };
char *tx_array[] = { "19.5dBm", "19  dBm", "18.5dBm", "17  dBm", "15  dBm",
		"13  dBm", "11  dBm", "8.5 dBm", "7   dBm", "5   dBm", "2   dBm",
		"-1  dBm" };
/////////////////////////////////////////////////parameter variable///////////////////////////////////////////////////////////

LED_t S1_LED_Value, S2_LED_Value;
RELAY_t TempLimit_Value[6];
uint8_t TEC_STATE_Value;
uint8_t HYSTERESIS_Value;
POS_t LAT_Value;
POS_t LONG_Value;
char PASSWORD_ADMIN_Value[5];
char PASSWORD_USER_Value[5];
uint16_t DOOR_Value;
int8_t UTC_OFF_Value;
uint8_t profile_user_Value;
uint8_t profile_admin_Value;
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
		const unsigned char *font, uint8_t align, unsigned char text_inv,
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
void create_menu(uint8_t selected, uint8_t page, uint8_t clear,
		bounding_box_t *text_pos) {

	if (clear)
		glcd_blank();
	bounding_box_t pos_[2];

	create_cell(0, 0, 128, 64, 1, 2, 1, pos_);
	for (uint8_t op = 0;
			op < MENU_ITEMS_IN_PAGE
					&& (op + MENU_ITEMS_IN_PAGE * page) < MENU_TOTAL_ITEMS;
			op++) {

		draw_text(menu[op + MENU_ITEMS_IN_PAGE * page], (op / 5) * 66 + 2,
				(op % 5) * 12 + 1, Tahoma8, 1, 0);
		text_pos[op].x1 = (op / 5) * 66 + 2;
		text_pos[op].y1 = (op % 5) * 12 + 1;
		text_pos[op].x2 = text_pos[op].x1
				+ text_width(menu[op + MENU_ITEMS_IN_PAGE * page], Tahoma8, 1);
		text_pos[op].y2 = text_pos[op].y1
				+ text_height(menu[op + MENU_ITEMS_IN_PAGE * page], Tahoma8);
		glcd_refresh();
	}
//	selected=selected%MENU_ITEMS_IN_PAGE;
	draw_text(menu[selected+MENU_ITEMS_IN_PAGE*page],
			(selected / 5) * 66 + 2, (selected % 5) * 12 + 1, Tahoma8, 1, 1);

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
	bounding_box_t pos_[8];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	//////////////////////////////////////////////////////////////////////////
	create_cell(pos_[0].x1, pos_[0].y2, 128, pos_[0].y2 - pos_[0].y1 + 1, 1, 2,
			1, pos_);

	pos_[0].x2 = pos_[0].x1 + 22;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 1, 1);
	(TEC_STATE_Value == 1) ? sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
	pos_[0].x1 += 22;
	pos_[0].x2 = 63;
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	pos_[1].x2 = pos_[1].x1 + 22;
	text_cell(pos_, 1, "Hys:", Tahoma8, LEFT_ALIGN, 1, 1);
	sprintf(tmp_str, "%3.1f'C", HYSTERESIS_Value / 10.0);
	pos_[1].x1 += 22;
	pos_[1].x2 = 127;
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 1, 1, pos_);
	pos_[0].x2 = pos_[0].x1 + 56;
	text_cell(pos_, 0, "Enviroment:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[1].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 1, "Camera:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 2, "Case:", Tahoma8, LEFT_ALIGN, 1, 1);

	if (TempLimit_Value[0].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[0].TemperatureH / 10.0,
				TempLimit_Value[0].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = 127;
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (TempLimit_Value[1].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[1].TemperatureH / 10.0,
				TempLimit_Value[1].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = 127;
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (TempLimit_Value[2].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[2].TemperatureH / 10.0,
				TempLimit_Value[2].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = 127;
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_form5_5(uint8_t clear) {
	char tmp_str[40];
	bounding_box_t pos_[8];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clock_cell(pos_);
	//////////////////////////////////////////////////////////////////////////
	create_cell(pos_[0].x1, pos_[0].y2, 128, pos_[0].y2 - pos_[0].y1 + 1, 1, 2,
			1, pos_);

	pos_[0].x2 = pos_[0].x1 + 22;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 1, 1);
	(TEC_STATE_Value == 1) ? sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
	pos_[0].x1 += 22;
	pos_[0].x2 = 63;
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	pos_[1].x2 = pos_[1].x1 + 22;
	text_cell(pos_, 1, "Hys:", Tahoma8, LEFT_ALIGN, 1, 1);
	sprintf(tmp_str, "%3.1f'C", HYSTERESIS_Value / 10.0);
	pos_[1].x1 += 22;
	pos_[1].x2 = 127;
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 1, 1, pos_);
	pos_[0].x2 = pos_[0].x1 + 56;
	text_cell(pos_, 0, "M Board:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[1].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 1, "TEC In:", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[2].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 2, "TEC Out:", Tahoma8, LEFT_ALIGN, 1, 1);

	if (TempLimit_Value[3].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[3].TemperatureH / 10.0,
				TempLimit_Value[3].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[0].x1 = pos_[0].x2 + 1;
	pos_[0].x2 = 127;
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (TempLimit_Value[4].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[4].TemperatureH / 10.0,
				TempLimit_Value[4].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[1].x1 = pos_[1].x2 + 1;
	pos_[1].x2 = 127;
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	if (TempLimit_Value[5].active)
		sprintf(tmp_str, "%4.1f/%4.1f", TempLimit_Value[5].TemperatureH / 10.0,
				TempLimit_Value[5].TemperatureL / 10.0);
	else
		sprintf(tmp_str, "-/-");
	pos_[2].x1 = pos_[2].x2 + 1;
	pos_[2].x2 = 127;
	text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
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
	text_cell(pos_, 0, "Door:", Tahoma8, LEFT_ALIGN, 1, 1);

	if (outside_light != 0xffff)
		sprintf(tmp_str, "%05d", outside_light);
	else
		sprintf(tmp_str, "-----");
	pos_[1].x1 = 90;
	text_cell(pos_, 1, tmp_str, Tahoma8, LEFT_ALIGN, 0, 0);
	pos_[1].x1 = 1;
	pos_[1].x2 = 70;
	text_cell(pos_, 1, "Ambient Light:", Tahoma8, LEFT_ALIGN, 1, 1);
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
void create_formpass(uint8_t clear, uint8_t profile, bounding_box_t *text_pos) {
	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
//	////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
//	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);
//	pos_[0].x2 = 57;
//	text_cell(pos_, 0, "PASSWORD", Tahoma8, LEFT_ALIGN, 1, 1);
//	pos_[0].x1 = 65;
//	pos_[0].x2 = pos_[0].x1 + 20;
//	text_pos[0] = create_button(pos_[0], "OK", 0, 0);
//	pos_[0].x1 = pos_[0].x2;
//	pos_[0].x2 = pos_[0].x1 + 42;
//	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
//	////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 1, 1, 1, pos_);
//	for (uint8_t i = 0; i < 4; i++) {
//		pos_[0].x1 = i * 32;
//		pos_[0].x2 = (i + 1) * 32 - 1;
//		text_pos[i + 2] = text_cell(pos_, 0, "*", Tahoma16, CENTER_ALIGN, 0, 0);
//	}
//
//	glcd_refresh();
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "PASSWORD", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 40, 64 - pos_[0].y2, 2, 1, 1, pos_);
	pos_[0].x1 = 0;
	pos_[0].x2 = 48;
	text_cell(pos_, 0, "User:", Tahoma8, LEFT_ALIGN, 1, 1);
	pos_[1].x1 = 0;
	pos_[1].x2 = 48;
	text_cell(pos_, 1, "Password:", Tahoma8, LEFT_ALIGN, 1, 1);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(40, pos_[0].y1, 128 - 40, 64 - pos_[0].y1, 2, 1, 1, pos_);
	glcd_refresh();
	pos_[0].x1 = 52;
	pos_[0].x2 = pos_[0].x1 + text_width("ADMIN", Tahoma8, 1) + 1;
	if (profile == USER_PROFILE)
		sprintf(tmp_str, "USER");
	else
		sprintf(tmp_str, "ADMIN");
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	pos_[1].x1 = 52;
	for (uint8_t i = 0; i < 4; i++) {
		pos_[1].x2 = pos_[1].x1 + text_width("W", Tahoma8, 1) + 1;
		text_pos[i + 3] = text_cell(pos_, 1, "*", Tahoma8, CENTER_ALIGN, 0, 0);
		pos_[1].x1 += 15;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
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

void create_formTemp1(uint8_t clear, bounding_box_t *text_pos,
		RELAY_t *tmp_limits, uint8_t tmp_tec, uint8_t tmp_hys) {
	char tmp_str[40];
	bounding_box_t pos_[3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "Temp SET1", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
//	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, pos_[0].y2 - pos_[0].y1 + 1, 1, 2, 1, pos_);

	pos_[0].x1 += 1;
	pos_[0].x2 = pos_[0].x1 + 22;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 0, 0);

	(tmp_tec == 1) ? sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
	pos_[0].x1 += 35;
	pos_[0].x2 = pos_[0].x1 + text_width("ENA", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	pos_[1].x1 += 1;
	pos_[1].x2 = pos_[1].x1 + 22;
	text_cell(pos_, 1, "Hys:", Tahoma8, LEFT_ALIGN, 0, 0);
	sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
	pos_[1].x1 += 35;
	pos_[1].x2 = pos_[1].x1 + text_width("5.5'C", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 1, 1, pos_);
	pos_[0].x1 += 1;
	pos_[0].x2 = pos_[0].x1 + 56;
	text_cell(pos_, 0, "Enviroment:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[1].x1 += 1;
	pos_[1].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 1, "Camera:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[2].x1 += 1;
	pos_[2].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 2, "Case:", Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[ENVIROMENT_TEMP].TemperatureH / 10.0);
	pos_[0].x1 = pos_[0].x2 + 5;
	pos_[0].x2 = pos_[0].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[ENVIROMENT_TEMP].TemperatureL / 10.0);
	pos_[0].x1 = pos_[0].x2 + 10;
	pos_[0].x2 = pos_[0].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	if (tmp_limits[ENVIROMENT_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[0].x1 = pos_[0].x2 + 3;
	pos_[0].x2 = pos_[0].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[CAM_TEMP].TemperatureH / 10.0);
	pos_[1].x1 = pos_[1].x2 + 5;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[CAM_TEMP].TemperatureL / 10.0);
	pos_[1].x1 = pos_[1].x2 + 10;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	if (tmp_limits[CAM_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[1].x1 = pos_[1].x2 + 3;
	pos_[1].x2 = pos_[1].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[CASE_TEMP].TemperatureH / 10.0);
	pos_[2].x1 = pos_[2].x2 + 5;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[10] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[CASE_TEMP].TemperatureL / 10.0);
	pos_[2].x1 = pos_[2].x2 + 10;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[11] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	if (tmp_limits[CASE_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[2].x1 = pos_[2].x2 + 3;
	pos_[2].x2 = pos_[2].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[12] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	glcd_refresh();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void create_formTemp2(uint8_t clear, bounding_box_t *text_pos,
		RELAY_t *tmp_limits, uint8_t tmp_tec, uint8_t tmp_hys) {
	char tmp_str[40];
	bounding_box_t pos_[3];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "Temp SET2", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);
//	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, pos_[0].y2 - pos_[0].y1 + 1, 1, 2, 1, pos_);

	pos_[0].x1 += 1;
	pos_[0].x2 = pos_[0].x1 + 22;
	text_cell(pos_, 0, "TEC:", Tahoma8, LEFT_ALIGN, 0, 0);

	(tmp_tec == 1) ? sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
	pos_[0].x1 += 28;
	pos_[0].x2 = pos_[0].x1 + text_width("ENA", Tahoma8, 1) + 1;
	text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	pos_[1].x1 += 1;
	pos_[1].x2 = pos_[1].x1 + 22;
	text_cell(pos_, 1, "Hys:", Tahoma8, LEFT_ALIGN, 0, 0);
	sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
	pos_[1].x1 += 28;
	pos_[1].x2 = pos_[1].x1 + text_width("5.5", Tahoma8, 1) + 1;
	text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 3, 1, 1, pos_);
	pos_[0].x1 += 1;
	pos_[0].x2 = pos_[0].x1 + 56;
	text_cell(pos_, 0, "M Board:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[1].x1 += 1;
	pos_[1].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 1, "TEC In:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[2].x1 += 1;
	pos_[2].x2 = pos_[1].x1 + 56;
	text_cell(pos_, 2, "TEC Out:", Tahoma8, LEFT_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[MOTHERBOARD_TEMP].TemperatureH / 10.0);
	pos_[0].x1 = pos_[0].x2 + 5;
	pos_[0].x2 = pos_[0].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[MOTHERBOARD_TEMP].TemperatureL / 10.0);
	pos_[0].x1 = pos_[0].x2 + 10;
	pos_[0].x2 = pos_[0].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	if (tmp_limits[MOTHERBOARD_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[0].x1 = pos_[0].x2 + 3;
	pos_[0].x2 = pos_[0].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[TECIN_TEMP].TemperatureH / 10.0);
	pos_[1].x1 = pos_[1].x2 + 5;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[TECIN_TEMP].TemperatureL / 10.0);
	pos_[1].x1 = pos_[1].x2 + 10;
	pos_[1].x2 = pos_[1].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();
	if (tmp_limits[TECIN_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[1].x1 = pos_[1].x2 + 3;
	pos_[1].x2 = pos_[1].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	/////////////////////////////////////////////////////////////////////////////////////////
	sprintf(tmp_str, "%4.1f", tmp_limits[TECOUT_TEMP].TemperatureH / 10.0);
	pos_[2].x1 = pos_[2].x2 + 5;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[8] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	sprintf(tmp_str, "%4.1f", tmp_limits[TECOUT_TEMP].TemperatureL / 10.0);
	pos_[2].x1 = pos_[2].x2 + 10;
	pos_[2].x2 = pos_[2].x1 + text_width("55.5", Tahoma8, 1) + 1;
	text_pos[9] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	if (tmp_limits[TECOUT_TEMP].active)
		sprintf(tmp_str, "A");
	else
		sprintf(tmp_str, "-");

	pos_[2].x1 = pos_[2].x2 + 3;
	pos_[2].x2 = pos_[2].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[10] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);

	//	/////////////////////////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////
void create_formUseroption(uint8_t clear, uint8_t user,
		bounding_box_t *text_pos) {
	char tmp_str[40];
	bounding_box_t pos_[8];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "User Option", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 128, 64 - pos_[0].y2, 4, 2, 1, pos_);

	pos_[0].x1 += 1;
	pos_[0].x2 = pos_[0].x1 + 37;
	text_cell(pos_, 0, "LED:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[1].x1 += 1;
	pos_[1].x2 = pos_[1].x1 + 49;
	text_cell(pos_, 1, "Time&Pos:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[2].x1 += 1;
	pos_[2].x2 = pos_[2].x1 + 37;
	text_cell(pos_, 2, "Temp:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[3].x1 += 1;
	pos_[3].x2 = pos_[3].x1 + 49;
	text_cell(pos_, 3, "User opt:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[4].x1 += 1;
	pos_[4].x2 = pos_[4].x1 + 37;
	text_cell(pos_, 4, "Wifi:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[5].x1 += 1;
	pos_[5].x2 = pos_[5].x1 + 49;
	text_cell(pos_, 5, "Upgrade:", Tahoma8, LEFT_ALIGN, 0, 0);

	pos_[6].x1 += 1;
	pos_[6].x2 = pos_[6].x1 + 37;
	text_cell(pos_, 6, "Pass:", Tahoma8, LEFT_ALIGN, 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	(user & 0x02) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[0].x1 = pos_[0].x2 + 5;
	pos_[0].x2 = pos_[0].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[2] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x04) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[1].x1 = pos_[1].x2 + 5;
	pos_[1].x2 = pos_[1].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[3] = text_cell(pos_, 1, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x08) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[2].x1 = pos_[2].x2 + 5;
	pos_[2].x2 = pos_[2].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[4] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x10) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[3].x1 = pos_[3].x2 + 5;
	pos_[3].x2 = pos_[3].x1 + text_width("A", Tahoma8, 1) + 1;
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x20) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[4].x1 = pos_[4].x2 + 5;
	pos_[4].x2 = pos_[4].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[5] = text_cell(pos_, 4, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x40) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[5].x1 = pos_[5].x2 + 5;
	pos_[5].x2 = pos_[5].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[6] = text_cell(pos_, 5, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	(user & 0x80) ? sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
	pos_[6].x1 = pos_[6].x2 + 5;
	pos_[6].x2 = pos_[6].x1 + text_width("A", Tahoma8, 1) + 1;
	text_pos[7] = text_cell(pos_, 6, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glcd_refresh();
}
/////////////////////////////////////////////////////////////////////////
void create_formWIFI1(uint8_t clear, WiFi_t tmp_wifi, bounding_box_t *text_pos) {

	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x1 = 1;
	pos_[0].x2 = 60;
	text_cell(pos_, 0, "WiFi 1", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 25, 64 - pos_[0].y2, 4, 1, 1, pos_);
	glcd_refresh();

	pos_[0].x1 = 1;
	pos_[0].x2 = 24;
	text_cell(pos_, 0, "ssid", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[1].x1 = 1;
	pos_[1].x2 = 24;
	text_cell(pos_, 1, "Pass", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[2].x1 = 1;
	pos_[2].x2 = 24;
	text_cell(pos_, 2, "Hide", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[3].x1 = 1;
	pos_[3].x2 = 24;
	text_cell(pos_, 3, "Clnt", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(24, pos_[0].y1, 128 - 24, 64 - pos_[0].y1, 4, 1, 1, pos_);
	glcd_refresh();
	pos_[0].x1 = 27;
	for (uint8_t index = 0; index < 10; index++) {
		sprintf(tmp_str, "%c", tmp_wifi.ssid[index]);
		pos_[0].x1 = pos_[0].x1;
		pos_[0].x2 = pos_[0].x1 + text_width("W", Tahoma8, 1);
		text_pos[2 + index] = text_cell(pos_, 0, tmp_str, Tahoma8, CENTER_ALIGN,0, 0);
		glcd_refresh();
		pos_[0].x1 += text_width("W", Tahoma8, 1) + 1;
//		index++;
	}
	pos_[1].x1 = 27;
	for (uint8_t index = 0; index < 10; index++) {
		sprintf(tmp_str, "%c", tmp_wifi.pass[index]);
		pos_[1].x1 = pos_[1].x1;
		pos_[1].x2 = pos_[1].x1 + text_width("W", Tahoma8, 1) ;
		text_pos[12 + index] = text_cell(pos_, 1, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
		glcd_refresh();
		pos_[1].x1 += text_width("W", Tahoma8, 1) + 1;
//		index++;
	}

	(tmp_wifi.ssidhidden) ? sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
	pos_[2].x1 = pos_[2].x1 + 2;
	pos_[2].x2 = pos_[2].x2 - 1;
	text_pos[22] = text_cell(pos_, 2, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	sprintf(tmp_str, "%d", tmp_wifi.maxclients);
	pos_[3].x1 = pos_[3].x1 + 2;
	pos_[3].x2 = pos_[3].x2 - 1;
//	pos_[3].x2 = pos_[3].x1 + text_width("5", Tahoma8, 1) + 1;
	text_pos[23] = text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}

/////////////////////////////////////////////////////////////////////////
void create_formWIFI2(uint8_t clear, WiFi_t tmp_wifi, bounding_box_t *text_pos) {

	char tmp_str[40];
	bounding_box_t pos_[4];
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (clear)
		glcd_blank();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, 0, 128, 13, 1, 1, 1, pos_);

	pos_[0].x2 = 60;
	text_cell(pos_, 0, "WiFi 2", Tahoma8, LEFT_ALIGN, 1, 1);

	pos_[0].x1 = 65;
	pos_[0].x2 = pos_[0].x1 + 20;
	text_pos[0] = create_button(pos_[0], "OK", 0, 0);

	pos_[0].x1 = pos_[0].x2;
	pos_[0].x2 = pos_[0].x1 + 42;
	text_pos[1] = create_button(pos_[0], "CANCEL", 0, 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y2, 30, 64 - pos_[0].y2, 4, 1, 1, pos_);
	glcd_refresh();

	pos_[0].x1 = 1;
	pos_[0].x2 = 29;
	text_cell(pos_, 0, "IP", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[1].x1 = 1;
	pos_[1].x2 = 29;
	text_cell(pos_, 1, "Gate", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[2].x1 = 1;
	pos_[2].x2 = 29;
	text_cell(pos_, 2, "Mask", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	pos_[3].x1 = 0;
	pos_[3].x2 = 29;
	text_cell(pos_, 3, "PWR", Tahoma8, LEFT_ALIGN, 0, 0);
	glcd_refresh();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(29, pos_[0].y1, 128 - 29, 64 - pos_[0].y1, 4, 1, 1, pos_);

	pos_[0].x1 = 32;
	for (uint8_t index = 0; index < 15; index++) {
		sprintf(tmp_str, "%c", tmp_wifi.ip[index]);
		pos_[0].x1 = pos_[0].x1 ;
		pos_[0].x2 = pos_[0].x1 + text_width("5", Tahoma8, 1) ;
		text_pos[2 + index] = text_cell(pos_, 0, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
		pos_[0].x1 += text_width("5", Tahoma8, 1)+1;
		glcd_refresh();
	}
	pos_[1].x1 = 32;
	for (uint8_t index = 0; index < 15; index++) {
		sprintf(tmp_str, "%c", tmp_wifi.gateway[index]);
		pos_[1].x1 = pos_[1].x1;
		pos_[1].x2 = pos_[1].x1 + text_width("5", Tahoma8, 1) ;
		text_pos[17 + index] = text_cell(pos_, 1, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
		pos_[1].x1 += text_width("5", Tahoma8, 1)+1;
		glcd_refresh();
	}

	pos_[2].x1 = 32;
	for (uint8_t index = 0; index < 15; index++) {
		sprintf(tmp_str, "%c", tmp_wifi.netmask[index]);
		pos_[2].x1 = pos_[2].x1;
		pos_[2].x2 = pos_[2].x1 + text_width("5", Tahoma8, 1) ;
		text_pos[32 + index] = text_cell(pos_, 2, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
		pos_[2].x1 += text_width("5", Tahoma8, 1)+1;
		glcd_refresh();
	}

	sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
	pos_[3].x1 = pos_[3].x1 + 2;
	pos_[3].x2 = pos_[3].x2 - 1;
	text_pos[47] = text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	glcd_refresh();

	/////////////////////////////////////////////////////////////////////////////////////////////////

	glcd_refresh();
}

/////////////////////////////////////////////////////////////////////////

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
////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t find_profile(uint8_t *frame) {
	char extracted_text[100];
	uint8_t cur_browser_profile;
	uint16_t size_msg=((uint16_t) frame[4]<<8)+frame[5];
	strncpy(extracted_text, (char*) &frame[6], size_msg);
	extracted_text[size_msg] = '\0';
//	printf("msg from esp:%s\n\r",extracted_text);
	char *ptr_splitted = strtok(extracted_text, ",");
	if (ptr_splitted != NULL) {
		cur_browser_profile = atoi(ptr_splitted);
	}
	return cur_browser_profile;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ready_msg_Dashboardpage(char *str2, uint16_t cur_outsidelight,
		uint8_t cur_doorstate, uint8_t cur_client_number, double *cur_voltage,
		double *cur_current, uint8_t cur_browser_profile) {
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	HAL_RTC_GetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cur_Date, RTC_FORMAT_BIN);
	change_daylightsaving(&cur_Date, &cur_time, 1);

	sprintf(str2, "%02d:%02d", cur_time.Hours, cur_time.Minutes);
	sprintf(str2, "%s,%04d-%02d-%02d", str2, cur_Date.Year + 2000,
			cur_Date.Month, cur_Date.Date);
	sprintf(str2, "%s,%d", str2, cur_outsidelight);
	if (cur_doorstate) {
		sprintf(str2, "%s,on", str2);
	} else
		sprintf(str2, "%s,off", str2);

	sprintf(str2, "%s,%d %d\' %05.2f\"%c,%d %d\' %05.2f\"%c", str2,
			LONG_Value.deg, LONG_Value.min, LONG_Value.second / 100.0,
			LONG_Value.direction, LAT_Value.deg, LAT_Value.min,
			LAT_Value.second / 100.0, LAT_Value.direction);

	sprintf(str2, "%s,%d", str2, cur_client_number);
	for (uint8_t i = 0; i < 4; i++) {
		if (cur_voltage[i] > 0) {
			sprintf(str2, "%s,%3.1f,%+4.3f", str2, cur_voltage[i],
					cur_current[i]);
		} else {
			sprintf(str2, "%s,-,-", str2);
		}
	}
	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%s,%d", str2, profile_admin_Value);
	else
		sprintf(str2, "%s,%d", str2, profile_user_Value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void ready_msg_LEDpage(char *str2, LED_t LED_Value, uint8_t cur_browser_profile) {
	sprintf(str2, "%d", LED_Value.TYPE_Value);
	sprintf(str2, "%s,%d", str2, LED_Value.DAY_BRIGHTNESS_Value);

	if (LED_Value.TYPE_Value == WHITE_LED)
		sprintf(str2, "%s,%2.1f", str2, LED_Value.DAY_BLINK_Value / 10.0);
	else
		sprintf(str2, "%s,-", str2);

	sprintf(str2, "%s,%d", str2, LED_Value.NIGHT_BRIGHTNESS_Value);
	if (LED_Value.TYPE_Value == WHITE_LED)
		sprintf(str2, "%s,%2.1f", str2, LED_Value.NIGHT_BLINK_Value / 10.0);
	else
		sprintf(str2, "%s,-", str2);

	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%s,%d", str2, profile_admin_Value);
	else
		sprintf(str2, "%s,%d", str2, profile_user_Value);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////
void ready_msg_TimePositionpage(char *str2, Time_t sunrise, Time_t sunset,
		uint8_t cur_browser_profile) {
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	HAL_RTC_GetTime(&hrtc, &cur_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cur_Date, RTC_FORMAT_BIN);
	change_daylightsaving(&cur_Date, &cur_time, 1);

	sprintf(str2, "%04d-%02d-%02d", cur_Date.Year + 2000, cur_Date.Month,
			cur_Date.Date);
	sprintf(str2, "%s,%02d:%02d", str2, cur_time.Hours, cur_time.Minutes);
	sprintf(str2, "%s,%02d", str2, LAT_Value.deg);
	sprintf(str2, "%s,%02d", str2, LAT_Value.min);
	sprintf(str2, "%s,%.2f", str2, LAT_Value.second / 100.0);
	sprintf(str2, "%s,%c", str2, LAT_Value.direction);
	sprintf(str2, "%s,%02d", str2, LONG_Value.deg);
	sprintf(str2, "%s,%02d", str2, LONG_Value.min);
	sprintf(str2, "%s,%.2f", str2, LONG_Value.second / 100.0);
	sprintf(str2, "%s,%c", str2, LONG_Value.direction);
	sprintf(str2, "%s,%.1f", str2, UTC_OFF_Value / 10.0);
	sprintf(str2, "%s,%02d:%02d", str2, sunrise.hr, sunrise.min);
	sprintf(str2, "%s,%02d:%02d", str2, sunset.hr, sunset.min);
	sprintf(str2, "%s,%.1f", str2, S1_LED_Value.ADD_SUNRISE_Value / 10.0);
	sprintf(str2, "%s,%.1f", str2, S1_LED_Value.ADD_SUNSET_Value / 10.0);

	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%s,%d", str2, profile_admin_Value);
	else
		sprintf(str2, "%s,%d", str2, profile_user_Value);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void ready_msg_Temperaturepage(char *str2, RELAY_t *TempLimit, uint8_t hys,
		uint8_t tec, uint8_t cur_browser_profile) {
	sprintf(str2, "%d", tec);
	sprintf(str2, "%s,%3.1f", str2, hys / 10.0);

	sprintf(str2, "%s,%4.1f", str2,
			TempLimit[ENVIROMENT_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2,
			TempLimit[ENVIROMENT_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[ENVIROMENT_TEMP].active);

	sprintf(str2, "%s,%4.1f", str2, TempLimit[CAM_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2, TempLimit[CAM_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[CAM_TEMP].active);

	sprintf(str2, "%s,%4.1f", str2, TempLimit[CASE_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2, TempLimit[CASE_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[CASE_TEMP].active);

	sprintf(str2, "%s,%4.1f", str2,
			TempLimit[MOTHERBOARD_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2,
			TempLimit[MOTHERBOARD_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[MOTHERBOARD_TEMP].active);

	sprintf(str2, "%s,%4.1f", str2, TempLimit[TECIN_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2, TempLimit[TECIN_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[TECIN_TEMP].active);

	sprintf(str2, "%s,%4.1f", str2, TempLimit[TECOUT_TEMP].TemperatureH / 10.0);
	sprintf(str2, "%s,%4.1f", str2, TempLimit[TECOUT_TEMP].TemperatureL / 10.0);
	sprintf(str2, "%s,%d", str2, TempLimit[TECOUT_TEMP].active);

	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%s,%d", str2, profile_admin_Value);
	else
		sprintf(str2, "%s,%d", str2, profile_user_Value);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void ready_msg_Useroptionpage(char *str2, uint8_t user,
		uint8_t cur_browser_profile) {
//	sprintf(str2,"%d",user&0x01);
	sprintf(str2, "%d", (user & 0x02) >> 1);
	sprintf(str2, "%s,%d", str2, (user & 0x04) >> 2);
	sprintf(str2, "%s,%d", str2, (user & 0x08) >> 3);
	sprintf(str2, "%s,%d", str2, (user & 0x10) >> 4);
	sprintf(str2, "%s,%d", str2, (user & 0x20) >> 5);
	sprintf(str2, "%s,%d", str2, (user & 0x40) >> 6);
	sprintf(str2, "%s,%d", str2, (user & 0x80) >> 7);

	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%s,%d", str2, profile_admin_Value);
	else
		sprintf(str2, "%s,%d", str2, profile_user_Value);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

void ready_msg_Wifipage(char *str2, uint8_t cur_browser_profile) {
	if (cur_browser_profile == 0)	//admin
		sprintf(str2, "%d", profile_admin_Value);
	else
		sprintf(str2, "%d", profile_user_Value);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void split_time(char *str, RTC_TimeTypeDef *t) {
	printf("time:%s\n\r", str);
	char *ptr_splitted = strtok(str, ":");
	t->Hours = atoi(ptr_splitted);
	ptr_splitted = strtok(NULL, ":");
	t->Minutes = atoi(ptr_splitted);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void split_date(char *str, RTC_DateTypeDef *d) {
	printf("date:%s,", str);
	char *ptr_splitted = strtok(str, "-");
	d->Year = atoi(ptr_splitted) - 2000;
	ptr_splitted = strtok(NULL, "-");
	d->Month = atoi(ptr_splitted);
	ptr_splitted = strtok(NULL, "-");
	d->Date = atoi(ptr_splitted);
	printf("%04d-%02d-%02d\n\r", d->Year, d->Month, d->Date);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int app_main(void)
{
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
  MX_USART3_UART_Init();
  MX_USB_HOST_Init();
  MX_LWIP_Init();
#if !(__DEBUG__)
  MX_IWDG_Init();
#endif
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	/////////////////////////
	uint8_t transmit_value[10];
  	uint8_t flag_change_form = 0;
	uint8_t counter_change_form = 0;
	uint8_t flag_log_data = 0;
	uint8_t counter_log_data = 0;
	uint16_t tmp_uint16;

	char tmp_str[100], tmp_str1[100], tmp_str2[120];
	char extracted_text[120]={0};
	uint8_t *extracted_data;
//	uint8_t *received_frame;

	char *ptr_splitted[25];
	uint8_t index_ptr;
	FRESULT fr;

	uint8_t index_option = 0;
	char cur_pass[7];
	char tmp_pass[7] = { '*', '*', '*', '*', '*', '*', 0 };
	char tmp_confirmpass[7] = { '*', '*', '*', '*', '*', '*', 0 };
	POS_t tmp_lat, tmp_long;
	int16_t tmp_utcoff;
	double tmp_dlat, tmp_dlong;
	RTC_TimeTypeDef tmp_time;
	RTC_DateTypeDef tmp_Date;
	LED_t tmp_LED;
	RELAY_t tmp_limits[6];
	uint16_t tmp_door;
	WiFi_t tmp_wifi;
	uint8_t tmp_TEC_STATE, tmp_hys, tmp_profile_user;
	bounding_box_t pos_[15];
	bounding_box_t text_pos[50];
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
	uint8_t menu_page = 0;
	uint8_t cur_profile = ADMIN_PROFILE;
	uint8_t cur_browser_profile = ADMIN_PROFILE;

	uint8_t algorithm_temp_state = 0;
	int16_t Env_temperature, prev_Env_temperature[3]={0}, Delta_T_Env,env_index_prv=0;
	int16_t Cam_temperature, prev_cam_temperature[3]={0}, Delta_T_cam,cam_index_prev=0;
	int16_t Tecin_temperature, prev_Tecin_temperature, Delta_T_Tecin;
	uint8_t second_algorithm_temperature=0;
	uint8_t flag_rtc_10s_general = 0;
	uint8_t counter_flag_10s_general = 0;
	uint8_t flag_rtc_1m_general = 0;
	uint8_t counter_flag_1m_general = 0;
	uint8_t cur_client_number;
	char version_chars[10]={0};
	char filename_chars[100]={0};
	char readline[100];
	char firmware_path[100];
	uint16_t firmware_version;
	uint32_t firmware_checksum;
	uint8_t upload_started=0;
	uint32_t byteswritten;
	FIL myfile;
	//////////////////////retarget////////////////
	//RetargetInit(&huart3);
	/////////////////////////transceiver PC<->ESP32/////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	printf(
			"\033[0;31;49m\n\r***********************************************************\n\r\033[0;39;49m");
	printf(
			"\033[0;31;49m*                  ACAM Application                       *\n\r\033[0;39;49m");
	printf(
			"\033[0;31;49m***********************************************************\n\r\033[0;39;49m");
	///////////////////////////////////////////////////////////////////////////////
	HAL_GPIO_WritePin(ESP32_EN_GPIO_Port, ESP32_EN_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(ESP32_EN_GPIO_Port, ESP32_EN_Pin, GPIO_PIN_SET); //enable esp32
	////////////////////////////////////////////////////////////////
//	HAL_UART_Receive_IT(&huart2, &received_byte, 1);
	HAL_UART_Receive_IT(&huart3, &received_byte_MB, 1);
//	uart_transmit_frame_MB(&huart3,2,uint8_t type,uint8_t channel, uint8_t *data)
//	HAL_UART_Transmit(&huart3, "Hello\n\r", 7, 3000);
//	while(1)
//	{
//		if(ESP_validbuffer)
//		{
//			ESP_validbuffer=0;
//			printf("\n\rBuffre_ESP=");
//			for(uint8_t i=0;i<ESP_buffer[2]+4;i++)
//				printf("%d,",ESP_buffer[i]);
//			printf("\n\r");
//			HAL_UART_Transmit(&huart2, ESP_buffer, ESP_buffer[2]+4, 1000);
//		}
//	}

	/////////////////////Turn power off & on USB flash///////////////
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_SET); //disable
	HAL_Delay(500);
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_RESET); //enable
	//////////////////////init LCD//////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);
	////////////////////////////////////////////////
//	create_form5(1);
//	create_form5_5(1);

//	//////////////////////////load Logo/////////////////////////////////////////
#if !(__DEBUG__)
	HAL_IWDG_Refresh(&hiwdg);
#endif
	draw_bmp_h(0,0,aCAM_logo_128_02_H[0],aCAM_logo_128_02_H[2],&aCAM_logo_128_02_H[4],1);
	draw_box(0, 0, 127, 63, 1);
	glcd_refresh();
	HAL_Delay(2000);
#if 0
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
#endif
	glcd_blank();

	///////////////////////initialize & checking sensors///////////////////////////////////////
	create_cell(0, 0, 128, 64, 4, 2, 1, pos_);
	uint8_t ch, inv;
	for (ch = TMP_CH0; ch <= TMP_CH7; ch++) {
		if ((status = tmp275_init(ch)) != TMP275_OK) {
			printf("tmp275 sensor (#%d) error\n\r", ch + 1);
			sprintf(tmp_str1, "Temp(%d)ERR", ch + 1);
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
		text_cell(text_pos, 0, "Loading Defaults", Tahoma8, CENTER_ALIGN, 0, 0);
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
	uint8_t sendready_counter=0,sendready_tick=1;
	uint8_t ESP32ready=0;
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

			counter_flag_10s_general++;
			if (counter_flag_10s_general > 10) {
				counter_flag_10s_general = 0;
				flag_rtc_10s_general = 1;
			}

			counter_flag_1m_general++;
			if (counter_flag_1m_general >= 60) {
				counter_flag_1m_general = 0;
				flag_rtc_1m_general = 1;
			}
			sendready_counter++;
			if(sendready_counter>=2)
			{
				sendready_counter=0;
				sendready_tick=1;
			}
			/////////////////////read sensors evey 1 s//////////////////////////
			for (uint8_t i = 0; i < 8; i++) {
				if (tmp275_readTemperature(i, &cur_temperature[i]) != HAL_OK) {
					cur_temperature[i] = (int16_t) 0x8fff;
				}
//				HAL_Delay(20);
			}
			////////////////////////////////////////////////////////////
			if (ina3221_readdouble((uint8_t) VOLTAGE_7V, &cur_voltage[0])!= HAL_OK) {
				cur_voltage[0] = -1.0;
				reinit_i2c(&hi2c3);
			}
			if (ina3221_readdouble((uint8_t) CURRENT_7V, &cur_current[0])!= HAL_OK) {
				cur_current[0] = -1.0;
				reinit_i2c(&hi2c3);
			}
			if (ina3221_readdouble((uint8_t) VOLTAGE_12V, &cur_voltage[1])!= HAL_OK) {
				cur_voltage[1] = -1.0;
				reinit_i2c(&hi2c3);

			}
			if (ina3221_readdouble((uint8_t) CURRENT_12V, &cur_current[1])!= HAL_OK) {
				cur_current[1] = -1.0;
				reinit_i2c(&hi2c3);

			}
			if (ina3221_readdouble((uint8_t) VOLTAGE_3V3, &cur_voltage[2])!= HAL_OK) {
				cur_voltage[2] = -1.0;
				reinit_i2c(&hi2c3);

			}
			if (ina3221_readdouble((uint8_t) CURRENT_3V3, &cur_current[2])!= HAL_OK) {
				cur_current[2] = -1.0;
				reinit_i2c(&hi2c3);

			}
			if (ina3221_readdouble((uint8_t) VOLTAGE_TEC, &cur_voltage[3])!= HAL_OK) {
				cur_voltage[3] = -1.0;
				reinit_i2c(&hi2c3);
			}
			if (ina3221_readdouble((uint8_t) CURRENT_TEC, &cur_current[3])!= HAL_OK) {
				cur_current[3] = -1.0;
				reinit_i2c(&hi2c3);
			}
			////////////////////////////////////////////////////////
			if (vcnl4200_ps(&cur_insidelight) != HAL_OK) {
				cur_insidelight = 0xffff;
				reinit_i2c(&hi2c3);
			}
			if (veml6030_als(&cur_outsidelight) != HAL_OK) {
				cur_outsidelight = 0xffff;
			}
		}
#if 0
		if (flag_rtc_1m_general) {
			flag_rtc_1m_general = 0;
			if (ActiveBrowserPage[DashboardActivePage]) {
				sprintf(tmp_str2, "%04d-%02d-%02d", cur_Date.Year + 2000,
						cur_Date.Month, cur_Date.Date);
				uart_transmit_frame(tmp_str2, cmd_event, DateEvent);
				sprintf(tmp_str2, "%02d:%02d", cur_time.Hours,
						cur_time.Minutes);
				uart_transmit_frame(tmp_str2, cmd_event, TimeEvent);
			}
		}
		/////////////////////send sensor data to ESP32:every 10s///////////////////////////

		if (flag_rtc_10s_general) {
			flag_rtc_10s_general = 0;
			////////////////Temperature//////////////////////////
			if (ActiveBrowserPage[DashboardActivePage]) {
				if (cur_temperature[0] == (int16_t) 0x8fff) {
					sprintf(tmp_str2, "-");
				} else {
					sprintf(tmp_str2, "%+4.1f", cur_temperature[0] / 10.0);
				}

				for (uint8_t i = 1; i < 8; i++) {
					if (cur_temperature[i] == (int16_t) 0x8fff) {
						sprintf(tmp_str2, "%s,-", tmp_str2);
					} else {
						sprintf(tmp_str2, "%s,%+4.1f", tmp_str2,
								cur_temperature[i] / 10.0);
					}
				}
				sprintf(tmp_str2, "%s,%02d:%02d:%02d", tmp_str2, cur_time.Hours,
						cur_time.Minutes, cur_time.Seconds);
				printf("to ESP32 Temp event:%s\n\r", tmp_str2);
				uart_transmit_frame(tmp_str2, cmd_event, TemperatureEvent);
			}
			//////////////////// memory//////////////////////
			DWORD fre_clust, fre_sect, tot_sect;
			FRESULT res;
			if (ActiveBrowserPage[DashboardActivePage]) {
				if (SDFatFS.fs_type
						== 0|| BSP_SD_IsDetected() == SD_NOT_PRESENT) {
					sprintf(tmp_str2, "-,-");
				} else {
					tot_sect = (SDFatFS.n_fatent - 2) * SDFatFS.csize;
					fre_sect = SDFatFS.free_clst * SDFatFS.csize;
					sprintf(tmp_str2, "%5lu,%5lu", (tot_sect - fre_sect) / 2048,
							fre_sect / 2048);
				}
				uart_transmit_frame(tmp_str2, cmd_event, SDCardEvent);
			}
			if (ActiveBrowserPage[DashboardActivePage]) {
				if (USBHFatFS.fs_type == 0) {

					sprintf(tmp_str2, "-,-");

				} else {
					tot_sect = (USBHFatFS.n_fatent - 2) * USBHFatFS.csize;
					fre_sect = USBHFatFS.free_clst * USBHFatFS.csize;
					sprintf(tmp_str2, "%5lu,%5lu", (tot_sect - fre_sect) / 2048,
							fre_sect / 2048);
				}
				uart_transmit_frame(tmp_str2, cmd_event, UsbMemEvent);
			}
			///////////////////////////Light/////////////////////////////
			if (ActiveBrowserPage[DashboardActivePage]) {
				sprintf(tmp_str2, "%d", cur_outsidelight);
				uart_transmit_frame(tmp_str2, cmd_event, LightoutsideEvent);
			}
			/////////////////////////Current Voltage//////////////////////
			if (ActiveBrowserPage[DashboardActivePage]) {
				sprintf(tmp_str2, "");
				for (uint8_t i = 0; i < 4; i++) {
					if (cur_voltage[i] > 0) {
						sprintf(tmp_str2, "%s%3.1f,%+4.3f,", tmp_str2,
								cur_voltage[i], cur_current[i]);
					} else {
						sprintf(tmp_str2, "%s-,-,", tmp_str2);
					}
				}
				uart_transmit_frame(tmp_str2, cmd_event, CVEvent);
			}
		}

		/////////////////////check door state & LOG//////////////////////////////////
		if (cur_insidelight > DOOR_Value) {
			cur_doorstate = 0;
		} else {
			cur_doorstate = 1;
		}
		if (cur_doorstate != prev_doorstate) {
			if (cur_doorstate) {
				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,Opened\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			} else {
				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,Closed\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
				sprintf(tmp_str2, "off");
			}
//			r_logdoor = Log_file(SDCARD_DRIVE, DOORSTATE_FILE, tmp_str2);
			if (ActiveBrowserPage[DashboardActivePage]) {
				if (cur_doorstate) {
					sprintf(tmp_str2, "on");

				} else {
					sprintf(tmp_str2, "off");

				}
				uart_transmit_frame(tmp_str2, cmd_event, DoorEvent);
			}
			prev_doorstate = cur_doorstate;
		}
#endif
		//////////////////////////Saves parameters in logs file in SDCARD,just one  time////
#if 0
		if (flag_log_param /*&& USBH_MSC_IsReady(&hUsbHostHS)*/) {
			flag_log_param = 0;
			sprintf(tmp_str2,
					"%04d-%02d-%02d,%02d:%02d:%02d,POSITION,%02d %02d' %05.2f\" %c,%02d %02d' %05.2f\" %c\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					LAT_Value.deg, LAT_Value.min, LAT_Value.second / 100.0,
					LAT_Value.direction, LONG_Value.deg, LONG_Value.min,
					LONG_Value.second / 100.0, LONG_Value.direction);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

			sprintf(tmp_str2,
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
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

			sprintf(tmp_str2,
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
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

			if (TEC_STATE_Value)
				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,TEC ENABLE\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			else
				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,TEC DISABLE\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

//			if (RELAY1_Value.active[0])
//				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%c%f",
//						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
//						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
//						RELAY1_Value.Edge[0],
//						RELAY1_Value.Temperature[0] / 10.0);
//			else
//				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,--",
//						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
//						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
//			if (RELAY1_Value.active[1])
//				sprintf(tmp_str2, "%s,%c%f\n", tmp_str, RELAY1_Value.Edge[1],
//						RELAY1_Value.Temperature[1] / 10.0);
//			else
//				sprintf(tmp_str2, "%s,--\n", tmp_str);
//			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);
//
//			if (RELAY2_Value.active[0])
//				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,%c%f",
//						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
//						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
//						RELAY2_Value.Edge[0],
//						RELAY2_Value.Temperature[0] / 10.0);
//			else
//				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,RELAY2,--",
//						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
//						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
//			if (RELAY2_Value.active[1])
//				sprintf(tmp_str, "%s,%c%f\n", tmp_str, RELAY2_Value.Edge[1],
//						RELAY2_Value.Temperature[1] / 10.0);
//			else
//				sprintf(tmp_str2, "%s,--\n", tmp_str);
//			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

			sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,Door,%d\n",
					cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
					cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
					DOOR_Value);
			r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);

		}
#endif
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
				////////////////////////////LED control////////////////////////////////////
#if 0
				if ((cur_time.Hours == 0 && cur_time.Minutes == 0&& cur_time.Seconds == 0) || (CalcAstro)) {
					CalcAstro = 0;
					tmp_dlat = POS2double(LAT_Value);
					tmp_dlong = POS2double(LONG_Value);
					Astro_sunRiseSet(tmp_dlat, tmp_dlong, UTC_OFF_Value / 10.0,
							cur_date_t, &cur_sunrise, &cur_noon, &cur_sunset,
							1);
					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						TimePositionPageEvent);
					}
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
#endif
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
				case DISP_FORM5_5:
					create_form5_5(0);
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
			/////////////////////Log Sensors//////////////////////////////////
#if 0
			if (flag_log_data) {
				flag_log_data = 0;
				counter_log_data = 0;

				/////////////////////log sensors//////////////////////////

				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
				for (uint8_t i = 0; i < 8; i++) {
					if (cur_temperature[i] == (int16_t) 0x8fff) {
						sprintf(tmp_str2, "%s-,", tmp_str2);
					} else {
						sprintf(tmp_str2, "%s%f,", tmp_str2,
								cur_temperature[i] / 10.0);
					}
				}
				sprintf(tmp_str2, "%s\n", tmp_str2);

				r_logtemp = Log_file(SDCARD_DRIVE, TEMPERATURE_FILE, tmp_str2);

				sprintf(tmp_str2,
						"%04d-%02d-%02d,%02d:%02d:%02d,%f,%f,%f,%f,%f,%f,%f,%f\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						cur_voltage[0], cur_current[0], cur_voltage[1],
						cur_current[1], cur_voltage[2], cur_current[2],
						cur_voltage[3], cur_current[3]);
				r_logvolt = Log_file(SDCARD_DRIVE, VOLTAMPERE_FILE, tmp_str2);

				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,%d,%d\n",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
						cur_insidelight, cur_outsidelight);

				r_loglight = Log_file(SDCARD_DRIVE, LIGHT_FILE, tmp_str2);
			}
#endif
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
					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						TimePositionPageEvent);
					}
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
					create_form5_5(1);
					DISP_state = DISP_FORM5_5;
					break;
				case DISP_FORM5_5:
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
				case DISP_FORM5_5:
					DISP_state = DISP_FORM4;
					break;
				case DISP_FORM6:
					DISP_state = DISP_FORM5;
					break;
				case DISP_FORM7:
					DISP_state = DISP_FORM5_5;
					break;
				}
			}
			if (joystick_read(Key_ENTER, Long_press)
					|| joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Both_press);
#if 0
				sprintf(tmp_pass, "0000");
				cur_profile = USER_PROFILE;
				create_formpass(1, cur_profile, text_pos);
				index_option = 2;
				if (cur_profile == USER_PROFILE)
					sprintf(tmp_str, "USER");
				else
					sprintf(tmp_str, "ADMIN");
				text_cell(text_pos, index_option, tmp_str, Tahoma8,
						CENTER_ALIGN, 1, 0);
				MENU_state = PASS_MENU;
				glcd_refresh();
#endif
			}
			break;
			/////////////////////////////////////PASS_MENU/////////////////////////////////////////////////
#if 0
		case PASS_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {

					if (index_option > 1) {
						draw_fill(text_pos[index_option].x1 - 1,
								text_pos[index_option].y1 + 1,
								text_pos[index_option].x2,
								text_pos[index_option].y2 - 1, 0);
					}
					switch (index_option) {
					case 0:	//OK
						break;
					case 1:	//CANCEL
						break;
					case 2:
						if (cur_profile == USER_PROFILE) {
							sprintf(tmp_str, "ADMIN");
							cur_profile = ADMIN_PROFILE;
						} else {
							sprintf(tmp_str, "USER");
							cur_profile = USER_PROFILE;
						}
						break;
					case 3:
					case 4:
					case 5:
					case 6:
						tmp_pass[index_option - 3] =
								(char) tmp_pass[index_option - 3] + 1;
						if (tmp_pass[index_option - 3] > '9'
								&& tmp_pass[index_option - 3] < 'A')
							tmp_pass[index_option - 3] = 'A';
						else if (tmp_pass[index_option - 3] > 'Z'
								&& tmp_pass[index_option - 3] < 'a')
							tmp_pass[index_option - 3] = 'a';
						if (tmp_pass[index_option - 3] > 'z')
							tmp_pass[index_option - 3] = '0';
						sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);
						break;
					}
					if (index_option > 1)
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);

					glcd_refresh();

				}

			}
			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					if (cur_profile == USER_PROFILE) {
						sprintf(tmp_str, "ADMIN");
						cur_profile = ADMIN_PROFILE;
					} else {
						sprintf(tmp_str, "USER");
						cur_profile = USER_PROFILE;
					}
					break;
				case 3:
				case 4:
				case 5:
				case 6:
					tmp_pass[index_option - 3] = (char) tmp_pass[index_option
							- 3] - 1;
					if (tmp_pass[index_option - 3] < '0')
						tmp_pass[index_option - 3] = 'z';
					else if (tmp_pass[index_option - 3] < 'a'
							&& tmp_pass[index_option - 3] > 'Z')
						tmp_pass[index_option - 3] = 'Z';
					else if (tmp_pass[index_option - 3] < 'A'
							&& tmp_pass[index_option - 3] > '9')
						tmp_pass[index_option - 3] = '9';
					sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);

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
							text_pos[index_option].x2,
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
					if (cur_profile == USER_PROFILE) {
						sprintf(tmp_str, "USER");
					} else {
						sprintf(tmp_str, "ADMIN");
					}
					break;
				case 2:
				case 3:
				case 4:
				case 5:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);

					break;
				case 6:
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					index_option = 6;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);
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
					if (cur_profile == USER_PROFILE) {
						sprintf(tmp_str, "USER");
					} else {
						sprintf(tmp_str, "ADMIN");
					}
					index_option = 2;
					break;
				case 4:
				case 5:
				case 6:
					index_option--;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					if (cur_profile == USER_PROFILE)
						sprintf(cur_pass, "%s", PASSWORD_USER_Value);
					else
						sprintf(cur_pass, "%s", PASSWORD_ADMIN_Value);
					if (strcmp(tmp_pass, cur_pass)) {
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
						menu_page = 0;
						create_menu(0, menu_page, 1, text_pos);
						index_option = 0;
						MENU_state = OPTION_MENU;
					}
					break;
				case 1:
					MENU_state = MAIN_MENU;
					DISP_state = DISP_IDLE;
					flag_change_form = 1;
					break;
				case 2:
				case 3:
				case 4:
				case 5:
					index_option++;
					sprintf(tmp_str, "%c", tmp_pass[index_option - 3]);
					break;

				case 6:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				glcd_refresh();
				if (index_option > 1 && MENU_state != OPTION_MENU)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
			break;
			/////////////////////////////////////OPTION_MENU/////////////////////////////////////////////////
		case OPTION_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN, Short_press);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				do {
					index_option++;
					if (menu_page == (MENU_TOTAL_PAGES - 1)) {
						if (index_option
								>= (MENU_TOTAL_ITEMS % MENU_ITEMS_IN_PAGE))
							index_option = 0;
					} else {
						if (index_option >= MENU_ITEMS_IN_PAGE)
							index_option = 0;

					}
				} while (!profile_active[cur_profile][index_option
						+ MENU_ITEMS_IN_PAGE * menu_page]);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				do {
					if (index_option == 0) {
						if (menu_page == (MENU_TOTAL_PAGES - 1))
							index_option =
							MENU_TOTAL_ITEMS % MENU_ITEMS_IN_PAGE;
						else
							index_option = MENU_ITEMS_IN_PAGE;
					}
					index_option--;
				} while (!profile_active[cur_profile][index_option
						+ MENU_ITEMS_IN_PAGE * menu_page]);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				do {

					if (index_option < 5) {
						index_option += 5;
						if (menu_page == (MENU_TOTAL_PAGES - 1)) {
							if (index_option
									>= MENU_TOTAL_ITEMS % MENU_ITEMS_IN_PAGE)
								index_option = index_option % 5;
						} else {
							if (index_option >= MENU_ITEMS_IN_PAGE)
								index_option = index_option % 5;
						}
					} else
						index_option -= 5;

				} while (!profile_active[cur_profile][index_option
						+ MENU_ITEMS_IN_PAGE * menu_page]);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);
				glcd_refresh();
			}
			if (joystick_read(Key_RIGHT, Short_press)) {
				joystick_init(Key_RIGHT, Short_press);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 0);
				do {
					index_option += 5;
					if (menu_page == (MENU_TOTAL_PAGES - 1)) {
						if (index_option
								>= (MENU_TOTAL_ITEMS % MENU_ITEMS_IN_PAGE))
							index_option = index_option % 5;
					} else {
						if (index_option >= MENU_ITEMS_IN_PAGE)
							index_option = index_option % 5;

					}
				} while (!profile_active[cur_profile][index_option
						+ MENU_ITEMS_IN_PAGE * menu_page]);
				draw_text(menu[index_option + MENU_ITEMS_IN_PAGE * menu_page],
						(index_option / 5) * 66 + 2,
						(index_option % 5) * 12 + 1, Tahoma8, 1, 1);

				glcd_refresh();
			}
			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				index_option = index_option + (uint8_t) POSITION_MENU
						+ MENU_ITEMS_IN_PAGE * menu_page;
				if (profile_active[cur_profile][index_option - POSITION_MENU]) {
					switch (index_option) {
					case POSITION_MENU:
						memcpy(&tmp_lat,&LAT_Value,sizeof(POS_t));

//						tmp_lat = LAT_Value;
						memcpy(&tmp_long,&LONG_Value,sizeof(POS_t));

//						tmp_long = LONG_Value;
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
					case TEMP1_MENU:
						memcpy(tmp_limits, TempLimit_Value,
								6 * sizeof(RELAY_t));
						tmp_hys = HYSTERESIS_Value;
						tmp_TEC_STATE = TEC_STATE_Value;
						create_formTemp1(1, text_pos, tmp_limits, tmp_TEC_STATE,
								tmp_hys);
						index_option = 2;
						if (tmp_TEC_STATE == 1)
							sprintf(tmp_str, "ENA");
						else
							sprintf(tmp_str, "DIS");
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);
						glcd_refresh();
						MENU_state = TEMP1_MENU;
						break;
					case TEMP2_MENU:
						memcpy(tmp_limits, TempLimit_Value,
								6 * sizeof(RELAY_t));
						tmp_hys = HYSTERESIS_Value;
						tmp_TEC_STATE = TEC_STATE_Value;
						create_formTemp2(1, text_pos, tmp_limits, tmp_TEC_STATE,
								tmp_hys);
						index_option = 2;
						sprintf(tmp_str, "%4.1f",
								tmp_limits[MOTHERBOARD_TEMP].TemperatureH
										/ 10.0);
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);
						glcd_refresh();
						MENU_state = TEMP2_MENU;
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
					case USEROPTION_MENU:
						if (cur_profile == ADMIN_PROFILE) {
							tmp_profile_user = profile_user_Value;
							create_formUseroption(1, tmp_profile_user,
									text_pos);
							index_option = 2;
							(tmp_profile_user & 0x02) ?
									sprintf(tmp_str, "A") :
									sprintf(tmp_str, "-");
							text_cell(text_pos, index_option, tmp_str, Tahoma8,
									CENTER_ALIGN, 1, 0);
							glcd_refresh();
							MENU_state = USEROPTION_MENU;
						}
						break;
					case NEXT_MENU:
						menu_page = 1;
						index_option = 0;
						create_menu(index_option, menu_page, 1, text_pos);
						MENU_state = OPTION_MENU;
						break;
					case PREV_MENU:
						menu_page = 0;
						index_option = NEXT_MENU-POSITION_MENU;
						create_menu(index_option, menu_page, 1, text_pos);
						MENU_state = OPTION_MENU;
						break;
					case WIFI1_MENU:
						MENU_state = WIFI1_MENU;
#if 0
						if(ESP32ready)
						{
						tmp_wifi = cur_wifi;
						create_formWIFI1(1, tmp_wifi, text_pos);
						index_option = 2;
						sprintf(tmp_str, "%c", tmp_wifi.ssid[0]);
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);
						glcd_refresh();
						MENU_state = WIFI1_MENU;
						}
#endif
						break;
					case WIFI2_MENU:
						MENU_state = WIFI2_MENU;
#if 0
						if(ESP32ready)
						{
						tmp_wifi = cur_wifi;
						create_formWIFI2(1, tmp_wifi, text_pos);
						index_option = 2;
						sprintf(tmp_str, "%c", tmp_wifi.ip[0]);
						text_cell(text_pos, index_option, tmp_str, Tahoma8,
								CENTER_ALIGN, 1, 0);
						glcd_refresh();
						MENU_state = WIFI2_MENU;
						}
#endif
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
					case COPY_MENU:
						MENU_state = COPY_MENU;
						break;
					case FACTORYRESET_MENU:
						MENU_state = FACTORYRESET_MENU;
						break;
					case EXIT_MENU:
						MENU_state = EXIT_MENU;
						break;
					}
				} else {
					index_option -= ((uint8_t) POSITION_MENU
							+ MENU_ITEMS_IN_PAGE * menu_page);
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
//					LAT_Value = tmp_lat;
//					LONG_Value = tmp_long;
					memcpy(&LAT_Value,&tmp_lat,sizeof(POS_t));
					memcpy(&LONG_Value,&tmp_long,sizeof(POS_t));
					UTC_OFF_Value = tmp_utcoff;

					Delay_Astro_calculation = 3;
					flag_rtc_1s_general = 1;
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_DUMMY], 0);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_deg], LAT_Value.deg);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_min], LAT_Value.min);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_second],
							LAT_Value.second);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LAT_direction],
							LAT_Value.direction);

					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_deg],
							LONG_Value.deg);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_min],
							LONG_Value.min);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_second],
							LONG_Value.second);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_LONG_direction],
							LONG_Value.direction);
					HAL_Delay(100);
					EE_WriteVariable(VirtAddVarTab[ADD_UTC_OFF], UTC_OFF_Value);
					sprintf(tmp_str2,
							"%04d-%02d-%02d,%02d:%02d:%02d,POSITION,%02d %02d' %05.2f\" %c,%02d %02d' %05.2f\" %c\n",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							LAT_Value.deg, LAT_Value.min,
							LAT_Value.second / 100.0, LAT_Value.direction,
							LONG_Value.deg, LONG_Value.min,
							LONG_Value.second / 100.0, LONG_Value.direction);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						TimePositionPageEvent);
					}
					if (ActiveBrowserPage[DashboardActivePage]) {
						ready_msg_Dashboardpage(tmp_str2, cur_outsidelight,
								cur_doorstate, cur_client_number, cur_voltage,
								cur_current, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						DashboardEvent);
					}
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU) {
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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

					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						TimePositionPageEvent);
					}
					if (ActiveBrowserPage[DashboardActivePage]) {
						ready_msg_Dashboardpage(tmp_str2, cur_outsidelight,
								cur_doorstate, cur_client_number, cur_voltage,
								cur_current, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						DashboardEvent);
					}

					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////LEDS1_MENU/////////////////////////////////////////////////
		case LEDS1_MENU:
			joystick_init( Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNRISE],
							S2_LED_Value.ADD_SUNRISE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S2_LED_ADD_SUNSET],
							S2_LED_Value.ADD_SUNSET_Value);

					sprintf(tmp_str2,
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
							tmp_str2);
					if (ActiveBrowserPage[LEDActivePage]) {
						ready_msg_LEDpage(tmp_str2, S1_LED_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						LEDS1PageEvent);
					}
					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						TimePositionPageEvent);
					}

					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
						if (ActiveBrowserPage[TimePositionActivePage]) {
							ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
									cur_sunset, cur_browser_profile);
							uart_transmit_frame(tmp_str2, cmd_event,
							TimePositionPageEvent);
						}
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
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////LEDS2_MENU/////////////////////////////////////////////////
		case LEDS2_MENU:
			joystick_init( Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNRISE],
							S1_LED_Value.ADD_SUNRISE_Value);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_S1_LED_ADD_SUNSET],
							S1_LED_Value.ADD_SUNSET_Value);
					sprintf(tmp_str2,
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
							tmp_str2);
					if (ActiveBrowserPage[LEDActivePage]) {
						ready_msg_LEDpage(tmp_str2, S2_LED_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
						LEDS2PageEvent);
					}
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
						if (ActiveBrowserPage[TimePositionActivePage]) {
							ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
									cur_sunset, cur_browser_profile);
							uart_transmit_frame(tmp_str2, cmd_event,
							TimePositionPageEvent);
						}
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

					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			break;
			/////////////////////////////////////TEMPS1_MENU/////////////////////////////////////////////////
		case TEMP1_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_TEC_STATE = 1 - tmp_TEC_STATE;
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					break;
				case 3:
					tmp_hys += 1;
					if (tmp_hys > HYSTERESIS_MAX)
						tmp_hys = HYSTERESIS_MIN;
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3].TemperatureH += 1;
					if (tmp_limits[(index_option - 4) / 3].TemperatureH
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 4) / 3].TemperatureH =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					tmp_limits[(index_option - 5) / 3].TemperatureL += 1;
					if (tmp_limits[(index_option - 5) / 3].TemperatureL
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 5) / 3].TemperatureL =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 6:
				case 9:
				case 12:
					tmp_limits[(index_option - 6) / 3].active = 1
							- tmp_limits[(index_option - 6) / 3].active;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_TEC_STATE = 1 - tmp_TEC_STATE;
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					break;
				case 3:
					tmp_hys += 10;
					if (tmp_hys > HYSTERESIS_MAX)
						tmp_hys = HYSTERESIS_MIN;
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3].TemperatureH += 10;
					if (tmp_limits[(index_option - 4) / 3].TemperatureH
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 4) / 3].TemperatureH =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					tmp_limits[(index_option - 5) / 3].TemperatureL += 10;
					if (tmp_limits[(index_option - 5) / 3].TemperatureL
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 5) / 3].TemperatureL =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 6:
				case 9:
				case 12:
					tmp_limits[(index_option - 6) / 3].active = 1
							- tmp_limits[(index_option - 6) / 3].active;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_TEC_STATE = 1 - tmp_TEC_STATE;
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					break;
				case 3:
					if (tmp_hys == HYSTERESIS_MIN)
						tmp_hys = HYSTERESIS_MIN + 1;
					tmp_hys -= 1;
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3].TemperatureH -= 1;
					if (tmp_limits[(index_option - 4) / 3].TemperatureH
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 4) / 3].TemperatureH =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					tmp_limits[(index_option - 5) / 3].TemperatureL -= 1;
					if (tmp_limits[(index_option - 5) / 3].TemperatureL
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 5) / 3].TemperatureL =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 6:
				case 9:
				case 12:
					tmp_limits[(index_option - 6) / 3].active = 1
							- tmp_limits[(index_option - 6) / 3].active;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					tmp_TEC_STATE = 1 - tmp_TEC_STATE;
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					break;
				case 3:
					if (tmp_hys <= HYSTERESIS_MIN + 10)
						tmp_hys = HYSTERESIS_MIN + 10;
					tmp_hys -= 10;
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3].TemperatureH -= 10;
					if (tmp_limits[(index_option - 4) / 3].TemperatureH
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 4) / 3].TemperatureH =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					tmp_limits[(index_option - 5) / 3].TemperatureL -= 10;
					if (tmp_limits[(index_option - 5) / 3].TemperatureL
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 5) / 3].TemperatureL =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 6:
				case 9:
				case 12:
					tmp_limits[(index_option - 6) / 3].active = 1
							- tmp_limits[(index_option - 6) / 3].active;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					(tmp_limits[2].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 12;
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
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					index_option = 2;
					break;
				case 4:
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					index_option = 3;
					break;
				case 5:
				case 8:
				case 11:
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureH
									/ 10.0);
					index_option -= 1;
					break;
				case 7:
				case 10:
					(tmp_limits[(index_option - 4) / 3 - 1].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option -= 1;
					break;
				case 6:
				case 9:
				case 12:
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 6) / 3].TemperatureL
									/ 10.0);
					index_option -= 1;
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
							text_pos[index_option].x2,
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
					(tmp_TEC_STATE) ?
							sprintf(tmp_str, "ENA") : sprintf(tmp_str, "DIS");
					index_option = 2;
					break;
				case 2:
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					index_option = 3;
					break;
				case 3:
				case 6:
				case 9:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					index_option += 1;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 12:
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					TEC_STATE_Value = tmp_TEC_STATE;
					HYSTERESIS_Value = tmp_hys;
					memcpy(TempLimit_Value, tmp_limits, 6 * sizeof(RELAY_t));

					EE_WriteVariable(VirtAddVarTab[ADD_TEC_STATE],
							TEC_STATE_Value);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_HYS_Temp],
							HYSTERESIS_Value);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempH],
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempL],
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_ENV_active],
							TempLimit_Value[ENVIROMENT_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempH],
							TempLimit_Value[CAM_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempL],
							TempLimit_Value[CAM_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CAM_active],
							TempLimit_Value[CAM_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempH],
							TempLimit_Value[CASE_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempL],
							TempLimit_Value[CASE_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CASE_active],
							TempLimit_Value[CASE_TEMP].active);
					HAL_Delay(50);

					if (TEC_STATE_Value)
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC ENABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					else
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC DISABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,Hysteresis,%f",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							HYSTERESIS_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureH
									/ 10.0,
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureL
									/ 10.0,
							TempLimit_Value[ENVIROMENT_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[CAM_TEMP].TemperatureH / 10.0,
							TempLimit_Value[CAM_TEMP].TemperatureL / 10.0,
							TempLimit_Value[CAM_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[CASE_TEMP].TemperatureH / 10.0,
							TempLimit_Value[CASE_TEMP].TemperatureL / 10.0,
							TempLimit_Value[CASE_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					if (ActiveBrowserPage[TemperatureActivePage]) {
						ready_msg_Temperaturepage(tmp_str2, TempLimit_Value,
								HYSTERESIS_Value, TEC_STATE_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								TemperaturePageEvent);
					}
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 2:
					sprintf(tmp_str, "%3.1f'C", tmp_hys / 10.0);
					index_option = 3;
					break;
				case 3:
				case 6:
				case 9:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3].TemperatureH
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 5) / 3].TemperatureL
									/ 10.0);
					break;
				case 5:
				case 8:
				case 11:
					index_option += 1;
					(tmp_limits[(index_option - 6) / 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 12:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();
			}
			break;
			/////////////////////////////////////TEMPS2_MENU/////////////////////////////////////////////////
		case TEMP2_MENU:
			joystick_init(Key_LEFT | Key_RIGHT | Key_ENTER, Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
				case 5:
				case 8:
					tmp_limits[(index_option - 2) / 3 + 3].TemperatureH += 1;
					if (tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 2) / 3 + 3].TemperatureH =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					tmp_limits[(index_option - 3) / 3 + 3].TemperatureL += 1;
					if (tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 3) / 3 + 3].TemperatureL =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3 + 3].active = 1
							- tmp_limits[(index_option - 4) / 3 + 3].active;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
				case 5:
				case 8:
					tmp_limits[(index_option - 2) / 3 + 3].TemperatureH += 10;
					if (tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 2) / 3 + 3].TemperatureH =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					tmp_limits[(index_option - 3) / 3 + 3].TemperatureL += 10;
					if (tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
							> TEMPERATURE_MAX)
						tmp_limits[(index_option - 3) / 3 + 3].TemperatureL =
								TEMPERATURE_MAX;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3 + 3].active = 1
							- tmp_limits[(index_option - 4) / 3 + 3].active;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
				case 5:
				case 8:
					tmp_limits[(index_option - 2) / 3 + 3].TemperatureH -= 1;
					if (tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 2) / 3 + 3].TemperatureH =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					tmp_limits[(index_option - 3) / 3 + 3].TemperatureL -= 1;
					if (tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 3) / 3 + 3].TemperatureL =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3 + 3].active = 1
							- tmp_limits[(index_option - 4) / 3 + 3].active;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
				case 5:
				case 8:
					tmp_limits[(index_option - 2) / 3 + 3].TemperatureH -= 10;
					if (tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 2) / 3 + 3].TemperatureH =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					tmp_limits[(index_option - 3) / 3 + 3].TemperatureL -= 10;
					if (tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
							< TEMPERATURE_MIN)
						tmp_limits[(index_option - 3) / 3 + 3].TemperatureL =
								TEMPERATURE_MIN;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 - 3].TemperatureL
									/ 10.0);
					break;
				case 4:
				case 7:
				case 10:
					tmp_limits[(index_option - 4) / 3 + 3].active = 1
							- tmp_limits[(index_option - 4) / 3 + 3].active;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}

				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					(tmp_limits[5].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
				case 6:
				case 9:
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureH
									/ 10.0);
					index_option -= 1;
					break;
				case 5:
				case 8:
					(tmp_limits[(index_option - 5) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option -= 1;
					break;
				case 4:
				case 7:
				case 10:
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 4) / 3 + 3].TemperatureL
									/ 10.0);
					index_option -= 1;
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
							text_pos[index_option].x2,
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
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 1) / 3 + 3].TemperatureH
									/ 10.0);
					index_option = 2;
					break;
				case 2:
				case 5:
				case 8:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					index_option += 1;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 4:
				case 7:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:	//OK
						//save in eeprom
					memcpy(TempLimit_Value, tmp_limits, 6 * sizeof(RELAY_t));

					EE_WriteVariable(VirtAddVarTab[ADD_MB_TempH],
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_MB_TempL],
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_MB_active],
							TempLimit_Value[MOTHERBOARD_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempH],
							TempLimit_Value[TECIN_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempL],
							TempLimit_Value[TECIN_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_active],
							TempLimit_Value[TECIN_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempH],
							TempLimit_Value[TECOUT_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempL],
							TempLimit_Value[TECOUT_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_active],
							TempLimit_Value[TECOUT_TEMP].active);
					HAL_Delay(50);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH
									/ 10.0,
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL
									/ 10.0,
							TempLimit_Value[MOTHERBOARD_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[TECIN_TEMP].TemperatureH / 10.0,
							TempLimit_Value[TECIN_TEMP].TemperatureL / 10.0,
							TempLimit_Value[TECIN_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[TECOUT_TEMP].TemperatureH / 10.0,
							TempLimit_Value[TECOUT_TEMP].TemperatureL / 10.0,
							TempLimit_Value[TECOUT_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					if (ActiveBrowserPage[TemperatureActivePage]) {
						ready_msg_Temperaturepage(tmp_str2, TempLimit_Value,
								HYSTERESIS_Value, TEC_STATE_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								TemperaturePageEvent);
					}
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 2:
				case 5:
				case 8:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 3) / 3 + 3].TemperatureL
									/ 10.0);
					break;
				case 3:
				case 6:
				case 9:
					index_option += 1;
					(tmp_limits[(index_option - 4) / 3 + 3].active) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 4:
				case 7:
					index_option += 1;
					sprintf(tmp_str, "%4.1f",
							tmp_limits[(index_option - 2) / 3 + 3].TemperatureH
									/ 10.0);
					break;
				case 10:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1 && MENU_state != OPTION_MENU) {
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
					sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,Door,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							DOOR_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 3:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1 && MENU_state != OPTION_MENU) {
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
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
						if (cur_profile == USER_PROFILE) {
							strncpy(PASSWORD_USER_Value, tmp_pass, 5);
							HAL_Delay(50);
							EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_0],
									PASSWORD_USER_Value[0]);
							HAL_Delay(50);
							EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_1],
									PASSWORD_USER_Value[1]);
							HAL_Delay(50);
							EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_2],
									PASSWORD_USER_Value[2]);
							HAL_Delay(50);
							EE_WriteVariable(VirtAddVarTab[ADD_PASSWORD_USER_3],
									PASSWORD_USER_Value[3]);

							sprintf(tmp_str,
									"%04d-%02d-%02d,%02d:%02d:%02d,PASSWORD USER CHANGED",
									cur_Date.Year + 2000, cur_Date.Month,
									cur_Date.Date, cur_time.Hours,
									cur_time.Minutes, cur_time.Seconds);
						} else {
							strncpy(PASSWORD_ADMIN_Value, tmp_pass, 5);
							HAL_Delay(50);
							EE_WriteVariable(
									VirtAddVarTab[ADD_PASSWORD_ADMIN_0],
									PASSWORD_ADMIN_Value[0]);
							HAL_Delay(50);
							EE_WriteVariable(
									VirtAddVarTab[ADD_PASSWORD_ADMIN_1],
									PASSWORD_ADMIN_Value[1]);
							HAL_Delay(50);
							EE_WriteVariable(
									VirtAddVarTab[ADD_PASSWORD_ADMIN_2],
									PASSWORD_ADMIN_Value[2]);
							HAL_Delay(50);
							EE_WriteVariable(
									VirtAddVarTab[ADD_PASSWORD_ADMIN_3],
									PASSWORD_ADMIN_Value[3]);

							sprintf(tmp_str2,
									"%04d-%02d-%02d,%02d:%02d:%02d,PASSWORD ADMIN CHANGED",
									cur_Date.Year + 2000, cur_Date.Month,
									cur_Date.Date, cur_time.Hours,
									cur_time.Minutes, cur_time.Seconds);
						}
						r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
								tmp_str2);

						index_option = MENU_state - POSITION_MENU;
						create_menu(index_option, menu_page, 1, text_pos);
						MENU_state = OPTION_MENU;
					}
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
			break;
			/////////////////////////////////////USEROPTION_MENU/////////////////////////////////////////////////
		case USEROPTION_MENU:
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)
					| joystick_read(Key_DOWN, Short_press)) {
				joystick_init(Key_DOWN | Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
				}
				switch (index_option) {
				case 0:	//OK
					break;
				case 1:	//CANCEL
					break;
				case 2:
					(tmp_profile_user & 0x02) ?
							(tmp_profile_user &= 0xFD) : (tmp_profile_user |=
									0x02);
					(tmp_profile_user & 0x02) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 3:
					(tmp_profile_user & 0x04) ?
							(tmp_profile_user &= 0xFB) : (tmp_profile_user |=
									0x04);
					(tmp_profile_user & 0x04) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 4:
					(tmp_profile_user & 0x08) ?
							(tmp_profile_user &= 0xF7) : (tmp_profile_user |=
									0x08);
					(tmp_profile_user & 0x08) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 5:
					(tmp_profile_user & 0x20) ?
							(tmp_profile_user &= 0xDF) : (tmp_profile_user |=
									0x20);
					(tmp_profile_user & 0x20) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 6:
					(tmp_profile_user & 0x40) ?
							(tmp_profile_user &= 0xBF) : (tmp_profile_user |=
									0x40);
					(tmp_profile_user & 0x40) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					break;
				case 7:
					(tmp_profile_user & 0x80) ?
							(tmp_profile_user &= 0x7F) : (tmp_profile_user |=
									0x80);
					(tmp_profile_user & 0x80) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
					(tmp_profile_user & 0x02) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 2;
					break;
				case 2:
					(tmp_profile_user & 0x04) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 3;
					break;
				case 3:
					(tmp_profile_user & 0x08) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 4;
					break;
				case 4:
					(tmp_profile_user & 0x20) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 5;
					break;
				case 5:
					(tmp_profile_user & 0x40) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 6;
					break;
				case 6:
					(tmp_profile_user & 0x80) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 7;
					break;
				case 7:
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
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:	//OK
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					(tmp_profile_user & 0x80) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
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
					(tmp_profile_user & 0x02) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 2;
					break;
				case 4:
					(tmp_profile_user & 0x04) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 3;
					break;
				case 5:
					(tmp_profile_user & 0x08) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 4;
					break;
				case 6:
					(tmp_profile_user & 0x20) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 5;
					break;
				case 7:
					(tmp_profile_user & 0x40) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 6;
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
					draw_fill(text_pos[index_option].x1 - 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				glcd_refresh();
				switch (index_option) {
				case 0:
					profile_user_Value = tmp_profile_user;
					if (profile_user_Value & 0x02) {
						profile_active[USER_PROFILE][2] = 1;
						profile_active[USER_PROFILE][3] = 1;
					} else {
						profile_active[USER_PROFILE][2] = 0;
						profile_active[USER_PROFILE][3] = 0;
					}

					if (profile_user_Value & 0x04) {
						profile_active[USER_PROFILE][0] = 1;
						profile_active[USER_PROFILE][1] = 1;
					} else {
						profile_active[USER_PROFILE][0] = 0;
						profile_active[USER_PROFILE][1] = 0;
					}

					if (profile_user_Value & 0x08) {
						profile_active[USER_PROFILE][5] = 1;
						profile_active[USER_PROFILE][6] = 1;
					} else {
						profile_active[USER_PROFILE][5] = 0;
						profile_active[USER_PROFILE][6] = 0;
					}
					if (profile_user_Value & 0x10) {
						profile_active[USER_PROFILE][7] = 1;
					} else {
						profile_active[USER_PROFILE][7] = 0;
					}
					if (profile_user_Value & 0x20) {
						profile_active[USER_PROFILE][11] = 1;
						profile_active[USER_PROFILE][12] = 1;
					} else {
						profile_active[USER_PROFILE][11] = 0;
						profile_active[USER_PROFILE][12] = 0;
					}

					if (profile_user_Value & 0x40) {
						profile_active[USER_PROFILE][13] = 1;
					} else {
						profile_active[USER_PROFILE][13] = 0;
					}

					if (profile_user_Value & 0x80) {
						profile_active[USER_PROFILE][8] = 1;
					} else {
						profile_active[USER_PROFILE][8] = 0;
					}
					EE_WriteVariable(VirtAddVarTab[ADD_PROFILE_USER],
							profile_user_Value);
					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,User Option,%x",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							profile_user_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					if (ActiveBrowserPage[DashboardActivePage]) {
						ready_msg_Dashboardpage(tmp_str2, cur_outsidelight,
								cur_doorstate, cur_client_number, cur_voltage,
								cur_current, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								DashboardEvent);
					}

					if (ActiveBrowserPage[LEDActivePage]) {
						ready_msg_LEDpage(tmp_str2, S1_LED_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								LEDS1PageEvent);
					}

					if (ActiveBrowserPage[TimePositionActivePage]) {
						ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
								cur_sunset, cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								TimePositionPageEvent);
					}

					if (ActiveBrowserPage[TemperatureActivePage]) {
						ready_msg_Temperaturepage(tmp_str2, TempLimit_Value,
								HYSTERESIS_Value, TEC_STATE_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								TemperaturePageEvent);
					}
					if (ActiveBrowserPage[UserOptionActivePage]) {
						ready_msg_Useroptionpage(tmp_str2, profile_user_Value,
								cur_browser_profile);
						uart_transmit_frame(tmp_str2, cmd_event,
								UseroptionPageEvent);
					}
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = MENU_state - POSITION_MENU;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 2:
					(tmp_profile_user & 0x04) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 3;
					break;
				case 3:
					(tmp_profile_user & 0x08) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 4;
					break;
				case 4:
					(tmp_profile_user & 0x20) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 5;
					break;
				case 5:
					(tmp_profile_user & 0x40) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 6;
					break;
				case 6:
					(tmp_profile_user & 0x80) ?
							sprintf(tmp_str, "A") : sprintf(tmp_str, "-");
					index_option = 7;
					break;
				case 7:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;

				}
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				}
				glcd_refresh();

			}
			break;
			/////////////////////////////////////WIFI1_MENU/////////////////////////////////////////////////
		case WIFI1_MENU:
			index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
			create_menu(index_option, menu_page, 1, text_pos);
			MENU_state = OPTION_MENU;
#if 0
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,
					Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1-1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
					tmp_wifi.ssid[index_option - 2] =
							(char) tmp_wifi.ssid[index_option - 2] + 1;

					if (tmp_wifi.ssid[index_option - 2] > ' '&& tmp_wifi.ssid[index_option - 2] < '0')
						tmp_wifi.ssid[index_option - 2] = '0';
					else if (tmp_wifi.ssid[index_option - 2] > '9'&& tmp_wifi.ssid[index_option - 2] < 'A')
						tmp_wifi.ssid[index_option - 2] = 'A';
					else if (tmp_wifi.ssid[index_option - 2] > 'Z'&& tmp_wifi.ssid[index_option - 2] < 'a')
						tmp_wifi.ssid[index_option - 2] = 'a';
					else if (tmp_wifi.ssid[index_option - 2] > 'z')
						tmp_wifi.ssid[index_option - 2] = ' ';
					sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);
					break;
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
				case 21:
					tmp_wifi.pass[index_option - 12] =	(char) tmp_wifi.pass[index_option - 12] + 1;

					if (tmp_wifi.pass[index_option - 12] > ' '&& tmp_wifi.pass[index_option - 12] < '0')
						tmp_wifi.pass[index_option - 12] = '0';
					else if (tmp_wifi.pass[index_option - 12] > '9'	&& tmp_wifi.pass[index_option - 12] < 'A')
						tmp_wifi.pass[index_option - 12] = 'A';
					else if (tmp_wifi.pass[index_option - 12] > 'Z'	&& tmp_wifi.pass[index_option - 12] < 'a')
						tmp_wifi.pass[index_option - 12] = 'a';
					else if (tmp_wifi.pass[index_option - 12] > 'z')
						tmp_wifi.pass[index_option - 12] = ' ';
					sprintf(tmp_str, "%c", tmp_wifi.pass[index_option - 12]);
					break;
				case 22:
					tmp_wifi.ssidhidden = 1 - tmp_wifi.ssidhidden;
					(tmp_wifi.ssidhidden) ?
							sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					break;
				case 23:
					tmp_wifi.maxclients++;
					if (tmp_wifi.maxclients > 2)
						tmp_wifi.maxclients = 1;
					sprintf(tmp_str, "%d", tmp_wifi.maxclients);
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
					draw_fill(text_pos[index_option].x1-1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
					tmp_wifi.ssid[index_option - 2] =
							(char) tmp_wifi.ssid[index_option - 2] - 1;

					if (tmp_wifi.ssid[index_option - 2] < ' ')
						tmp_wifi.ssid[index_option - 2] = 'z';
					else if (tmp_wifi.ssid[index_option - 2] < '0'
							&& tmp_wifi.ssid[index_option - 2] > ' ')
						tmp_wifi.ssid[index_option - 2] = ' ';
					else if (tmp_wifi.ssid[index_option - 2] < 'A'
							&& tmp_wifi.ssid[index_option - 2] > '9')
						tmp_wifi.ssid[index_option - 2] = '9';
					else if (tmp_wifi.ssid[index_option - 2] < 'a'
							&& tmp_wifi.ssid[index_option - 2] > 'Z')
						tmp_wifi.ssid[index_option - 2] = 'Z';
					sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);

					break;
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
				case 21:
					tmp_wifi.pass[index_option - 12] =
							(char) tmp_wifi.pass[index_option - 12] - 1;

					if (tmp_wifi.pass[index_option - 12] < ' ')
						tmp_wifi.pass[index_option - 12] = 'z';
					else if (tmp_wifi.pass[index_option - 12] < '0'
							&& tmp_wifi.pass[index_option - 12] > ' ')
						tmp_wifi.pass[index_option - 12] = ' ';
					else if (tmp_wifi.pass[index_option - 12] < 'A'
							&& tmp_wifi.pass[index_option - 12] > '9')
						tmp_wifi.pass[index_option - 12] = '9';
					else if (tmp_wifi.pass[index_option - 12] < 'a'
							&& tmp_wifi.pass[index_option - 12] > 'Z')
						tmp_wifi.pass[index_option - 12] = 'Z';
					sprintf(tmp_str, "%c", tmp_wifi.pass[index_option - 12]);
					break;
				case 22:
					tmp_wifi.ssidhidden = 1 - tmp_wifi.ssidhidden;
					(tmp_wifi.ssidhidden) ?
							sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					break;
				case 23:
					tmp_wifi.maxclients--;
					if (tmp_wifi.maxclients < 1)
						tmp_wifi.maxclients = 2;
					sprintf(tmp_str, "%d", tmp_wifi.maxclients);
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
							text_pos[index_option].x2,
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
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,0);
					index_option = 2;
					sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);
					break;
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (tmp_wifi.ssid[index_option - 2] == ' ') {
						for(uint8_t i=index_option-2;i<10;i++)
						{
							tmp_wifi.ssid[i]=' ';
							sprintf(tmp_str, "%c", tmp_wifi.ssid[i]);
							draw_fill(text_pos[i+2].x1 + 1,
									text_pos[i+2].y1 + 1,
									text_pos[i+2].x2,
									text_pos[i+2].y2 - 1, 0);
							text_cell(text_pos, i+2, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
						}
						index_option = 12;
						sprintf(tmp_str, "%c",	tmp_wifi.pass[index_option - 12]);
					} else {
						index_option++;
						sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);
					}
					break;
				case 11:
					index_option++;
					sprintf(tmp_str, "%c",tmp_wifi.pass[index_option - 12]);
					break;
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
					if (tmp_wifi.pass[index_option - 12] == ' ') {
						for(uint8_t i=index_option-12;i<10;i++)
						{
							tmp_wifi.pass[i]=' ';
							sprintf(tmp_str, "%c", tmp_wifi.pass[i]);
							draw_fill(text_pos[i+12].x1 + 1,
									text_pos[i+12].y1 + 1,
									text_pos[i+12].x2,
									text_pos[i+12].y2 - 1, 0);
							text_cell(text_pos, i+12, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
						}
						index_option = 22;
						(tmp_wifi.ssidhidden) ?	sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					} else {
						index_option++;
						sprintf(tmp_str, "%c",tmp_wifi.pass[index_option - 12]);
					}
					break;
				case 21:
					index_option++;
					(tmp_wifi.ssidhidden) ?	sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					break;
				case 22:
					index_option++;
					sprintf(tmp_str, "%d", tmp_wifi.maxclients);
					break;
				case 23:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,		CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					index_option = 23;
					sprintf(tmp_str, "%d", tmp_wifi.maxclients);
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
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
					do {
						index_option--;
					} while (tmp_wifi.ssid[index_option - 2] == ' ' && index_option>2);
					sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);
					break;
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
				case 21:
				case 22:
					do {
						index_option--;

					} while (tmp_wifi.pass[index_option - 12] == ' ' && index_option>12);
					sprintf(tmp_str, "%c", tmp_wifi.pass[index_option - 12]);
					break;
				case 23:
					index_option--;
					(tmp_wifi.ssidhidden) ?
							sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:	//OK

					cur_wifi = tmp_wifi;

					sprintf(tmp_str2, "%s,%s,%s,%s,%s,%d,%d,%d",
							strtok(cur_wifi.ssid, " "),
							strtok(cur_wifi.pass, " "),
							cur_wifi.ip,
							cur_wifi.gateway,
							cur_wifi.netmask,
							cur_wifi.ssidhidden,
							cur_wifi.maxclients,
							cur_wifi.txpower);
					uart_transmit_frame(tmp_str2, cmd_event, WifiPageEvent);
					index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (tmp_wifi.ssid[index_option - 2] == ' ') {
						for(uint8_t i=index_option-2;i<10;i++)
						{
							tmp_wifi.ssid[i]=' ';
							sprintf(tmp_str, "%c", tmp_wifi.ssid[i]);
							draw_fill(text_pos[i+2].x1 + 1,
									text_pos[i+2].y1 + 1,
									text_pos[i+2].x2,
									text_pos[i+2].y2 - 1, 0);
							text_cell(text_pos, i+2, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
						}
						index_option = 12;
						sprintf(tmp_str, "%c",	tmp_wifi.pass[index_option - 12]);
					} else {
						index_option++;
						sprintf(tmp_str, "%c", tmp_wifi.ssid[index_option - 2]);
					}
					break;
				case 11:
					index_option++;
					sprintf(tmp_str, "%c",tmp_wifi.pass[index_option - 12]);
					break;
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
					if (tmp_wifi.pass[index_option - 12] == ' ') {
						for(uint8_t i=index_option-12;i<10;i++)
						{
							tmp_wifi.pass[i]=' ';
							sprintf(tmp_str, "%c", tmp_wifi.pass[i]);
							draw_fill(text_pos[i+12].x1 + 1,
									text_pos[i+12].y1 + 1,
									text_pos[i+12].x2,
									text_pos[i+12].y2 - 1, 0);
							text_cell(text_pos, i+12, tmp_str, Tahoma8,	CENTER_ALIGN, 0, 0);
						}
						index_option = 22;
						(tmp_wifi.ssidhidden) ?	sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					} else {
						index_option++;
						sprintf(tmp_str, "%c",tmp_wifi.pass[index_option - 12]);
					}
					break;
				case 21:
					index_option++;
					(tmp_wifi.ssidhidden) ?	sprintf(tmp_str, "Y") : sprintf(tmp_str, "N");
					break;
				case 22:
					index_option++;
					sprintf(tmp_str, "%d", tmp_wifi.maxclients);
					break;
				case 23:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1 && MENU_state != OPTION_MENU)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
#endif
			break;
			/////////////////////////////////////WIFI2_MENU/////////////////////////////////////////////////
		case WIFI2_MENU:
			index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
			create_menu(index_option, menu_page, 1, text_pos);
			MENU_state = OPTION_MENU;
#if 0
			joystick_init(Key_DOWN | Key_TOP | Key_LEFT | Key_RIGHT | Key_ENTER,Long_press);
			if (joystick_read(Key_TOP, Short_press)) {
				joystick_init(Key_TOP, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1-1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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
				case 6:
				case 7:
				case 8:
				case 10:
				case 11:
				case 12:
				case 14:
				case 15:
				case 16:
					tmp_wifi.ip[index_option - 2] =	(char) tmp_wifi.ip[index_option - 2] + 1;
					if (tmp_wifi.ip[index_option - 2] > '9')
						tmp_wifi.ip[index_option - 2] = '0';
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 17:
				case 18:
				case 19:
				case 21:
				case 22:
				case 23:
				case 25:
				case 26:
				case 27:
				case 29:
				case 30:
				case 31:
					tmp_wifi.gateway[index_option - 17] =	(char) tmp_wifi.gateway[index_option - 17] + 1;

					if (tmp_wifi.gateway[index_option - 17] > '9')
						tmp_wifi.gateway[index_option - 17] = '0';
					sprintf(tmp_str, "%c", tmp_wifi.gateway[index_option - 17]);
					break;
				case 32:
				case 33:
				case 34:
				case 36:
				case 37:
				case 38:
				case 40:
				case 41:
				case 42:
				case 44:
				case 45:
				case 46:
					tmp_wifi.netmask[index_option - 32] =	(char) tmp_wifi.netmask[index_option - 32] + 1;

					if (tmp_wifi.netmask[index_option - 32] > '9')
						tmp_wifi.netmask[index_option - 32] = '0';
					sprintf(tmp_str, "%c", tmp_wifi.netmask[index_option - 32]);
					break;
				case 47:
					tmp_wifi.txpower++;
					if(tmp_wifi.txpower>11)
						tmp_wifi.txpower=0;
					sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
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
					draw_fill(text_pos[index_option].x1-1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
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

				case 6:
				case 7:
				case 8:

				case 10:
				case 11:
				case 12:

				case 14:
				case 15:
				case 16:
					tmp_wifi.ip[index_option - 2] =	(char) tmp_wifi.ip[index_option - 2] - 1;
					if (tmp_wifi.ip[index_option - 2] < '0')
						tmp_wifi.ip[index_option - 2] = '9';
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 17:
				case 18:
				case 19:

				case 21:
				case 22:
				case 23:

				case 25:
				case 26:
				case 27:

				case 29:
				case 30:
				case 31:
					tmp_wifi.gateway[index_option - 17] =	(char) tmp_wifi.gateway[index_option - 17] -1;

					if (tmp_wifi.gateway[index_option - 17] < '0')
						tmp_wifi.gateway[index_option - 17] = '9';
					sprintf(tmp_str, "%c", tmp_wifi.gateway[index_option - 17]);
					break;
				case 32:
				case 33:
				case 34:

				case 36:
				case 37:
				case 38:

				case 40:
				case 41:
				case 42:

				case 44:
				case 45:
				case 46:
					tmp_wifi.netmask[index_option - 32] =	(char) tmp_wifi.netmask[index_option - 32] - 1;

					if (tmp_wifi.netmask[index_option - 32] < '0')
						tmp_wifi.netmask[index_option - 32] = '9';
					sprintf(tmp_str, "%c", tmp_wifi.netmask[index_option - 32]);
					break;
				case 47:
					if(tmp_wifi.txpower==0)
						tmp_wifi.txpower=12;
					tmp_wifi.txpower--;
					sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
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
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,1);
					index_option = 1;
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,0);
					index_option = 2;
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 2:
				case 3:
				case 4:

				case 6:
				case 7:
				case 8:

				case 10:
				case 11:
				case 12:

				case 14:
				case 15:
					index_option++;
					if(index_option==5||index_option==9||index_option==13) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 16:

				case 17:
				case 18:
				case 19:

				case 21:
				case 22:
				case 23:

				case 25:
				case 26:
				case 27:

				case 29:
				case 30:
					index_option++;
					if(index_option==20||index_option==24||index_option==28) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.gateway[index_option - 17]);
					break;
				case 31:

				case 32:
				case 33:
				case 34:

				case 36:
				case 37:
				case 38:

				case 40:
				case 41:
				case 42:

				case 44:
				case 45:
					index_option++;
					if(index_option==35||index_option==39||index_option==43) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.netmask[index_option - 32]);
					break;
				case 46:
					index_option++;
					sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
					break;
				case 47:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,		CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
			if (joystick_read(Key_LEFT, Short_press)) {
				joystick_init(Key_LEFT, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 0, 0);

					index_option = 47;
					sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
					break;
				case 1:
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 0,
							0);
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				case 2:
					index_option = 1;
					text_cell(text_pos, 1, "CANCEL", Tahoma8, CENTER_ALIGN, 1,1);
					break;
				case 3:
				case 4:

				case 6:
				case 7:
				case 8:

				case 10:
				case 11:
				case 12:

				case 14:
				case 15:
				case 16:

				case 17:
					index_option--;
					if(index_option==5||index_option==9||index_option==13) index_option--;
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 18:
				case 19:

				case 21:
				case 22:
				case 23:

				case 25:
				case 26:
				case 27:

				case 29:
				case 30:
				case 31:

				case 32:
					index_option--;
					if(index_option==20||index_option==24||index_option==28) index_option--;
					sprintf(tmp_str, "%c", tmp_wifi.gateway[index_option - 17]);
					break;
				case 33:
				case 34:

				case 36:
				case 37:
				case 38:

				case 40:
				case 41:
				case 42:

				case 44:
				case 45:
				case 46:

				case 47:
					index_option--;
					if(index_option==35||index_option==39||index_option==43) index_option--;
					sprintf(tmp_str, "%c", tmp_wifi.netmask[index_option - 32]);
					break;
				}
				if (index_option > 1)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,	CENTER_ALIGN, 1, 0);

				glcd_refresh();

			}

			if (joystick_read(Key_ENTER, Short_press)) {
				joystick_init(Key_ENTER, Short_press);
				if (index_option > 1) {
					draw_fill(text_pos[index_option].x1 + 1,
							text_pos[index_option].y1 + 1,
							text_pos[index_option].x2,
							text_pos[index_option].y2 - 1, 0);
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 0, 0);
				}
				switch (index_option) {
				case 0:	//OK

					cur_wifi = tmp_wifi;
					sprintf(tmp_str2, "%s,%s,%s,%s,%s,%d,%d,%d",
							strtok(cur_wifi.ssid, " "),
							strtok(cur_wifi.pass, " "),
							cur_wifi.ip,
							cur_wifi.gateway,
							cur_wifi.netmask,
							cur_wifi.ssidhidden,
							cur_wifi.maxclients,
							cur_wifi.txpower);
					uart_transmit_frame(tmp_str2, cmd_event, WifiPageEvent);
					index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 1:					//CANCEL
					index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
					create_menu(index_option, menu_page, 1, text_pos);
					MENU_state = OPTION_MENU;
					break;
				case 2:
				case 3:
				case 4:

				case 6:
				case 7:
				case 8:

				case 10:
				case 11:
				case 12:

				case 14:
				case 15:
					index_option++;
					if(index_option==5||index_option==9||index_option==13) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.ip[index_option - 2]);
					break;
				case 16:

				case 17:
				case 18:
				case 19:

				case 21:
				case 22:
				case 23:

				case 25:
				case 26:
				case 27:

				case 29:
				case 30:
					index_option++;
					if(index_option==20||index_option==24||index_option==28) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.gateway[index_option - 17]);
					break;
				case 31:

				case 32:
				case 33:
				case 34:

				case 36:
				case 37:
				case 38:

				case 40:
				case 41:
				case 42:

				case 44:
				case 45:
					index_option++;
					if(index_option==35||index_option==39||index_option==43) index_option++;
					sprintf(tmp_str, "%c", tmp_wifi.netmask[index_option - 32]);
					break;
				case 46:
					index_option++;
					sprintf(tmp_str, "%s", tx_array[tmp_wifi.txpower]);
					break;
				case 47:
					text_cell(text_pos, 0, "OK", Tahoma8, CENTER_ALIGN, 1, 1);
					index_option = 0;
					break;
				}
				if (index_option > 1 && MENU_state != OPTION_MENU)
					text_cell(text_pos, index_option, tmp_str, Tahoma8,
							CENTER_ALIGN, 1, 0);
				glcd_refresh();

			}
#endif
			break;
			/////////////////////////////////////COPY_MENU/////////////////////////////////////////////////
		case COPY_MENU:
//			Copy2USB();
			index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
			create_menu(index_option, menu_page, 1, text_pos);
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
						text_pos[index_option].x2,
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
						text_pos[index_option].x2,
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
					index_option = (MENU_state - POSITION_MENU)%MENU_ITEMS_IN_PAGE;
					create_menu(index_option, menu_page, 1, text_pos);
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
				if (index_option > 1 && MENU_state != OPTION_MENU) {
					Peripherials_DeInit();
					HAL_Delay(100);
					SCB->AIRCR = 0x05FA0000 | (uint32_t) 0x04; //system reset
					while (1)
						;
				}
				glcd_refresh();
			}

			break;
			/////////////////////////////////////FACTORYRESET_MENU/////////////////////////////////////////////////
		case FACTORYRESET_MENU:
			glcd_blank();
			create_cell(0, 0, 128, 64, 1, 1, 1, text_pos);
			text_cell(text_pos, 0, "Loading Defaults", Tahoma8, CENTER_ALIGN, 0,
					0);
			glcd_refresh();
			Write_defaults();
			update_values();

			index_option = (MENU_state - POSITION_MENU) % MENU_ITEMS_IN_PAGE;
			create_menu(index_option, menu_page, 1, text_pos);
			MENU_state = OPTION_MENU;
			break;
			/////////////////////////////////////EXIT_MENU/////////////////////////////////////////////////
		case EXIT_MENU:
			MENU_state = MAIN_MENU;
			DISP_state = DISP_IDLE;
			flag_change_form = 1;
			break;
#endif
		}

		///////////////////////////////////////////////END SWITCH//////////////////////////////////////////////////////
		/////////////////////Temperature Control Algorithm//////////////////////////////////
#if 0
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
			}
			else //if((NTC_Centigrade<NTCTH_Value &&  TEC_overtemp==0)|| (NTC_Centigrade<NTCTL_Value))
			{
				if (TempLimit_Value[CAM_TEMP].active && (uint16_t) cur_temperature[TMP_CH5] != 0x8fff) {
					Cam_temperature=cur_temperature[TMP_CH5];
					Delta_T_cam = Cam_temperature - prev_cam_temperature[0];
					if (Cam_temperature >= TempLimit_Value[CAM_TEMP].TemperatureH) {
						second_algorithm_temperature=0;
						FAN_ON();
						if (algorithm_temp_state != 1) {
							algorithm_temp_state = 1;
							sprintf(tmp_str2,
									"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,-",
									cur_Date.Year + 2000,
									cur_Date.Month, cur_Date.Date,
									cur_time.Hours, cur_time.Minutes,
									cur_time.Seconds);
							r_logparam = Log_file(SDCARD_DRIVE,	PARAMETER_FILE, tmp_str2);
						}

					} else if ((Delta_T_cam <-1)	&& (Cam_temperature	> (TempLimit_Value[CAM_TEMP].TemperatureH- HYSTERESIS_Value))) //s2
					{
						second_algorithm_temperature=0;
						FAN_ON();
						if (algorithm_temp_state != 2) {
							algorithm_temp_state = 2;
							sprintf(tmp_str2,
									"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,-",
									cur_Date.Year + 2000,
									cur_Date.Month, cur_Date.Date,
									cur_time.Hours, cur_time.Minutes,
									cur_time.Seconds);
							r_logparam = Log_file(SDCARD_DRIVE,
									PARAMETER_FILE, tmp_str2);
						}
					}
					else if ((Delta_T_cam >1)	&& (Cam_temperature	> (TempLimit_Value[CAM_TEMP].TemperatureH- HYSTERESIS_Value))) //s2
					{
						second_algorithm_temperature=1;
//						FAN_OFF();
						if (algorithm_temp_state != 3) {
							algorithm_temp_state = 3;
							sprintf(tmp_str2,
									"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,-",
									cur_Date.Year + 2000,
									cur_Date.Month, cur_Date.Date,
									cur_time.Hours, cur_time.Minutes,
									cur_time.Seconds);
							r_logparam = Log_file(SDCARD_DRIVE,
									PARAMETER_FILE, tmp_str2);
						}
					}
					else if ((Cam_temperature<= (TempLimit_Value[CAM_TEMP].TemperatureH- HYSTERESIS_Value))) //s2
					{
							second_algorithm_temperature=1;
//							FAN_OFF();
							if (algorithm_temp_state != 4) {
								algorithm_temp_state = 4;
								sprintf(tmp_str2,
										"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,-",
										cur_Date.Year + 2000,
										cur_Date.Month, cur_Date.Date,
										cur_time.Hours, cur_time.Minutes,
										cur_time.Seconds);
								r_logparam = Log_file(SDCARD_DRIVE,
										PARAMETER_FILE, tmp_str2);
							}

					}
					prev_cam_temperature[0]=prev_cam_temperature[1];
					prev_cam_temperature[1]=prev_cam_temperature[2];
					prev_cam_temperature[2]=Cam_temperature;
				} else {
					FAN_ON();
				}
				if(second_algorithm_temperature)
				{
					if (TempLimit_Value[ENVIROMENT_TEMP].active	&& (uint16_t)cur_temperature[TMP_CH4] != 0x8fff) {
						Env_temperature = cur_temperature[TMP_CH4];
						Delta_T_Env = Env_temperature - prev_Env_temperature[0];
						if (Env_temperature	>= TempLimit_Value[ENVIROMENT_TEMP].TemperatureH) //s1
						{
								FAN_ON();
//								TEC_COLD();
								if (algorithm_temp_state != 5) {
									algorithm_temp_state = 5;
									sprintf(tmp_str2,
											"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,COLD",
											cur_Date.Year + 2000,
											cur_Date.Month, cur_Date.Date,
											cur_time.Hours, cur_time.Minutes,
											cur_time.Seconds);
									r_logparam = Log_file(SDCARD_DRIVE,
											PARAMETER_FILE, tmp_str2);
								}
						} else if ((Delta_T_Env <-1)&& (Env_temperature> (TempLimit_Value[ENVIROMENT_TEMP].TemperatureH- HYSTERESIS_Value))) //s2
							{
								FAN_ON();
//								TEC_COLD();
								if (algorithm_temp_state != 6) {
									algorithm_temp_state = 6;
									sprintf(tmp_str2,
											"%04d-%02d-%02d,%02d:%02d:%02d,FAN,ON,TEC,-",
											cur_Date.Year + 2000,
											cur_Date.Month, cur_Date.Date,
											cur_time.Hours, cur_time.Minutes,
											cur_time.Seconds);
									r_logparam = Log_file(SDCARD_DRIVE,
											PARAMETER_FILE, tmp_str2);
								}

						} else if ((Delta_T_Env > 1)&&(Env_temperature> (TempLimit_Value[ENVIROMENT_TEMP].TemperatureH- HYSTERESIS_Value))) //s3
							{
								FAN_OFF();
								if (algorithm_temp_state != 7) {
									algorithm_temp_state = 7;
									sprintf(tmp_str2,
											"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,-",
											cur_Date.Year + 2000,
											cur_Date.Month, cur_Date.Date,
											cur_time.Hours, cur_time.Minutes,
											cur_time.Seconds);
									r_logparam = Log_file(SDCARD_DRIVE,
											PARAMETER_FILE, tmp_str2);
								}
						} else if (Env_temperature	<= (TempLimit_Value[ENVIROMENT_TEMP].TemperatureH- HYSTERESIS_Value)) //s4
						{
							FAN_OFF();
							if (algorithm_temp_state != 8) {
								algorithm_temp_state = 8;
								sprintf(tmp_str2,
										"%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF,TEC,-",
										cur_Date.Year + 2000,
										cur_Date.Month, cur_Date.Date,
										cur_time.Hours, cur_time.Minutes,
										cur_time.Seconds);
								r_logparam = Log_file(SDCARD_DRIVE,
										PARAMETER_FILE, tmp_str2);
							}
						}
						prev_Env_temperature[0] = prev_Env_temperature[1];
						prev_Env_temperature[1] = prev_Env_temperature[2];
						prev_Env_temperature[2] = Env_temperature;
					} else {
						FAN_ON();
					}

				}
			}
			/////////////////////////////////////if Tec is not active///////////////////////////////////////////////
		} else {
			FAN_ON();
			if (algorithm_temp_state != 9) {
				algorithm_temp_state = 9;
				sprintf(tmp_str2, "%04d-%02d-%02d,%02d:%02d:%02d,FAN,OFF",
						cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
						cur_time.Hours, cur_time.Minutes, cur_time.Seconds);
				r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE, tmp_str2);
			}
		}
#endif
		//////////////////////////////////////////check ESP32 Frame////////////////////////////////////////
#if 0
		if (received_valid) {
			received_valid = 0;

			memcpy(received_frame,received_tmp_frame,received_frame_length*sizeof(uint8_t));
			memset(received_tmp_frame,0,received_frame_length*sizeof(uint8_t));

			printf("from ESP %d bytes,type=%x,code=%x\n\r",
					received_frame_length, (uint8_t) received_cmd_type,
					received_cmd_code);
			switch (received_cmd_type) {
			case cmd_none:
				break;
				/////////////////////////////////////////////////
			case cmd_current:
				for (uint8_t i = 0; i < MAX_BROWSER_PAGE; i++)
					ActiveBrowserPage[i] = 0;
				cur_browser_profile = find_profile(received_frame);
				switch (received_cmd_code) {
				case readDashboard:

					counter_flag_1m_general = cur_time.Seconds;

					ready_msg_Dashboardpage(tmp_str2, cur_outsidelight,
							cur_doorstate, cur_client_number, cur_voltage,
							cur_current, cur_browser_profile);

					uart_transmit_frame(tmp_str2, cmd_current, readDashboard);

					flag_rtc_10s_general=1;
					counter_flag_10s_general=0;
//					if (cur_temperature[0] == (int16_t) 0x8fff) {
//						sprintf(tmp_str2, "-");
//					} else {
//						sprintf(tmp_str2, "%+4.1f", cur_temperature[0] / 10.0);
//					}
//
//					for (uint8_t i = 1; i < 8; i++) {
//						if (cur_temperature[i] == (int16_t) 0x8fff) {
//							sprintf(tmp_str2, "%s,-", tmp_str2);
//						} else {
//							sprintf(tmp_str2, "%s,%+4.1f", tmp_str2,
//									cur_temperature[i] / 10.0);
//						}
//					}
//					sprintf(tmp_str2, "%s,%02d:%02d:%02d", tmp_str2, cur_time.Hours,
//							cur_time.Minutes, cur_time.Seconds);
//					uart_transmit_frame(tmp_str2, cmd_event, TemperatureEvent);


					ActiveBrowserPage[DashboardActivePage] = 1;
					break;
				case LEDS1page:
					ready_msg_LEDpage(tmp_str2, S1_LED_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current, LEDS1page);
					ActiveBrowserPage[LEDActivePage] = 1;
					break;
				case LEDS2page:
					ready_msg_LEDpage(tmp_str2, S2_LED_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current, LEDS2page);
					ActiveBrowserPage[LEDActivePage] = 1;
					break;
				case TimePositionpage:
					ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
							cur_sunset, cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current,
					TimePositionpage);
					ActiveBrowserPage[TimePositionActivePage] = 1;
					break;
				case Temperaturepage:
					ready_msg_Temperaturepage(tmp_str2, TempLimit_Value,
							HYSTERESIS_Value, TEC_STATE_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current, Temperaturepage);
					ActiveBrowserPage[TemperatureActivePage] = 1;
					break;
				case Useroptionpage:
					ready_msg_Useroptionpage(tmp_str2, profile_user_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current, Useroptionpage);
					ActiveBrowserPage[UserOptionActivePage] = 1;
					break;
				case Wifipage:
					ready_msg_Wifipage(tmp_str2, cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_current, Wifipage);
					ActiveBrowserPage[WifiActivePage] = 1;
					break;
				case Upgradepage:
					ActiveBrowserPage[UpgradeActivePage] = 1;
					break;
				case Passwordpage:
					uart_transmit_frame(tmp_str2, cmd_current, Passwordpage);
					ActiveBrowserPage[PasswordActivePage] = 1;
					break;

				}
				printf("to ESP32(%d)%s\n\r", strlen(tmp_str2), tmp_str2);

				break;
				/////////////////////////////////////////////////
			case cmd_default:
				cur_browser_profile = find_profile(received_frame);
				switch (received_cmd_code) {
				case LEDS1page:
					ready_msg_LEDpage(tmp_str2, S1_LED_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_default, LEDS1page);
					break;
				case LEDS2page:
					ready_msg_LEDpage(tmp_str2, S2_LED_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_default, LEDS2page);
					break;
				case TimePositionpage:
					ready_msg_TimePositionpage(tmp_str2, cur_sunrise,
							cur_sunset, cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_default,
					TimePositionpage);

					break;
				case Temperaturepage:
					ready_msg_Temperaturepage(tmp_str2, TempLimit_Value,
							HYSTERESIS_Value, TEC_STATE_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_default, Temperaturepage);
					break;
				case Useroptionpage:
					ready_msg_Useroptionpage(tmp_str2, profile_user_Value,
							cur_browser_profile);
					uart_transmit_frame(tmp_str2, cmd_default, Useroptionpage);
//					ActiveBrowserPage[UserOptionActivePage] = 1;
				case Wifipage:
					uart_transmit_frame(tmp_str2, cmd_default, Wifipage);

					break;
				case Passwordpage:
					uart_transmit_frame(tmp_str2, cmd_default, Passwordpage);
					break;
				}
				break;
				/////////////////////////////////////////////////
			case cmd_submit:
				strncpy(extracted_text, (char*) &received_frame[6],received_msg_length);
				extracted_text[received_msg_length] = '\0';
				printf("from ESP:%s\n\r", extracted_text);
				ptr_splitted[0] = strtok(extracted_text, ",");
				index_ptr = 0;
				while (ptr_splitted[index_ptr] != NULL) {
					index_ptr++;
					ptr_splitted[index_ptr] = strtok(NULL, ",");
				}

				switch (received_cmd_code) {
				case LEDS1page:
					uart_transmit_frame("OK", cmd_submit, LEDS1page);
					S1_LED_Value.TYPE_Value = atoi(ptr_splitted[0]);
					S1_LED_Value.DAY_BRIGHTNESS_Value = atoi(ptr_splitted[1]);
					S1_LED_Value.DAY_BLINK_Value = (uint8_t) (atof(
							ptr_splitted[2]) * 10.0);
					S1_LED_Value.NIGHT_BRIGHTNESS_Value = atoi(ptr_splitted[3]);
					S1_LED_Value.NIGHT_BLINK_Value = (uint8_t) (atof(
							ptr_splitted[4]) * 10.0);

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
					sprintf(tmp_str2,
							"%04d-%02d-%02d,%02d:%02d:%02d,LED SET2,%s,%d,%f,%f,%d,%f,%f\n",
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
							tmp_str2);
					break;
				case LEDS2page:
					uart_transmit_frame("OK", cmd_submit, LEDS2page);

					S2_LED_Value.TYPE_Value = atoi(ptr_splitted[0]);
					S2_LED_Value.DAY_BRIGHTNESS_Value = atoi(ptr_splitted[1]);
					S2_LED_Value.DAY_BLINK_Value = (uint8_t) (atof(
							ptr_splitted[2]) * 10.0);
					S2_LED_Value.NIGHT_BRIGHTNESS_Value = atoi(ptr_splitted[3]);
					S2_LED_Value.NIGHT_BLINK_Value = (uint8_t) (atof(
							ptr_splitted[4]) * 10.0);

					if (S2_LED_Value.TYPE_Value == WHITE_LED
							&& S1_LED_Value.TYPE_Value == WHITE_LED) {
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
					sprintf(tmp_str2,
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
							tmp_str2);

					break;
				case TimePositionpage:
					uart_transmit_frame("OK", cmd_submit, TimePositionpage);

					split_date(ptr_splitted[0], &tmp_Date);
					split_time(ptr_splitted[1], &tmp_time);

					LAT_Value.deg = atoi(ptr_splitted[2]);
					printf("Lat:%s,%d\n\r", ptr_splitted[2], LAT_Value.deg);
					LAT_Value.min = atoi(ptr_splitted[3]);
					printf("Lat:%s,%d\n\r", ptr_splitted[3], LAT_Value.min);
					LAT_Value.second =
							(uint16_t) (atof(ptr_splitted[4]) * 100.0);
					printf("Lat:%s,%d\n\r", ptr_splitted[4], LAT_Value.second);
					LAT_Value.direction = *ptr_splitted[5];
					printf("Lat:%s,%c\n\r", ptr_splitted[5],
							LAT_Value.direction);

					LONG_Value.deg = atoi(ptr_splitted[6]);
					printf("Long:%s,%d\n\r", ptr_splitted[6], LONG_Value.deg);
					LONG_Value.min = atoi(ptr_splitted[7]);
					printf("Long:%s,%d\n\r", ptr_splitted[7], LONG_Value.min);
					LONG_Value.second = (uint16_t) (atof(ptr_splitted[8])
							* 100.0);
					printf("Long:%s,%d\n\r", ptr_splitted[8],
							LONG_Value.second);
					LONG_Value.direction = *ptr_splitted[9];
					printf("Long:%s,%c\n\r", ptr_splitted[9],
							LONG_Value.direction);
					UTC_OFF_Value = (int8_t) (atof(ptr_splitted[10]) * 10.0);
					printf("UTC:%s,%d\n\r", ptr_splitted[10], UTC_OFF_Value);

					S1_LED_Value.ADD_SUNRISE_Value =
							S2_LED_Value.ADD_SUNRISE_Value = (int8_t) (atof(
									ptr_splitted[11]) * 10.0);
					printf("sunrise:%s,%d\n\r", ptr_splitted[11],
							S2_LED_Value.ADD_SUNRISE_Value);
					S1_LED_Value.ADD_SUNSET_Value =
							S2_LED_Value.ADD_SUNSET_Value = (int8_t) (atof(
									ptr_splitted[12]) * 10.0);
					printf("sunset:%s,%d\n\r", ptr_splitted[12],
							S2_LED_Value.ADD_SUNSET_Value);

					Delay_Astro_calculation = 3;
					flag_rtc_1s_general = 1;

					tmp_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
					tmp_time.StoreOperation = RTC_STOREOPERATION_RESET;
					change_daylightsaving(&tmp_Date, &tmp_time, 0);

					HAL_RTC_SetDate(&hrtc, &tmp_Date, RTC_FORMAT_BIN);
					HAL_Delay(100);

					HAL_RTC_SetTime(&hrtc, &tmp_time, RTC_FORMAT_BIN);
					HAL_Delay(100);

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
					sprintf(tmp_str2,
							"%04d-%02d-%02d,%02d:%02d:%02d,POSITION,%02d %02d' %05.2f\" %c,%02d %02d' %05.2f\" %c\n",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							LAT_Value.deg, LAT_Value.min,
							LAT_Value.second / 100.0, LAT_Value.direction,
							LONG_Value.deg, LONG_Value.min,
							LONG_Value.second / 100.0, LONG_Value.direction);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					break;
				case Temperaturepage:
					uart_transmit_frame("OK", cmd_submit, Temperaturepage);

					memcpy(TempLimit_Value, tmp_limits, 6 * sizeof(RELAY_t));
					TEC_STATE_Value = (uint8_t) atoi(ptr_splitted[0]);
					printf("TEC:%s\n\r", ptr_splitted[0]);
					HYSTERESIS_Value = (int16_t) (atof(ptr_splitted[1]) * 10.0);
					printf("HYS:%s\n\r", ptr_splitted[1]);
					TempLimit_Value[ENVIROMENT_TEMP].TemperatureH =
							(int16_t) (atof(ptr_splitted[2]) * 10.0);
					printf("TH ENV:%s\n\r", ptr_splitted[2]);
					TempLimit_Value[ENVIROMENT_TEMP].TemperatureL =
							(int16_t) (atof(ptr_splitted[3]) * 10.0);
					TempLimit_Value[ENVIROMENT_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[4]));

					TempLimit_Value[CAM_TEMP].TemperatureH = (int16_t) (atof(
							ptr_splitted[5]) * 10.0);
					TempLimit_Value[CAM_TEMP].TemperatureL = (int16_t) (atof(
							ptr_splitted[6]) * 10.0);
					TempLimit_Value[CAM_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[7]));

					TempLimit_Value[CASE_TEMP].TemperatureH = (int16_t) (atof(
							ptr_splitted[8]) * 10.0);
					TempLimit_Value[CASE_TEMP].TemperatureL = (int16_t) (atof(
							ptr_splitted[9]) * 10.0);
					TempLimit_Value[CASE_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[10]));

					TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH =
							(int16_t) (atof(ptr_splitted[11]) * 10.0);
					TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL =
							(int16_t) (atof(ptr_splitted[12]) * 10.0);
					TempLimit_Value[MOTHERBOARD_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[13]));

					TempLimit_Value[TECIN_TEMP].TemperatureH = (int16_t) (atof(
							ptr_splitted[14]) * 10.0);
					TempLimit_Value[TECIN_TEMP].TemperatureL = (int16_t) (atof(
							ptr_splitted[15]) * 10.0);
					TempLimit_Value[TECIN_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[16]));

					TempLimit_Value[TECOUT_TEMP].TemperatureH = (int16_t) (atof(
							ptr_splitted[17]) * 10.0);
					TempLimit_Value[TECOUT_TEMP].TemperatureL = (int16_t) (atof(
							ptr_splitted[18]) * 10.0);
					TempLimit_Value[TECOUT_TEMP].active = (uint8_t) (atoi(
							ptr_splitted[19]));

					EE_WriteVariable(VirtAddVarTab[ADD_TEC_STATE],
							TEC_STATE_Value);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_HYS_Temp],
							HYSTERESIS_Value);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempH],
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_ENV_TempL],
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_ENV_active],
							TempLimit_Value[ENVIROMENT_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempH],
							TempLimit_Value[CAM_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CAM_TempL],
							TempLimit_Value[CAM_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CAM_active],
							TempLimit_Value[CAM_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempH],
							TempLimit_Value[CASE_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CASE_TempL],
							TempLimit_Value[CASE_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_CASE_active],
							TempLimit_Value[CASE_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_MB_TempH],
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_MB_TempL],
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_MB_active],
							TempLimit_Value[MOTHERBOARD_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempH],
							TempLimit_Value[TECIN_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_TempL],
							TempLimit_Value[TECIN_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECIN_active],
							TempLimit_Value[TECIN_TEMP].active);
					HAL_Delay(50);

					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempH],
							TempLimit_Value[TECOUT_TEMP].TemperatureH);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_TempL],
							TempLimit_Value[TECOUT_TEMP].TemperatureL);
					HAL_Delay(50);
					EE_WriteVariable(VirtAddVarTab[ADD_TECOUT_active],
							TempLimit_Value[TECOUT_TEMP].active);
					HAL_Delay(50);

					if (TEC_STATE_Value)
						sprintf(tmp_str,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC ENABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					else
						sprintf(tmp_str2,
								"%04d-%02d-%02d,%02d:%02d:%02d,TEC DISABLE\n",
								cur_Date.Year + 2000, cur_Date.Month,
								cur_Date.Date, cur_time.Hours, cur_time.Minutes,
								cur_time.Seconds);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,Hysteresis,%f",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							HYSTERESIS_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureH
									/ 10.0,
							TempLimit_Value[ENVIROMENT_TEMP].TemperatureL
									/ 10.0,
							TempLimit_Value[ENVIROMENT_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[CAM_TEMP].TemperatureH / 10.0,
							TempLimit_Value[CAM_TEMP].TemperatureL / 10.0,
							TempLimit_Value[CAM_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[CASE_TEMP].TemperatureH / 10.0,
							TempLimit_Value[CASE_TEMP].TemperatureL / 10.0,
							TempLimit_Value[CASE_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureH
									/ 10.0,
							TempLimit_Value[MOTHERBOARD_TEMP].TemperatureL
									/ 10.0,
							TempLimit_Value[MOTHERBOARD_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[TECIN_TEMP].TemperatureH / 10.0,
							TempLimit_Value[TECIN_TEMP].TemperatureL / 10.0,
							TempLimit_Value[TECIN_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);

					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,RELAY1,%f,%f,%d",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							TempLimit_Value[TECOUT_TEMP].TemperatureH / 10.0,
							TempLimit_Value[TECOUT_TEMP].TemperatureL / 10.0,
							TempLimit_Value[TECOUT_TEMP].active);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					break;
				case Useroptionpage:
					uart_transmit_frame("OK", cmd_submit, Useroptionpage);
					profile_user_Value = 1;
					for (uint8_t i = 0; i < 7; i++)
						profile_user_Value |= atoi(ptr_splitted[i]) << (i + 1);
					printf("profile_user_Value=%x\n\r", profile_user_Value);
					if (profile_user_Value & 0x02) {
						profile_active[USER_PROFILE][2] = 1;
						profile_active[USER_PROFILE][3] = 1;
					} else {
						profile_active[USER_PROFILE][2] = 0;
						profile_active[USER_PROFILE][3] = 0;
					}

					if (profile_user_Value & 0x04) {
						profile_active[USER_PROFILE][0] = 1;
						profile_active[USER_PROFILE][1] = 1;
					} else {
						profile_active[USER_PROFILE][0] = 0;
						profile_active[USER_PROFILE][1] = 0;
					}

					if (profile_user_Value & 0x08) {
						profile_active[USER_PROFILE][5] = 1;
						profile_active[USER_PROFILE][6] = 1;
					} else {
						profile_active[USER_PROFILE][5] = 0;
						profile_active[USER_PROFILE][6] = 0;
					}
					if (profile_user_Value & 0x10) {
						profile_active[USER_PROFILE][7] = 1;
					} else {
						profile_active[USER_PROFILE][7] = 0;
					}
					if (profile_user_Value & 0x20) {
						profile_active[USER_PROFILE][11] = 1;
						profile_active[USER_PROFILE][12] = 1;
					} else {
						profile_active[USER_PROFILE][11] = 0;
						profile_active[USER_PROFILE][12] = 0;
					}

					if (profile_user_Value & 0x40) {
						profile_active[USER_PROFILE][13] = 1;
					} else {
						profile_active[USER_PROFILE][13] = 0;
					}

					if (profile_user_Value & 0x80) {
						profile_active[USER_PROFILE][8] = 1;
					} else {
						profile_active[USER_PROFILE][8] = 0;
					}
					EE_WriteVariable(VirtAddVarTab[ADD_PROFILE_USER],
							profile_user_Value);
					sprintf(tmp_str,
							"%04d-%02d-%02d,%02d:%02d:%02d,User Option,%x",
							cur_Date.Year + 2000, cur_Date.Month, cur_Date.Date,
							cur_time.Hours, cur_time.Minutes, cur_time.Seconds,
							profile_user_Value);
					r_logparam = Log_file(SDCARD_DRIVE, PARAMETER_FILE,
							tmp_str2);
					break;
				case Wifipage:
					uart_transmit_frame("OK", cmd_submit, Wifipage);
					break;
				case Passwordpage:
					uart_transmit_frame("OK", cmd_submit, Passwordpage);
					break;
				}
				break;
				/////////////////////////////////////////////////
			case cmd_event:
				if(received_cmd_code!=DataUploadEvent)
				{
					strncpy(extracted_text, (char*) &received_frame[6],	received_msg_length);
					extracted_text[received_msg_length] = '\0';
					printf("cmd_event from ESP:%s\n\r", extracted_text);
					ptr_splitted[0] = strtok(extracted_text, ",");
					index_ptr = 0;
					while (ptr_splitted[index_ptr] != NULL) {
						index_ptr++;
						ptr_splitted[index_ptr] = strtok(NULL, ",");
					}
				}
				switch (received_cmd_code) {
				case ClientsEvent:
					cur_client_number = atoi(ptr_splitted[0]);
					printf("clients from esp:%d\n\r", cur_client_number);
					break;
				case WifiPageEvent:
					strcpy(cur_wifi.ssid, ptr_splitted[0]);
					for (uint8_t i = strlen(cur_wifi.ssid); i < 10; i++)
						cur_wifi.ssid[i] = ' ';
					cur_wifi.ssid[10] = 0;

					strcpy(cur_wifi.pass, ptr_splitted[1]);
					for (uint8_t i = strlen(cur_wifi.pass); i < 10; i++)
						cur_wifi.pass[i] = ' ';
					cur_wifi.pass[10] = 0;

					strcpy(cur_wifi.ip, ptr_splitted[2]);
					cur_wifi.ip[15] = 0;
					strcpy(cur_wifi.gateway, ptr_splitted[3]);
					cur_wifi.gateway[15] = 0;
					strcpy(cur_wifi.netmask, ptr_splitted[4]);
					cur_wifi.netmask[15] = 0;
					cur_wifi.ssidhidden = atoi(ptr_splitted[5]);
					cur_wifi.maxclients = atoi(ptr_splitted[6]);
					cur_wifi.txpower = atoi(ptr_splitted[7]);

					break;
				case IamreadyEvent:
					printf("I am ready from ESP32\n\r");
					uart_transmit_frame("", cmd_event, ThanksEvent);
					printf("Send Thanks to ESP32\n\r");
					break;
				case ThanksEvent:
					printf("Thanks from ESP32\n\r");
					HAL_Delay(100);
					uart_transmit_frame("", cmd_current, Wifipage);
					ESP32ready=1;
					break;
				case StartUploadEvent:
					strcpy(filename_chars,ptr_splitted[0]);
					strcpy(version_chars,ptr_splitted[1]);
					printf("Start Uploading file:%s(v:%s)\n\r",filename_chars,version_chars);
					fr=f_open(&myfile, "0:/boot.ini", FA_READ);
					if (fr==FR_NO_FILE) {
						/////////////generate boot.ini file
						printf("FatFs error code: %u\n\r");
						/////////////////////////////////
					}
					else if(fr==FR_OK)
					{

						f_gets(readline, 100, &myfile);
						readline[strlen(readline) - 1] = 0;
						firmware_version = atol(readline);
						printf("firmware version:%d\n\r",firmware_version);

						f_gets(readline, 100, &myfile);
						readline[strlen(readline) - 1] = 0;
						sprintf(firmware_path, "%s", readline);
						printf("firmware path:%s\n\r",firmware_path);

						f_gets(readline, 100, &myfile);
						readline[strlen(readline) - 1] = 0;
						firmware_checksum = atol(readline);
						printf("firmware checksum:%d\n\r",firmware_checksum);
						f_close(&myfile);

						if (f_open(&myfile,"0:/tmp_firm.bin", FA_WRITE | FA_CREATE_ALWAYS)!= FR_OK) {
							uart_transmit_frame("", cmd_error, UploadError);
							upload_started=0;
						}
						else
						{
							uart_transmit_frame("create file", cmd_event, StartUploadEvent);
							upload_started=1;
						}
						total_firmwaresize=0;
						f_close(&myfile);

					}
					else
					{
						uart_transmit_frame("file create err", cmd_error, UploadError);
						f_close(&myfile);
					}
					break;
				case DataUploadEvent:
					if(upload_started)
					{
						extracted_data=(uint8_t *)malloc(received_msg_length* sizeof(uint8_t));
						memset(extracted_data,0,received_msg_length* sizeof(uint8_t));
						memcpy(extracted_data,&received_frame[6],received_msg_length * sizeof(uint8_t));

						if ((fr = f_open(&myfile, "0:/tmp_firm.bin", FA_WRITE | FA_OPEN_APPEND))!= FR_OK) {
							printf("error open to file %d\n\r",fr);
							uart_transmit_frame("open err", cmd_error, UploadError);
							upload_started=0;
						}
						else
						{
							if ((fr = f_write(&myfile, extracted_data, (UINT)received_msg_length, (UINT*) &byteswritten))!= FR_OK) {
								printf("error write to file %d\n\r",fr);
								uart_transmit_frame("write err", cmd_error, UploadError);
								upload_started=0;
								f_close(&myfile);
							}
							else
							{
								f_close(&myfile);

								total_firmwaresize+=(size_t)received_msg_length;
								sprintf(tmp_str2,"received pack:%lu B",total_firmwaresize);
								uart_transmit_frame(tmp_str2, cmd_event, DataUploadEvent);
								printf("%s,write:%lu\n\r",tmp_str2,byteswritten);
								if(total_firmwaresize>700 && total_firmwaresize <3000)
								{
									for(uint16_t i=0;i<20;i++)
										printf("%x ",extracted_data[0x400-total_firmwaresize+byteswritten+i]);
								}
								printf("\n\r");
								HAL_Delay(50);
							}
						}



//						if((fr=file_write("0:/tmp_firm.bin",extracted_data,(uint32_t)received_msg_length))!=FR_OK)
//						{
//							printf("error write to file %d\n\r",fr);
//							uart_transmit_frame("write err", cmd_error, UploadError);
//							upload_started=0;
//						}
//						else
//						{
//							total_firmwaresize+=(size_t)received_msg_length;
//							sprintf(tmp_str2,"received pack:%lu B",total_firmwaresize);
//							uart_transmit_frame(tmp_str2, cmd_event, DataUploadEvent);
//							printf("%s\n\r",tmp_str2);
//						}
//
						free(extracted_data);
					}
					else
					{
						uart_transmit_frame("upload not start", cmd_error, UploadError);
						printf("ERR:data received while it is not started\n\r");
					}
				break;
				case EndUploadEvent:
					if(upload_started)
					{
						printf("End Uploading file and reboot firmware:%lu\n\r",total_firmwaresize);
						f_unlink(firmware_path);
						if(f_rename("0:/tmp_firm.bin", firmware_path)==FR_OK)
						{
							sharedmem = FORCE_WRITE_FROM_SD;
							Peripherials_DeInit();
							HAL_Delay(100);
							SCB->AIRCR = 0x05FA0000 | (uint32_t) 0x04; //system reset
							while (1)		;
						}
						else
						{
							printf("copy tmp file to %s Error\n\r",firmware_path);
						}
						upload_started=0;

					}
					else
					{
						uart_transmit_frame("", cmd_error, UploadError);
						printf("ERR:data upload ended  while it is not started\n\r");
					}
					break;
				}
				break;
				/////////////////////////////////////////////////
			case cmd_error:
				break;

			}
		}

		///////////send i am ready to ESP32///////////////////
		if(!ESP32ready && sendready_tick)
		{
			uart_transmit_frame("", cmd_event, IamreadyEvent);
			printf("send I am ready to ESP32\n\r");
			sendready_tick=0;
		}
		///////////////////////////////////////////frame error while uploading firmware////////////////////////////////////////////////////////
		if(frame_error && upload_started)
		{
			frame_error=0;
			upload_started=0;
			uart_transmit_frame("", cmd_error, UploadError);
			printf("upload started & frame error\n\r");
		}
		///////////////////////////////////////////end ESP32////////////////////////////////////////////////////////

#endif
		/////////////////////////////////////////check MB<->MICRO///////////////////////////////////////////////////
		if (received_valid_MB) {
			received_valid_MB = 0;
			memcpy(received_frame_MB,received_tmp_frame_MB,MAX_SIZE_FRAME_MB*sizeof(uint8_t));
			memset(received_tmp_frame_MB,0,MAX_SIZE_FRAME_MB*sizeof(uint8_t));
			uint8_t ch=received_frame_MB[INDEX_CHANNEL_BYTE];
			switch(received_frame_MB[INDEX_TYPE_BYTE])
			{
			case TYPE_MB_TEMPERATURE:
				if (cur_temperature[ch]==(int16_t)0x8fff) {
					transmit_value[0]=1;
					uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					reinit_i2c(&hi2c3);
				}
				else
				{
					transmit_value[0]=cur_temperature[ch]>>8;
					transmit_value[1]=cur_temperature[ch]&0xff;
					uart_transmit_frame_MB(&huart3, 2, TYPE_MB_TEMPERATURE, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
				}
				break;
			case TYPE_MB_LIGHT:
				if(ch==0)
				{
					if (cur_insidelight==(uint16_t)0xffff) {
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						transmit_value[0]=cur_insidelight>>8;
						transmit_value[1]=cur_insidelight&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_LIGHT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}

				}
				else
				{
					if (cur_outsidelight==(uint16_t)0xffff) {
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						transmit_value[0]=cur_outsidelight>>8;
						transmit_value[1]=cur_outsidelight&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_LIGHT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}
				}
				break;
			case TYPE_MB_VOLTAGE:
					if (cur_voltage[ch]==-1.0) {
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						tmp_uint16=(uint16_t)(cur_voltage[ch]*100.0);
						transmit_value[0]=tmp_uint16>>8;
						transmit_value[1]=tmp_uint16&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_VOLTAGE, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}
				break;
			case TYPE_MB_CURRENT:
				if (cur_current[ch]==-1.0) {
					transmit_value[0]=1;
					uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					reinit_i2c(&hi2c3);
				}
				else
				{
					tmp_uint16=(uint16_t)(cur_current[ch]*100.0);
					transmit_value[0]=tmp_uint16>>8;
					transmit_value[1]=tmp_uint16&0xff;
					uart_transmit_frame_MB(&huart3, 2, TYPE_MB_CURRENT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
				}
				break;
			case TYPE_MB_LED:
				if(ch==0)
				{
					tmp_uint16=((uint16_t)received_frame_MB[INDEX_VALUE_BYTE+1]<<8)+received_frame_MB[INDEX_VALUE_BYTE+2];
					if(pca9632_setbrighnessblinking(LEDS1,received_frame_MB[INDEX_VALUE_BYTE],	(double)tmp_uint16/10.0)!=PCA9632_OK)
					{
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					}
					else
					{
						transmit_value[0]=0;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					}
				}
				else if (ch==1)
				{
					tmp_uint16=((uint16_t)received_frame_MB[INDEX_VALUE_BYTE+1]<<8)+received_frame_MB[INDEX_VALUE_BYTE+2];
					if(pca9632_setbrighnessblinking(LEDS2,received_frame_MB[INDEX_VALUE_BYTE],	(double)tmp_uint16/10.0)!=PCA9632_OK)
					{
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					}
					else
					{
						transmit_value[0]=0;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					}

				}
				break;
			case TYPE_MB_FAN:
				if(received_frame_MB[INDEX_VALUE_BYTE])
				{
					FAN_ON();

				}
				else
				{
					FAN_OFF();
				}
				transmit_value[0]=0;
				uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
				break;
			}
		}
		//////////////////////////////////////////end MB<-> MICRO////////////////////////////////////////////////


#if __LWIP__
		MX_LWIP_Process();
#endif
#if 0
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
#endif
#if 0
		MX_USB_HOST_Process();
#endif
#if !(__DEBUG__)
	HAL_IWDG_Refresh(&hiwdg);
#endif
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
#if 0

    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
#endif

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
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
void Error_Handler(void)
{
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
