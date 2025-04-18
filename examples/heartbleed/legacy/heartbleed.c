// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>

#include <gpio.h>
#include <lcd_st7735.h>
#include <m3x6_16pt.h>
#include <pwm.h>
#include <sonata_system.h>
#include <spi.h>
#include <timer.h>

#include "../common.h"
#include "heartbleed.h"
#include "lcd.h"

#define DEBUG_DEMO true
#define LENGTH_SCROLL_MILLIS 350u
#define BACKGROUND_COLOR BGRColorBlack
#define FOREGROUND_COLOR BGRColorWhite

// Define our own GPIO_IN_DBNC as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_IN_DBNC_AM GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_IN_DBNC_REG))

/**
 * @brief Busy waits for a given amount of time, constantly polling for any
 * joystick input and recording it to avoid inputs being eaten between
 * frames. This will only avoid inputs being eaten if the joystick did not
 * move on the previous invocation of this function. This produces behaviour
 * where holding the joystick ends when letting go of the joystick, but a
 * single small tap between frames will be recorded.
 *
 * @param milliseconds The time to wait for in milliseconds.
 * @param onlyFirst True if this should only record the first joystick input,
 * False if it should record all (OR them together).
 * @param consecutive True if this should avoid eating inputs on consecutive
 * inputs in the same direction, False if it should only poll for the first
 * movement in a direction.
 * @return The joystick state that was read.
 */
uint8_t wait_with_input(uint32_t milliseconds, bool onlyFirst, bool consecutive)
{
	static uint8_t prevDirection = 0x0;

	const uint32_t CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
	const uint32_t Cycles               = milliseconds * CyclesPerMillisecond;
	const uint64_t Start                = rdcycle64();
	uint64_t       end                  = Start + Cycles;
	uint64_t       current              = Start;
	uint8_t        joystickInput        = 0x0;
	while (end > current)
	{
		if (onlyFirst && !joystickInput)
		{
			joystickInput = read_joystick();
		}
		else if (!onlyFirst)
		{
			joystickInput |= read_joystick();
		}
		current = rdcycle64();
	}

	// If this is the first invocation in which the joystick was held in this
	// direction (i.e. a 'tap' or the start of a hold), or the `consecutive`
	// setting is set, use the polled input to avoid inputs being eaten.
	// If the `consecutive` setting is False and we previously moved in this
	// direction, use only the GPIO read at the end of waiting.
	uint8_t retDirection;
	if (!consecutive && (prevDirection & joystickInput))
	{
		retDirection = read_joystick();
	}
	else
	{
		retDirection = joystickInput;
	}
	prevDirection = joystickInput;
	return retDirection;
};

/**
 * @brief Waits for `LENGTH_SCROLL_MILLIS` while polling constantly for user
 * input on the joystick. Uses joystick up/down movements to control the
 * size of the request length, and detects pressed to submit the "heartbeat"
 * request.
 *
 * @param gpio The Sonata GPIO driver to use for I/O operations
 * @param current A param/outparam storing the current response length, which
 * will be updated to store the new response length if it changes.
 * @return Whether to submit the response or not (true = submit).
 */
bool length_joystick_control(size_t *current)
{
	const enum JoystickDir IncreaseDirection = JoystickUp | JoystickRight;
	const enum JoystickDir DecreaseDirection = JoystickDown | JoystickLeft;
	const size_t           SizeLimit         = 256;

	uint8_t joystickInput = wait_with_input(LENGTH_SCROLL_MILLIS, true, false);
	if (joystick_in_direction(joystickInput, IncreaseDirection))
	{
		if (!joystick_in_direction(joystickInput, DecreaseDirection))
		{
			*current = (*current >= SizeLimit) ? SizeLimit : (*current + 1);
		}
	}
	else if (joystick_in_direction(joystickInput, DecreaseDirection))
	{
		*current = (*current <= 0) ? 0 : (*current - 1);
	}
	else
	{
		return joystick_is_pressed(joystickInput);
	}
	return false;
}

/**
 * @brief Display the initial demo information to the LCD, including controls
 * and initial heartbeat request information.
 *
 * @param lcd A handle to Sonata's LCD.
 * @param bufferMessage The (not null-terminated) message that will be sent.
 * @param actual_req_len The genuine length of the `bufferMessage`. This MUST
 * match, or is undefined behaviour.
 */
void initial_lcd_write(St7735Context *lcd,
                       const char    *bufferMessage,
                       size_t         actual_req_len)
{
	lcd_draw_str(lcd,
	             5,
	             5,
	             LcdFontM3x6_16pt,
	             "Move Joystick to Change Length.",
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
	lcd_draw_str(lcd,
	             5,
	             15,
	             LcdFontM3x6_16pt,
	             "Press Joystick to Send.",
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
	lcd_draw_str(lcd,
	             5,
	             30,
	             LcdFontM3x6_16pt,
	             "Buffer Message: ",
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
	char displayMessage[actual_req_len + 1];
	memcpy(displayMessage, bufferMessage, actual_req_len);
	displayMessage[actual_req_len] = '\0';
	lcd_draw_str(lcd,
	             70,
	             30,
	             LcdFontM3x6_16pt,
	             displayMessage,
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
	lcd_draw_str(lcd,
	             5,
	             40,
	             LcdFontM3x6_16pt,
	             "Request Length: ",
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
}

/**
 * @brief Display the request length integer to the LCD.
 *
 * @param lcd A handle to Sonata's LCD
 * @param request_length The request length to write
 */
void draw_request_length(St7735Context *lcd, size_t request_length)
{
	const size_t ReqLenStrLen = 15u;
	char         req_len_s[ReqLenStrLen];
	size_t_to_str_base10(req_len_s, request_length);
	lcd_draw_str(lcd,
	             70,
	             40,
	             LcdFontM3x6_16pt,
	             req_len_s,
	             BACKGROUND_COLOR,
	             FOREGROUND_COLOR);
}

/**
 * @brief This function contains the control logic loop to fetch joystick
 * input related to the request length until the user presses the joystick
 * to submit the request.
 *
 * @param lcd A handle to Sonata's LCD
 * @param request_length An out parameter containing the initial request
 * length, and to which the final request length will be written.
 */
void get_request_length(St7735Context *lcd, size_t *request_length)
{
	// Initial draw
	draw_request_length(lcd, *request_length);

	// Get user input
	DEBUG_LOG("Waiting for user input on the joystick...");
	bool inputSubmitted = false;
	while (!inputSubmitted)
	{
		size_t prev_length = *request_length;
		inputSubmitted     = length_joystick_control(request_length);
		if (*request_length == prev_length)
		{
			continue; // Only re-draw when changed
		}
		lcd_draw_str(lcd,
		             70,
		             40,
		             LcdFontM3x6_16pt,
		             "       ",
		             BACKGROUND_COLOR,
		             FOREGROUND_COLOR);
		draw_request_length(lcd, *request_length);
	}

	DEBUG_LOG("Heartbeat submitted with length %d", (int)(*request_length));
}

/**
 * @brief This function contains the logic for writing the heartbeat/bleed
 * response message to the LCD, breaking the message across lines where it is
 * necessary.
 *
 * @param lcd A handle to Sonata's LCD
 * @param request_length The length of the heartbeat request. May not be the
 * actual message length.
 * @param result The heartbeat response to reply with. This should not be
 * null-terminated.
 */
void draw_heartbleed_response(St7735Context *lcd,
                              size_t         request_length,
                              const char    *result)
{
	const uint32_t CharsPerLine = 50u;

	// Format the result message for LCD display as a null-terminated string.
	const size_t PrefixLen = 10u;
	char         ResponsePrefix[PrefixLen + 1];
	memcpy(ResponsePrefix, "Response: ", PrefixLen + 1);
	char result_s[PrefixLen + request_length + 1];
	memcpy(result_s, ResponsePrefix, PrefixLen);
	memcpy(&result_s[PrefixLen], result, request_length);
	result_s[PrefixLen + request_length] = '\0';

	// Break the result message into several lines if it is too long to fit on
	// one line.
	char        result_line[CharsPerLine + 1];
	const char *result_char = result_s;
	size_t      line_length = 0u;
	size_t      line_num    = 0u;
	while (*result_char != '\0')
	{
		result_line[line_length++] = *result_char;
		result_char++;
		if (line_length == CharsPerLine)
		{
			result_line[line_length] = '\0';
			lcd_draw_str(lcd,
			             5,
			             55 + 10 * line_num,
			             LcdFontM3x6_16pt,
			             result_line,
			             BACKGROUND_COLOR,
			             FOREGROUND_COLOR);
			line_length = 0;
			line_num++;
		}
	}
	// Write the final line containing the remainder of the message.
	if (line_length)
	{
		result_line[line_length] = '\0';
		lcd_draw_str(lcd,
		             5,
		             55 + 10 * line_num,
		             LcdFontM3x6_16pt,
		             result_line,
		             BACKGROUND_COLOR,
		             FOREGROUND_COLOR);
	}
}

int main()
{
	// Initialise UART drivers for debug logging
	uart0 = UART_FROM_BASE_ADDR(UART0_BASE);
	uart1 = UART_FROM_BASE_ADDR(UART1_BASE);
	uart_init(uart0);
	uart_init(uart1);

	write_to_uart("Initialized UART driver");

	// Initialise the timer
	timer_init();
	timer_enable(SYSCLK_FREQ / 1000);

	// Initialise LCD display driver
	LCD_Interface lcdInterface;
	spi_t         lcdSpi;
	St7735Context lcd;
	spi_init(&lcdSpi, LCD_SPI, LcdSpiSpeedHz);
	lcd_init(&lcdSpi, &lcd, &lcdInterface);
	lcd_clean(&lcd, BACKGROUND_COLOR);

	// Turn on LCD backlight via PWM
	pwm_t lcd_bl = PWM_FROM_ADDR_AND_INDEX(PWM_BASE, PWM_LCD);
	set_pwm(lcd_bl, 1, 255);

// We represent the heartbeat message as a small array of incoming bytes,
// and initialise with its actual size.
#define ACTUAL_REQ_LEN 4
	const char bufferMessage[ACTUAL_REQ_LEN] = {'B', 'i', 'r', 'd'};

	size_t req_len = ACTUAL_REQ_LEN;
	while (true)
	{
		initial_lcd_write(&lcd, bufferMessage, ACTUAL_REQ_LEN);
		get_request_length(&lcd, &req_len);
		const char *result = heartbleed(bufferMessage, req_len);
		lcd_fill_rect(
		  &lcd, 5, 55, lcd.parent.width, lcd.parent.height, BACKGROUND_COLOR);
		draw_heartbleed_response(&lcd, req_len, result);
		free((void *)result);
	}

	return 0;
}
