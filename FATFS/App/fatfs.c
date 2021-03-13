/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */
uint8_t retUSBH;    /* Return value for USBH */
char USBHPath[4];   /* USBH logical drive path */
FATFS USBHFatFS;    /* File system object for USBH logical drive */
FIL USBHFile;       /* File object for USBH */

/* USER CODE BEGIN Variables */
extern RTC_HandleTypeDef hrtc;
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);
  /*## FatFS: Link the USBH driver ###########################*/
  retUSBH = FATFS_LinkDriver(&USBH_Driver, USBHPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
	RTC_TimeTypeDef c_time;
	RTC_DateTypeDef c_date;
	HAL_RTC_GetTime(&hrtc,&c_time,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc,&c_date,RTC_FORMAT_BIN);
	c_date.Year+=20; //Year origin from the 1980 (0..127, e.g. 37 for 2017), GetDat year is from 2000(.c_date.year=21:2021)
return	((DWORD)c_date.Year << 25 | (DWORD)c_date.Month << 21 | (DWORD)c_date.Date << 16 | (DWORD) c_time.Hours <<11 | (DWORD) c_time.Minutes <<5 | (DWORD) (c_time.Seconds>>1)<<0);

  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */
void MX_FATFS_DeInit(void)
{
	FATFS_UnLinkDriver(SDPath);
	FATFS_UnLinkDriver(USBHPath);
	BSP_SD_DeInit();
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
