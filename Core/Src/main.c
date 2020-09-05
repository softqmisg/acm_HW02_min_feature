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
#include "fonts/font_lucidaconsole.h"
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

uint8_t usart2_datardy = 0, usart3_datardy = 0;
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//	if(huart->Instance==USART2)
//		usart2_datardy=1;
//	else if(huart->Instance==USART3)
//		usart3_datardy=1;
//}
////////////////////////
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
	f_mount(0, "", 1);
	return fr;
}

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
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t als, white;
uint8_t buffer[2];
HAL_StatusTypeDef status;
double voltage, current, temperature[8];

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
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
  //MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 2 */
	/////////////////////////
  while(1);
	uint8_t backlight = 100;
	char tmp_str[100];
	RTC_TimeTypeDef cur_time;
	RTC_DateTypeDef cur_Date;
	cur_time.Hours = 12;
	cur_time.Minutes = 59;
	cur_time.Seconds = 0;
	uint32_t byteswritten;
	FIL myfile;
	FRESULT fr;
		uint8_t y = 0, x = 0;
		bounding_box_t ch_box;
//////////////////////////////////////
		RetargetInit(&huart3);

		//////////////////////init LCD
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);

	//////////////////////////init USB&SD/////////////////////////
	//wait for valid to USBH
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_RESET);
	while(!USBH_MSC_IsReady(&hUsbHostHS)) MX_USB_HOST_Process();
	if ((fr = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1)) != FR_OK) {
		printf("error mount USB\n\r");

	} else
	{
		printf("mount USB\n\r");
	}
	sprintf(tmp_str, "Write in USB\n\r");
	if (file_write_USB("1:/usb.txt", tmp_str) != FR_OK) {
		printf("error write usb\n\r");
	} else
	{
		printf("write usb ok\n\r");
	}

	////////////////////////////////////////////////////////////////////////
	if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
		printf("error mount SD\n\r");

	} else
	{
		printf("mount SD\n\r");
	}
	sprintf(tmp_str, "Write in SD\n\r");
	if (file_write_SD("0:/SDCARD.txt", tmp_str) != FR_OK) {
		printf("error write SD\n\r");
	} else
	{
		printf("write sd ok\n\r");
	}

	/////////////////////////tranciver PC<->ESP32/////////////////////////////
	char pc_data[] = "hello\n\r";
	//HAL_UART_Transmit(&huart3, (uint8_t*)"\033[0;0H", strlen("\033[0;0H"));
	//HAL_UART_Transmit(&huart3, (uint8_t*)"\033[2J", strlen("\033[2J"));

	HAL_GPIO_WritePin(ESP32_EN_GPIO_Port, ESP32_EN_Pin, GPIO_PIN_SET);
	while (1) {
		HAL_UART_Receive(&huart2, pc_data, 1, HAL_MAX_DELAY);
		//pc_data[0]='a'+x;
		//HAL_Delay(50);
		ch_box = draw_char(pc_data[0], x, y, Tahoma8, 0);
		x = ch_box.x2 + 1;
		if (x > 63) {
			x = 0;
			y = ch_box.y2 + 1;
			if (y > 127)
				y = 0;
		}
		glcd_refresh();
//		if(usart3_datardy)
//		{
//			usart3_datardy=0;
//			HAL_UART_Receive_IT(&huart3, pc_data, 1);
//			draw_char(pc_data[0], x, y, Tahoma8, 0);
//			x++;
//			if(x>63)
//			{
//				x=0;y+=11;if(y>127) y=0;
//			}
//		}
		//HAL_UART_Transmit(&huart3, (uint8_t *)pc_data, strlen(pc_data), HAL_MAX_DELAY);
//		draw_text(pc_data,y,0,Tahoma8,1,0);
//		y+=text_height(pc_data, Tahoma8);
//		if(y>128) y=0;
//		HAL_Delay(2000);
	}
	//////////////////////////load Logo/////////////////////////////////////////
		bmp_img img;
		bmp_img_read(&img, "logo.bmp");
		f_mount(0, "", 1);
		draw_bmp_h(0, 0, img.img_header.biWidth,img.img_header.biHeight , img.img_pixels, 1);
		bmp_img_free(&img);
		glcd_refresh();
		HAL_Delay(5000);

		////////////////////////keypad check//////////////////////////////////////////
		glcd_init(64,128);
		glcd_flip_screen(3);
	//	glcd_pixel(0, 0, 1);
	//	glcd_refresh();
	//	glcd_pixel(0, 1, 1);
	//	glcd_refresh();
	//	glcd_pixel(0, 15, 1);
	//	glcd_refresh();
	//	glcd_pixel(0, 10, 1);
	//	glcd_refresh();
	//	glcd_pixel(30, 10, 1);glcd_refresh();
	//	glcd_pixel(30, 15, 1);glcd_refresh();
	//	draw_box(0, 10, 30, 15, 1);glcd_refresh();
	//
	//	while(1);
		joystick_init(Key_ALL,Short_press|Long_press);
		uint8_t select=0;
		while(!select)
		{
			joystick_init(Key_ALL, Long_press);
			if(joystick_read(Key_DOWN,Short_press))
			{
				joystick_init(Key_DOWN,Short_press);
				draw_text("D",10,0 , Tahoma8, 0,0);

			}
			else
			{
				draw_text("D",10,0 , Tahoma8, 0,1);
			}


			if(joystick_read(Key_TOP,Short_press))
			{
				joystick_init(Key_TOP,Short_press);
				draw_text("T",20,0 , Tahoma8, 0,0);

			}
			else
			{
				draw_text("T",20,0 , Tahoma8, 0,1);

			}

			if(joystick_read(Key_LEFT,Short_press))
			{
				joystick_init(Key_LEFT,Short_press);
				draw_text("L",30,0 , Tahoma8, 0,0);

			}
			else
			{
				draw_text("L",30,0 , Tahoma8, 0,1);
			}

			if(joystick_read(Key_RIGHT,Short_press))
			{
				joystick_init(Key_RIGHT,Short_press);
				draw_text("R",40,0 , Tahoma8, 0,0);

			}
			else
			{
				draw_text("R",40,0 , Tahoma8, 0,1);

			}

			if(joystick_read(Key_ENTER,Short_press))
			{
				joystick_init(Key_ENTER,Short_press);
				draw_text("E",50,0 , Tahoma8, 0,0);
				select=1;

			}
			else
			{
				draw_text("E",50,0 , Tahoma8, 0,1);

			}
			glcd_refresh();
			HAL_Delay(500);

		}
		///////////////////////generate menu/////////////////////////////////////////////
		glcd_blank();
		char *menu[]={"Option 1","Option 2","Option 3","Option 4","Option 5","Option 6"};
		for(uint8_t op=0;op<5;op++)
		{
			draw_text(menu[op], 0,op*11 , Tahoma8, 1,0);
		}
		uint8_t cur_op=0;
		draw_text(menu[cur_op], 0,cur_op*11 , Tahoma8, 1,1);
		cur_op++;
		glcd_refresh();
		joystick_init(Key_ALL,Both_press);
		do
		{
			joystick_init(Key_ALL, Long_press);
			if(joystick_read(Key_DOWN, Short_press))
			{
				joystick_init(Key_DOWN,Short_press);
				if(cur_op>0)
					draw_text(menu[cur_op-1], 0,(cur_op-1)*11 , Tahoma8, 1,0);
				if(cur_op>4)
					cur_op=0;
				draw_text(menu[cur_op], 0,cur_op*11 , Tahoma8, 1,1);
				cur_op++;
				glcd_refresh();

			}
		}while(1);
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
		sprintf(tmp_str,"MANUFACTURE INA321(1):%c%c\n\r",buffer[0],buffer[1]);
		if(file_write("test.txt",tmp_str)!=FR_OK)
		{
			HAL_Delay(1000);
		}
		status = ina3221_readreg(INA3221_LED_BASEADDRESS, INA3221_DIE_ID, buffer);
		sprintf(tmp_str,"DIE INA321(1):%c%c\n\r",buffer[0],buffer[1]);
		if(file_write("test.txt",tmp_str)!=FR_OK)
		{
			HAL_Delay(1000);
		}
		status = ina3221_readreg(INA3221_TEC_BASEADDRESS, INA3221_MANUFACTURE_ID,
				buffer);
		sprintf(tmp_str,"MANUFACTURE INA321(2):%c%c\n\r",buffer[0],buffer[1]);
		if(file_write("test.txt",tmp_str)!=FR_OK)
		{
			HAL_Delay(1000);
		}
		status = ina3221_readreg(INA3221_TEC_BASEADDRESS, INA3221_DIE_ID, buffer);
		sprintf(tmp_str,"DIE INA321(2):%c%c\n\r",buffer[0],buffer[1]);
		if(file_write("test.txt",tmp_str)!=FR_OK)
		{
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
				if(status==HAL_OK)
				{
					sprintf(tmp_str,"tmp(0x%x):%f\n\r",ch+TMP275_BASEADDRESS,temperature[ch]);

				}
				else
				{
					sprintf(tmp_str,"tmp(0x%x):ERROR\n\r",ch+TMP275_BASEADDRESS);

				}
				if(file_write("test.txt",tmp_str)!=FR_OK)
				{
					HAL_Delay(1000);
				}
				HAL_Delay(100);
			}
			//////////////////////////////////////////////////////////
			status = vcnl4200_read((uint8_t) VCNL4200_ALS_Data, &als);
			if(status==VCNL4200_OK)
			{
				sprintf(tmp_str,"als:%d\n\r",als);

			}
			else
			{
				sprintf(tmp_str,"als:ERROR\n\r");

			}
			if(file_write("test.txt",tmp_str)!=FR_OK)
			{
				HAL_Delay(1000);
			}
			status = vcnl4200_read((uint8_t) VCNL4200_WHITE_Data, &white);
			if(status==VCNL4200_OK)
			{
				sprintf(tmp_str,"white:%d\n\r",white);

			}
			else
			{
				sprintf(tmp_str,"white:ERROR\n\r");

			}
			if(file_write("test.txt",tmp_str)!=FR_OK)
			{
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
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
