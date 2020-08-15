/*
 * st7565-config.h
 *
 *  Created on: Aug 12, 2020
 *      Author: mehdi
 */

#ifndef INC_ST7565_CONFIG_H_
#define INC_ST7565_CONFIG_H_

// Setup for ST7565R in SPI mode
#include "main.h"
#include "tim.h"

#define DISP_BACK_TIMER		htim4
#define DISP_BACK_CHANNEL	TIM_CHANNEL_4
////////////////////////////////////////////////////////
#define ST7567_CS_PIN		DISP_CS_Pin
#define ST7567_CS_PORT		DISP_CS_GPIO_Port
#define ST7567_CMD_PIN		DISP_CMD_Pin
#define ST7567_CMD_PORT		DISP_CMD_GPIO_Port
#define ST7567_RST_PIN		DISP_RST_Pin
#define ST7567_RST_PORT		DISP_RST_GPIO_Port
#define ST7567_SID_PIN		DISP_SID_Pin
#define ST7567_SID_PORT		DISP_SID_GPIO_Port
#define ST7567_SCLK_PIN		DISP_SCK_Pin
#define ST7567_SCLK_PORT	DISP_SCK_GPIO_Port
///////////////////////////////////////////////////////////////////////////////////////////////////
/** Screen width in pixels (tested with 128) */
#define SCREEN_WIDTH 128
/** Screen height in pixels (tested with 64) */
#define SCREEN_HEIGHT 64

/** Define this if your screen is incorrectly shifted by 4 pixels */
//#define ST7565_REVERSE

/** By default we only write pages that have changed.  Undefine this
    if you want less/faster code at the expense of more SPI operations. */
#undef ST7565_DIRTY_PAGES 1


#endif /* INC_ST7565_CONFIG_H_ */
