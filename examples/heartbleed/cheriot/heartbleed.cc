// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>

#include "../../../libraries/lcd.hh"
#include <platform-gpio.hh>

#include "../common.h"

using namespace CHERI;
using namespace sonata::lcd;

constexpr bool     DebugDemo          = true;
constexpr uint32_t LengthScrollMillis = 350u;
constexpr Color    BackgroundColor    = Color::Black;
constexpr Color    ForegroundColor    = Color::White;

using Debug             = ConditionalDebug<DebugDemo, "Heartbleed">;
using JoystickDirection = SonataGpioBoard::JoystickDirection;
using JoystickValue     = SonataGpioBoard::JoystickValue;

/**
 * @brief Busy waits for a given amount of time, constantly polling for any
 * joystick input and recording it to avoid inputs being eaten between
 * frames. This will only avoid inputs being eaten if the joystick did not
 * move on the previous invocation of this function. This produces behaviour
 * where holding the joystick ends when letting go of the joystick, but a
 * single small tap between frames will be recorded.
 *
 * @param milliseconds The time to wait for in milliseconds.
 * @param gpio The Sonata GPIO driver to use for I/O operations.
 * @param onlyFirst True if this should only record the first joystick input,
 * False if it should record all (OR them together).
 * @param consecutive True if this should avoid eating inputs on consecutive
 * inputs in the same direction, False if it should only poll for the first
 * movement in a direction.
 * @return The joystick state that was read.
 */
JoystickValue wait_with_input(uint32_t                  milliseconds,
                              volatile SonataGpioBoard *gpio,
                              bool                      onlyFirst,
                              bool                      consecutive)
{
	static JoystickDirection prevDirection =
	  static_cast<JoystickDirection>(0x0);

	const uint32_t    CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
	const uint32_t    Cycles        = milliseconds * CyclesPerMillisecond;
	const uint64_t    Start         = rdcycle64();
	uint64_t          end           = Start + Cycles;
	uint64_t          current       = Start;
	JoystickDirection joystickInput = static_cast<JoystickDirection>(0x0);
	while (end > current)
	{
		if (onlyFirst && !joystickInput)
		{
			joystickInput = static_cast<JoystickDirection>(
			  gpio->input & SonataGpioBoard::Inputs::Joystick);
		}
		else if (!onlyFirst)
		{
			joystickInput = static_cast<JoystickDirection>(
			  (gpio->input & SonataGpioBoard::Inputs::Joystick) |
			  static_cast<uint32_t>(joystickInput));
		}
		current = rdcycle64();
	}

	// If this is the first invocation in which the joystick was held in this
	// direction (i.e. a 'tap' or the start of a hold), or the `consecutive`
	// setting is set, use the polled input to avoid inputs being eaten.
	// If the `consecutive` setting is False and we previously moved in this
	// direction, use only the GPIO read at the end of waiting.
	JoystickDirection retDirection;
	if (!consecutive && (static_cast<uint32_t>(prevDirection) &
	                     static_cast<uint32_t>(joystickInput)))
	{
		retDirection = static_cast<JoystickDirection>(
		  gpio->input & SonataGpioBoard::Inputs::Joystick);
	}
	else
	{
		retDirection = joystickInput;
	}
	prevDirection = joystickInput;
	return {retDirection};
};

/**
 * @brief Waits for `LengthScrollMillis` while polling constantly for user
 * input on the joystick. Uses joystick up/down movements to control the
 * size of the request length, and detects pressed to submit the "heartbeat"
 * request.
 *
 * @param gpio The Sonata GPIO driver to use for I/O operations
 * @param current A param/outparam storing the current response length, which
 * will be updated to store the new response length if it changes.
 * @return Whether to submit the response or not (true = submit).
 */
bool length_joystick_control(volatile SonataGpioBoard *gpio, size_t *current)
{
	constexpr JoystickDirection IncreaseDirection =
	  static_cast<JoystickDirection>(JoystickDirection::Up |
	                                 JoystickDirection::Right);
	constexpr JoystickDirection DecreaseDirection =
	  static_cast<JoystickDirection>(JoystickDirection::Down |
	                                 JoystickDirection::Left);
	constexpr size_t SizeLimit = 256;

	auto joystickInput = wait_with_input(LengthScrollMillis, gpio, true, false);
	if (joystickInput.is_direction_pressed(IncreaseDirection))
	{
		if (!joystickInput.is_direction_pressed(DecreaseDirection))
		{
			*current = (*current >= SizeLimit) ? SizeLimit : (*current + 1);
		}
	}
	else if (joystickInput.is_direction_pressed(DecreaseDirection))
	{
		*current = (*current <= 0) ? 0 : (*current - 1);
	}
	else
	{
		return joystickInput.is_pressed();
	}
	return false;
}

/**
 * @brief Display the initial demo information to the LCD, including controls
 * and initial heartbeat request information.
 *
 * @param lcd The Sonata LCD driver to use.
 * @param bufferMessage The (not null-terminated) message that will be sent.
 * @param actual_req_len The genuine length of the `bufferMessage`. This MUST
 * match, or is undefined behaviour.
 */
void initial_lcd_write(SonataLcd  *lcd,
                       const char *bufferMessage,
                       size_t      actual_req_len)
{
	lcd->draw_str({5, 5},
	              "Move Joystick to Change Length.",
	              BackgroundColor,
	              ForegroundColor);
	lcd->draw_str(
	  {5, 15}, "Press Joystick to Send.", BackgroundColor, ForegroundColor);
	lcd->draw_str(
	  {5, 30}, "Buffer Message: ", BackgroundColor, ForegroundColor);
	char displayMessage[actual_req_len + 1];
	memcpy(displayMessage, bufferMessage, actual_req_len);
	displayMessage[actual_req_len] = '\0';
	lcd->draw_str({70, 30}, displayMessage, BackgroundColor, ForegroundColor);
	lcd->draw_str(
	  {5, 40}, "Request Length: ", BackgroundColor, ForegroundColor);
}

/**
 * @brief Display the request length integer to the LCD.
 *
 * @param lcd The Sonata LCD Driver
 * @param gpio The Sonata GPIO driver to use for I/O operations
 * @param request_length The request length to display
 */
void draw_request_length(SonataLcd                *lcd,
                         volatile SonataGpioBoard *gpio,
                         size_t                    request_length)
{
	constexpr size_t ReqLenStrLen = 15u;
	char             req_len_s[ReqLenStrLen];
	size_t_to_str_base10(req_len_s, request_length);
	lcd->draw_str({70, 40}, req_len_s, BackgroundColor, ForegroundColor);
}

/**
 * @brief This function contains the control logic loop to fetch joystick
 * input related to the request length until the user presses the joystick
 * to submit the request.
 *
 * @param lcd The Sonata LCD Driver
 * @param gpio The Sonata GPIO driver to use for I/O operations
 * @param request_length An out parameter containing the initial request
 * length, and to which the final request length will be written.
 */
void get_request_length(SonataLcd                *lcd,
                        volatile SonataGpioBoard *gpio,
                        size_t                   *request_length)
{
	// Initial draw
	draw_request_length(lcd, gpio, *request_length);

	// Get user input
	Debug::log("Waiting for user input on the joystick...");
	bool inputSubmitted = false;
	while (!inputSubmitted)
	{
		size_t prev_length = *request_length;
		inputSubmitted     = length_joystick_control(gpio, request_length);
		if (*request_length == prev_length)
		{
			continue; // Only re-draw when changed
		}
		lcd->draw_str({70, 40}, "       ", BackgroundColor, ForegroundColor);
		draw_request_length(lcd, gpio, *request_length);
	}

	Debug::log("Heartbeat submitted with length {}", (int)(*request_length));
}

/**
 * @brief This function contains the logic for writing the heartbeat/bleed
 * response message to the LCD, breaking the message across lines where it is
 * necessary.
 *
 * @param lcd The Sonata LCD driver to use.
 * @param request_length The length of the heartbeat request. May not be the
 * actual message length.
 * @param result The heartbeat response to reply with. This should not be
 * null-terminated.
 */
void draw_heartbleed_response(SonataLcd  *lcd,
                              size_t      request_length,
                              const char *result)
{
	constexpr uint32_t CharsPerLine = 50u;

	// Format the result message for LCD display as a null-terminated string.
	constexpr size_t PrefixLen                     = 10u;
	constexpr char   ResponsePrefix[PrefixLen + 1] = "Response: ";
	char             result_s[PrefixLen + request_length + 1];
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
			lcd->draw_str({5, 55 + 10 * line_num},
			              result_line,
			              BackgroundColor,
			              ForegroundColor);
			line_length = 0;
			line_num++;
		}
	}
	// Write the final line containing the remainder of the message.
	if (line_length)
	{
		result_line[line_length] = '\0';
		lcd->draw_str({5, 55 + 10 * line_num},
		              result_line,
		              BackgroundColor,
		              ForegroundColor);
	}
}

[[noreturn]] void __cheri_compartment("heartbleed") entry()
{
	// Initialise the LCD driver and calculate display information
	SonataLcd *lcd         = new SonataLcd();
	Size       displaySize = lcd->resolution();
	Point      centre      = {displaySize.width / 2, displaySize.height / 2};
	lcd->clean(BackgroundColor);

	// Initialise GPIO driver to interact with the Joystick
	auto gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);

	// We represent the heartbeat message as a small array of incoming bytes,
	// and initialise with its actual size.
	constexpr size_t actual_req_len                = 4;
	const char       bufferMessage[actual_req_len] = {'B', 'i', 'r', 'd'};

	size_t req_len = actual_req_len;
	while (true)
	{
		initial_lcd_write(lcd, bufferMessage, actual_req_len);
		get_request_length(lcd, gpio, &req_len);
		const char *result = heartbleed(bufferMessage, req_len);
		lcd->fill_rect({5, 55, displaySize.width, displaySize.height},
		               BackgroundColor);
		draw_heartbleed_response(lcd, req_len, result);
		free((void *)result);
	}

	// Driver cleanup
	delete lcd;
}
