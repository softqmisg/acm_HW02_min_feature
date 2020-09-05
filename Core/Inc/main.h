/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ALERT2_TMP275_Pin GPIO_PIN_3
#define ALERT2_TMP275_GPIO_Port GPIOE
#define ALERT1_TMP275_Pin GPIO_PIN_2
#define ALERT1_TMP275_GPIO_Port GPIOE
#define DISP_CMD_Pin GPIO_PIN_1
#define DISP_CMD_GPIO_Port GPIOE
#define DISP_RST_Pin GPIO_PIN_0
#define DISP_RST_GPIO_Port GPIOE
#define DISP_SID_Pin GPIO_PIN_8
#define DISP_SID_GPIO_Port GPIOB
#define DISP_SCK_Pin GPIO_PIN_5
#define DISP_SCK_GPIO_Port GPIOB
#define BTN2_Pin GPIO_PIN_4
#define BTN2_GPIO_Port GPIOB
#define BTN1_Pin GPIO_PIN_3
#define BTN1_GPIO_Port GPIOB
#define ALERT3_TMP275_Pin GPIO_PIN_4
#define ALERT3_TMP275_GPIO_Port GPIOE
#define ALERT0_TMP275_Pin GPIO_PIN_5
#define ALERT0_TMP275_GPIO_Port GPIOE
#define ALERT4_TMP275_Pin GPIO_PIN_6
#define ALERT4_TMP275_GPIO_Port GPIOE
#define DISP_LEDA_Pin GPIO_PIN_9
#define DISP_LEDA_GPIO_Port GPIOB
#define DISP_CS_Pin GPIO_PIN_7
#define DISP_CS_GPIO_Port GPIOB
#define BTN5_Pin GPIO_PIN_6
#define BTN5_GPIO_Port GPIOB
#define BTN4_Pin GPIO_PIN_15
#define BTN4_GPIO_Port GPIOG
#define BTN3_Pin GPIO_PIN_12
#define BTN3_GPIO_Port GPIOG
#define uSD_Detect_Pin GPIO_PIN_1
#define uSD_Detect_GPIO_Port GPIOD
#define ESP32_EN_Pin GPIO_PIN_4
#define ESP32_EN_GPIO_Port GPIOD
#define TEC_ONOFF_Pin GPIO_PIN_13
#define TEC_ONOFF_GPIO_Port GPIOF
#define TEC_CURDIR_Pin GPIO_PIN_12
#define TEC_CURDIR_GPIO_Port GPIOF
#define USB_PWR_EN_Pin GPIO_PIN_11
#define USB_PWR_EN_GPIO_Port GPIOE
#define INT_VCNL4200_Pin GPIO_PIN_14
#define INT_VCNL4200_GPIO_Port GPIOE
#define RESET_MB_Pin GPIO_PIN_15
#define RESET_MB_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
