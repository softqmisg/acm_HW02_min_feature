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
#define VER	"20230805"
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
		sprintf(tmp_str, "%s     C=%4.2fA", tmp_str, current[3]);
	else
		sprintf(tmp_str, "%s     C=---", tmp_str);
	text_cell(pos_, 3, tmp_str, Tahoma8, CENTER_ALIGN, 0, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	create_cell(0, pos_[0].y1, 25, (pos_[3].y2 - pos_[0].y1) + 1, 4, 1, 1,
			pos_);
	text_cell(pos_, 0, "7.0", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 1, "12", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 2, "3.3", Tahoma8, CENTER_ALIGN, 1, 1);
	text_cell(pos_, 3, "MB ", Tahoma8, CENTER_ALIGN, 1, 1);
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
	/////////////////////////Motherboard Start/////////////////////
	MB_ON();
	MB_PW_START();
	/////////////////////////transceiver PC<->ESP32/////////////////////////////

	////////////////////////////////////////////////////////////////
	HAL_UART_Receive_IT(&huart3, &received_byte_MB, 1);

	//////////////////////init LCD//////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);
	////////////////////////////////////////////////
//	create_form5(1);
//	create_form5_5(1);

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
	HAL_Delay(300);
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
	HAL_Delay(300);
	glcd_blank();
	create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
	glcd_refresh();
	///////////////////////////RTC interrupt enable///////////////////////////////////////////////////
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS)
			!= HAL_OK) {
		Error_Handler();
	}

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
		/////////////////////////////////////State Machine///////////////////////////////////////////////////////////

		/////////////////////////////////////////check MB<->MICRO///////////////////////////////////////////////////
		if (received_valid_MB) {
			received_valid_MB = 0;
			glcd_blank();
			create_cell(0, 0, 128, 64, 1, 1, 1, pos_);
			glcd_refresh();

			memcpy(received_frame_MB,received_tmp_frame_MB,MAX_SIZE_FRAME_MB*sizeof(uint8_t));
			memset(received_tmp_frame_MB,0,MAX_SIZE_FRAME_MB*sizeof(uint8_t));
			uint8_t ch=received_frame_MB[INDEX_CHANNEL_BYTE];
			switch(received_frame_MB[INDEX_TYPE_BYTE])
			{
			case TYPE_MB_TEMPERATURE:
				sprintf(tmp_str,"read temp%d",ch);
				draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);
				tmp275_init(ch);
				if (tmp275_readTemperature(i, &cur_temperature[i]) != HAL_OK) {
					cur_temperature[i] = (int16_t) 0x8fff;
				}

				if (cur_temperature[ch]==(int16_t)0x8fff) {
					sprintf(tmp_str,"T%d=---",ch);
					draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
					transmit_value[0]=1;
					uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					reinit_i2c(&hi2c3);
				}
				else
				{
					sprintf(tmp_str,"Temp%d=%.1f",ch,(double)cur_temperature[ch]/10.0);
					draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
					transmit_value[0]=cur_temperature[ch]>>8;
					transmit_value[1]=cur_temperature[ch]&0xff;
					uart_transmit_frame_MB(&huart3, 2, TYPE_MB_TEMPERATURE, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);

				}
				glcd_refresh();

				break;
			case TYPE_MB_LIGHT:
				sprintf(tmp_str,"read Light%d",ch);
				draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);
				if(ch==0)
				{
					vcnl4200_init();
					if (vcnl4200_ps(&cur_insidelight) != HAL_OK) {
						cur_insidelight = 0xffff;
						reinit_i2c(&hi2c3);
					}

					if (cur_insidelight==(uint16_t)0xffff) {
						sprintf(tmp_str,"inside=---");
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						sprintf(tmp_str,"inside=%d",cur_insidelight);
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						transmit_value[0]=cur_insidelight>>8;
						transmit_value[1]=cur_insidelight&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_LIGHT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}

				}
				else
				{
					veml6030_init();
					if (veml6030_als(&cur_outsidelight) != HAL_OK) {
						cur_outsidelight = 0xffff;
					}
					if (cur_outsidelight==(uint16_t)0xffff) {
						sprintf(tmp_str,"outside=---",ch);
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						sprintf(tmp_str,"outside=%d",cur_outsidelight);
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						transmit_value[0]=cur_outsidelight>>8;
						transmit_value[1]=cur_outsidelight&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_LIGHT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}
				}
				glcd_refresh();
				break;
			case TYPE_MB_VOLTAGE:
					sprintf(tmp_str,"read Voltage%d",ch);
					draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);
					if (cur_voltage[ch]==-1.0) {
						sprintf(tmp_str,"Voltage%d=---",ch);
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						transmit_value[0]=1;
						uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
						reinit_i2c(&hi2c3);
					}
					else
					{
						sprintf(tmp_str,"Voltage%d=%.2f",ch,(double)cur_voltage[ch]);
						draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
						tmp_uint16=(uint16_t)(cur_voltage[ch]*100.0);
						transmit_value[0]=tmp_uint16>>8;
						transmit_value[1]=tmp_uint16&0xff;
						uart_transmit_frame_MB(&huart3, 2, TYPE_MB_VOLTAGE, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
					}
					glcd_refresh();
				break;
			case TYPE_MB_CURRENT:
				sprintf(tmp_str,"read Current%d",ch);
				draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);
				if (cur_current[ch]==-1.0) {
					sprintf(tmp_str,"Current%d=---",ch);
					draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
					transmit_value[0]=1;
					uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
					reinit_i2c(&hi2c3);
				}
				else
				{
					sprintf(tmp_str,"Current%d=%.2f",ch,(double)cur_current[ch]);
					draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
					tmp_uint16=(uint16_t)(cur_current[ch]*100.0);
					transmit_value[0]=tmp_uint16>>8;
					transmit_value[1]=tmp_uint16&0xff;
					uart_transmit_frame_MB(&huart3, 2, TYPE_MB_CURRENT, received_frame_MB[INDEX_CHANNEL_BYTE], transmit_value);
				}
				glcd_refresh();
				break;
			case TYPE_MB_LED:
				pca9632_init();
				sprintf(tmp_str,"Set Led%d",ch);
				draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);

				sprintf(tmp_str,"brightness=%d%%",received_frame_MB[INDEX_VALUE_BYTE]);
				draw_text(tmp_str, 5, 30, Tahoma8, 1, 0);
				sprintf(tmp_str,"blinking=%.1f s",(double)tmp_uint16/10.0);
				draw_text(tmp_str, 5, 50, Tahoma8, 1, 0);

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
				glcd_refresh();
				break;
			case TYPE_MB_FAN:

				if(received_frame_MB[INDEX_VALUE_BYTE])
				{
					sprintf(tmp_str,"set fan ON");
					draw_text(tmp_str, 5, 10, Tahoma8, 1, 0);
					FAN_ON();

				}
				else
				{
					sprintf(tmp_str,"set fan OFF");

					FAN_OFF();
				}
				transmit_value[0]=0;
				uart_transmit_frame_MB(&huart3, 1, TYPE_MB_ERROR,ch, transmit_value);
				glcd_refresh();
				break;
			}
		}
		//////////////////////////////////////////end MB<-> MICRO////////////////////////////////////////////////


#if __LWIP__
		MX_LWIP_Process();
#endif

	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {


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
