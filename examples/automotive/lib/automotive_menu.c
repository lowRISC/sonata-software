// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <stdint.h>

#include "./cursor.h"
#include "automotive_common.h"
#include "automotive_menu.h"

/**
 * Perform differential drawing on the "cursor" / "option select" icon in order
 * to display the automotive demo menu.
 *
 * `prev` is the index that the previous cursor was out (now covered with a
 * black square).
 * `current` is the new index that the cursor is now at (now drawn at).
 * `cursorImg` is a bool flag. If true, a cursor bitmap is drawn. If false,
 * a white 5x5 square replaces it instead.
 */
static void
fill_option_select_rects(uint8_t prev, uint8_t current, bool cursorImg)
{
	callbacks.lcd.fill_rect(
	  lcdCentre.x - 64, lcdCentre.y - 22 + prev * 20, 5, 5, ColorBlack);
	if (cursorImg)
	{
		callbacks.lcd.draw_img_rgb565(lcdCentre.x - 64,
		                              lcdCentre.y - 22 + current * 20,
		                              5,
		                              5,
		                              cursorImg5x5);
		return;
	}
	callbacks.lcd.fill_rect(
	  lcdCentre.x - 64, lcdCentre.y - 22 + current * 20, 5, 5, ColorWhite);
}

/**
 * The main demo selection menu loop. Initialises display information
 * for drawing the menu to the LCD, and also contains the logic for
 * navigating the menu and selecting a demo.
 *
 * Returns a `DemoApplication` that has been selected by the user.
 * It is not possible to not select an application (i.e. "quit").
 */
DemoApplication select_demo()
{
	// Display static menu information to the LCD
	callbacks.lcd.clean(ColorBlack);
	callbacks.lcd.draw_str(lcdCentre.x - 60,
	                       lcdCentre.y - 50,
	                       LucidaConsole_12pt,
	                       "Select Demo",
	                       ColorBlack,
	                       ColorWhite);
	const char *demoOptions[] = {
	  "[1] Analogue",
	  "[2] Digital",
	  "[3] Joystick",
	  "[4] No pedal",
	};
	for (uint8_t i = 0; i < 4; i++)
	{
		callbacks.lcd.draw_str(lcdCentre.x - 55,
		                       lcdCentre.y - 25 + i * 20,
		                       LucidaConsole_10pt,
		                       demoOptions[i],
		                       ColorBlack,
		                       ColorDarkerGrey);
	}
	const uint8_t NumOptions = 4;

	// Demo menu input & selection logic
	uint8_t    prevOption = 0, currentOption = 0;
	const bool CursorImg      = true;
	bool       optionSelected = false;
	fill_option_select_rects(prevOption, currentOption, CursorImg);
	callbacks.uart_send("Waiting for user input in the main menu...\n");
	while (!optionSelected)
	{
		prevOption                  = currentOption;
		const uint8_t JoystickInput = callbacks.joystick_read();
		if (joystick_in_direction(JoystickInput, Right))
		{
			currentOption =
			  (currentOption == 0) ? (NumOptions - 1) : (currentOption - 1);
			fill_option_select_rects(prevOption, currentOption, CursorImg);
			callbacks.wait(callbacks.time() + callbacks.waitTime * 2);
		}
		else if (joystick_in_direction(JoystickInput, Left))
		{
			currentOption = (currentOption + 1) % NumOptions;
			fill_option_select_rects(prevOption, currentOption, CursorImg);
			callbacks.wait(callbacks.time() + callbacks.waitTime * 2);
		}
		else if (joystick_in_direction(JoystickInput, Pressed))
		{
			optionSelected = true;
		}
	}

	callbacks.lcd.clean(ColorBlack);
	return (DemoApplication)currentOption;
}
