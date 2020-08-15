/*
 * st7565.c
 *
 *  Created on: Aug 12, 2020
 *      Author: mehdi
 */
#include "st7565-config.h"
#include "st7565.h"
unsigned char glcd_flipped = 0;

/** Global buffer to hold the current screen contents. */
// This has to be kept here because the width & height are set in
// st7565-config.h
unsigned char glcd_buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];

#ifdef ST7565_DIRTY_PAGES
unsigned char glcd_dirty_pages;
#endif
void st7567_usdelay(uint16_t delay) {
	uint16_t t = (uint16_t) delay;
	while (t) {
		__asm("nop");
		t--;
	}
}
/*
 * SPI emulation
 * send data
 */
void glcd_data(uint8_t data) {

	uint8_t bits = 0x80;
	ST7567_CMD_SET;
	ST7567_SCLK_RESET;
	ST7567_CS_RESET;

	while (bits) {
		ST7567_SCLK_RESET;
//		st7567_usdelay(1);
		if (data & bits)
			ST7567_SID_SET;
		else
			ST7567_SID_RESET;
		ST7567_SCLK_SET;
		//	st7567_usdelay(1);
		bits >>= 1;
	}
	ST7567_CS_SET;
	ST7567_CMD_RESET;

}

void glcd_command(uint8_t command) {

	uint8_t bits = 0x80;
	ST7567_CMD_RESET;
	ST7567_SCLK_RESET;
	ST7567_CS_RESET;

	while (bits) {
		ST7567_SCLK_RESET;
//		st7567_usdelay(1);
		if (command & bits)
			ST7567_SID_SET;
		else
			ST7567_SID_RESET;
		ST7567_SCLK_SET;
		//	st7567_usdelay(1);
		bits >>= 1;
	}
	ST7567_CS_SET;
	ST7567_CMD_SET;
}
/*
 *
 */
void glcd_pixel(unsigned char x, unsigned char y, unsigned char colour) {

	if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT)
		return;

//	// Real screen coordinates are 0-63, not 1-64.
//	x -= 1;
//	y -= 1;

	unsigned short array_pos = x + ((y / 8) * 128);

#ifdef ST7565_DIRTY_PAGES
#warning ** ST7565_DIRTY_PAGES enabled, only changed pages will be written to the GLCD **
    glcd_dirty_pages |= 1 << (array_pos / 128);
#endif

	if (colour) {
		glcd_buffer[array_pos] |= 1 << (y % 8);
	} else {
		glcd_buffer[array_pos] &= 0xFF ^ 1 << (y % 8);
	}
}

void glcd_blank() {
	// Reset the internal buffer
	for (int n = 1; n <= (SCREEN_WIDTH * SCREEN_HEIGHT / 8) - 1; n++) {
		glcd_buffer[n] = 0;
	}

	// Clear the actual screen
	for (int y = 0; y < 8; y++) {
		glcd_command(GLCD_CMD_SET_PAGE | y);

		// Reset column to 0 (the left side)
		glcd_command(GLCD_CMD_COLUMN_LOWER);
		glcd_command(GLCD_CMD_COLUMN_UPPER);

		// We iterate to 132 as the internal buffer is 65*132, not
		// 64*124.
		for (int x = 0; x < 132; x++) {
			glcd_data(0x00);
		}
	}
}

void glcd_refresh() {
	for (int y = 0; y < 8; y++) {

#ifdef ST7565_DIRTY_PAGES
        // Only copy this page if it is marked as "dirty"
        if (!(glcd_dirty_pages & (1 << y))) continue;
#endif

		glcd_command(GLCD_CMD_SET_PAGE | y);

		// Reset column to the left side.  The internal memory of the
		// screen is 132*64, we need to account for this if the display
		// is flipped.
		//
		// Some screens seem to map the internal memory to the screen
		// pixels differently, the ST7565_REVERSE define allows this to
		// be controlled if necessary.
#ifdef ST7565_REVERSE
		if (!glcd_flipped) {
#else
		if (glcd_flipped) {
#endif
			glcd_command(GLCD_CMD_COLUMN_LOWER | 4);
		} else {
			glcd_command(GLCD_CMD_COLUMN_LOWER);
		}
		glcd_command(GLCD_CMD_COLUMN_UPPER);

		for (int x = 0; x < 128; x++) {
			glcd_data(glcd_buffer[y * 128 + x]);
		}
	}

#ifdef ST7565_DIRTY_PAGES
    // All pages have now been updated, reset the indicator.
    glcd_dirty_pages = 0;
#endif
}
/*
 *
 */
void glcd_backlight(uint8_t brightness) {
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = (uint16_t) (__HAL_TIM_GET_AUTORELOAD(&DISP_BACK_TIMER)
			* brightness) / 100;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&DISP_BACK_TIMER, &sConfigOC,
	DISP_BACK_CHANNEL) != HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_PWM_Start(&DISP_BACK_TIMER, DISP_BACK_CHANNEL);
}
/*
 *
 */
void glcd_init() {

	// Select the chip
	ST7567_CS_RESET;

	ST7567_RST_RESET;

	// Datasheet says "wait for power to stabilise" but gives
	// no specific time!
	HAL_Delay(50);

	ST7567_RST_SET;
	//software reset
	glcd_command(GLCD_CMD_RESET);
	// Set LCD bias to 1/9th
	glcd_command(GLCD_CMD_BIAS_7);

	// Horizontal output direction (ADC segment driver selection)
	glcd_command(GLCD_CMD_HORIZONTAL_REVERSE);

	// Vertical output direction (common output mode selection)
	glcd_command(GLCD_CMD_VERTICAL_REVERSE);

	// The screen is the "normal" way up
	glcd_flipped = 0;

	// Set internal resistor.  A suitable middle value is used as
	// the default.
	glcd_command(GLCD_CMD_RESISTOR | 0x04);

	// Power control setting (datasheet step 7)
	// Note: Skipping straight to 0x7 works with my hardware.
	glcd_command(GLCD_CMD_POWER_CONTROL | 0x4);
	//	DelayMs(50);
	glcd_command(GLCD_CMD_POWER_CONTROL | 0x6);
	//	DelayMs(50);
	glcd_command(GLCD_CMD_POWER_CONTROL | 0x7);
	//	DelayMs(10);

	// Volume set (brightness control).  A middle value is used here
	// also.
	glcd_command(GLCD_CMD_VOLUME_MODE);
	glcd_command(0x10);

	// Reset start position to the top
	glcd_command(GLCD_CMD_DISPLAY_START);

	// Turn the display on
	glcd_command(GLCD_CMD_DISPLAY_ON);

	// Unselect the chip
	ST7567_CS_SET;
	glcd_blank();
	glcd_backlight(50);
	glcd_contrast(4, 16);
	glcd_flip_screen(0);
}

/*
 *  flip screen
 *  flip=1, R->L and B->T
 *  flip=0, L->R and T->B
 */
void glcd_flip_screen(unsigned char flip) {
	if (flip) {
		glcd_command(GLCD_CMD_HORIZONTAL_NORMAL);
		glcd_command(GLCD_CMD_VERTICAL_REVERSE);
		glcd_flipped = 0;
	} else {
		glcd_command(GLCD_CMD_HORIZONTAL_REVERSE);
		glcd_command(GLCD_CMD_VERTICAL_NORMAL);
		glcd_flipped = 1;
	}
}
/*
 * invert the color of screen
 * white on black=GLCD_CMD_DISPLAY_NORMAL
 * black on white=GLCD_CMD_DISPLAY_REVERSE
 */
void glcd_inverse_screen(unsigned char inverse) {
	if (inverse) {
		glcd_command(GLCD_CMD_DISPLAY_REVERSE);
	} else {
		glcd_command(GLCD_CMD_DISPLAY_NORMAL);
	}
}
/*
 *  fill buffer with test pattern and show it
 */
void glcd_test_card() {
	unsigned char p = 0xF0;

	for (int n = 1; n <= (SCREEN_WIDTH * SCREEN_HEIGHT / 8); n++) {
		glcd_buffer[n - 1] = p;

		if (n % 4 == 0) {
			unsigned char q = p;
			p = p << 4;
			p |= q >> 4;
		}
	}

	glcd_refresh();
}
/*
 *  set contrast of st7565
 */
void glcd_contrast(char resistor_ratio, char contrast) {
	if (resistor_ratio > 7 || contrast > 63)
		return;

	glcd_command(GLCD_CMD_RESISTOR | resistor_ratio);
	glcd_command(GLCD_CMD_VOLUME_MODE);
	glcd_command(contrast);
}
