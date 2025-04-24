// Copyright lowRISC Contributors.
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
constexpr uint32_t LengthScrollMillis = 150u;
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
void initial_lcd_write(SonataLcd *lcd)
{
	lcd->draw_str({5, 5},
	              "Move Joystick to Change Length.",
	              BackgroundColor,
	              ForegroundColor,
	              Font::M5x7_16pt);
	lcd->draw_str({5, 15},
	              "Press Joystick to Send.",
	              BackgroundColor,
	              ForegroundColor,
	              Font::M5x7_16pt);
	lcd->draw_str({5, 30},
	              "Request a larger buffer ",
	              BackgroundColor,
	              ForegroundColor,
	              Font::M5x7_16pt);
	lcd->draw_str({5, 40},
	              "Suggested Length: ",
	              BackgroundColor,
	              ForegroundColor,
	              Font::M5x7_16pt);
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
	lcd->draw_str(
	  {110, 40}, req_len_s, BackgroundColor, ForegroundColor, Font::M5x7_16pt);
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
		lcd->draw_str({110, 40},
		              "       ",
		              BackgroundColor,
		              ForegroundColor,
		              Font::M5x7_16pt);
		draw_request_length(lcd, gpio, *request_length);
	}

	Debug::log("Heartbeat submitted with length {}", (int)(*request_length));
}

/**
 * @brief This function mocks the network and show the package on the lcd
 * instead.
 *
 * @param lcd A handle to Sonata's LCD
 * @param package The package to be sent.
 * @param len The length of the package .
 */
void network_send(void *handle, const char *package, size_t len)
{
	constexpr uint32_t CharsPerLine = 29;
	SonataLcd         *lcd          = (SonataLcd *)handle;

	size_t w_border = 2;
	lcd->fill_rect({w_border, 50, 160 - w_border, 128}, Color::Grey);

	// Break the result message into several lines if it is too long to fit on
	// one line.
	char        line_content[CharsPerLine + 1];
	const char *cursor      = package;
	size_t      line_length = 0u;
	size_t      line_num    = 0u;
	while (len-- != 0)
	{
		if (*cursor == 0)
		{
			line_content[line_length++] = '`';
		}
		else if (isprint((int)(*cursor)) == 0)
		{
			line_content[line_length++] = '%';
		}
		else
		{
			line_content[line_length++] = *cursor;
		}
		cursor++;
		if (line_length == CharsPerLine)
		{
			line_content[line_length] = '\0';
			lcd->draw_str({5, 55 + 10 * line_num},
			              line_content,
			              Color::Grey,
			              Color::Black,
			              Font::M5x7_16pt);
			line_length = 0;
			line_num++;
		}
	}
	// Write the final line containing the remainder of the message.
	if (line_length)
	{
		line_content[line_length] = '\0';
		lcd->draw_str({5, 55 + 10 * line_num},
		              line_content,
		              Color::Grey,
		              Color::Black,
		              Font::M5x7_16pt);
	}
}

[[noreturn]] void __cheri_compartment("heartbleed") entry()
{
	// Initialise the LCD driver and calculate display information
	SonataLcd *lcd         = new SonataLcd();
	Size       displaySize = lcd->resolution();
	Point      centre      = {displaySize.width / 2, displaySize.height / 2};
	lcd->clean(BackgroundColor);
	size_t w_border = 2;
	lcd->fill_rect({w_border, 50, 160 - w_border, 128}, Color::Grey);

	// Initialise GPIO driver to interact with the Joystick
	auto gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);

	size_t req_len = 8;
	while (true)
	{
		// We allocate a big chunck of memory to temporary store a json file
		// with sensitive information. Then we free the buffer without cleaning
		// the content.
		const size_t DbSize = 128;
		char        *ptr    = (char *)malloc(DbSize);
		read_file("clients.db", ptr, DbSize);
		free(ptr);

		initial_lcd_write(lcd);

		// Wait for the request.
		get_request_length(lcd, gpio, &req_len);

		const char *result =
		  run_query("SELECT name FROM animal WHERE can_fly=yes LIMIT 1");

		// We send back the response to the request without checking that the
		// requested length exeeds the needed size, which can leek information.
		heartbleed(lcd, result, req_len);

		free((void *)result);
	}

	// Driver cleanup
	delete lcd;
}
