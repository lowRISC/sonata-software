// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>

#include "analogue_pedal.h"
#include "automotive_common.h"
#include "sound_icon.h"

// Pointers to memory that has been allocated for use by the system.
static AnalogueTaskOne *taskOneMem;
static AnalogueTaskTwo *taskTwoMem;

/**
 * Initialise the memory pointers for the "Analogue Pedal" demo.
 *
 * For demo purposes, `taskTwo` should be located directly
 * preceding `taskOne` in memory.
 */
void init_analogue_pedal_demo_mem(AnalogueTaskOne *taskOne,
                                  AnalogueTaskTwo *taskTwo)
{
	taskOneMem = taskOne;
	taskTwoMem = taskTwo;
}

/**
 * A function that linearly interpolates between red (0x0000FF) and
 * green (0x00FF00).
 *
 * `portion` defines the number of segments between red and green.
 * `segments` defines the total number of segments dividing the colour range
 */
static uint64_t lerp_green_to_red(uint32_t portion, uint32_t segments)
{
	uint64_t red   = ColorRed;
	uint64_t green = ColorGreen;
	red *= portion;
	green *= (segments - portion);
	red /= segments;
	green /= segments;
	return (red & 0xFF) | (green & 0xFF00);
}

/**
 * Draws the outline of the volume bar to the LCD.
 *
 * `x` and `y` specify the position of the top-left corner of the volume bar.
 * `maxVolume` describes how large the bar should be, where each one square/
 * segment of the bar corresponds to 1 volume.
 */
static void outline_volume_bar(uint32_t x, uint32_t y, uint32_t maxVolume)
{
	callbacks.lcd.fill_rect(x, y, 7 + maxVolume * 6, 13, ColorWhite);
	callbacks.lcd.fill_rect(x + 2, y + 2, 3 + maxVolume * 6, 9, ColorBlack);
}

/**
 * Draws the volume bar (minus its outline) to the LCD. The volume bar
 * consists of a square being drawn for each volume value, where the
 * square is coloured according to the current value in
 * `taskTwoMem->framebuffer` at that position/index.
 *
 * `x` and `y` specify the position of the top-left corner of the volume bar.
 * `maxVolume` describes how large the bar should be, where each one square/
 * segment of the bar corresponds to 1 volume.
 */
static void draw_volume_bar(uint32_t x, uint32_t y, uint32_t maxVolume)
{
	for (uint32_t i = 0; i < maxVolume; ++i)
	{
		callbacks.lcd.fill_rect(
		  x + 4 + (i * 6), y + 4, 5, 5, taskTwoMem->framebuffer[i]);
	}
}

/**
 * Runs "Task One", which is the task that reads the analogue accelerator pedal
 * information and transmits the previous pedal information via the Ethernet
 * callback. These saved acceleration values accidentally get overwritten by
 * task two when the bug occurs, causing it to send a much larger acceleration
 * than intended.
 */
static void analogue_task_one()
{
	// Transmit the previously measured pedal information to the car.
	callbacks.uart_send("Sending pedal data: acceleration=%u, braking=%u.\n",
	                    (unsigned int)taskOneMem->acceleration,
	                    (unsigned int)taskOneMem->braking);
	const uint64_t FrameData[2] = {taskOneMem->acceleration,
	                               taskOneMem->braking};
	send_data_frame(FrameData, FixedDemoHeader, 2);

	// Read the next pedal information to send via callback.
	taskOneMem->acceleration = callbacks.analogue_pedal_read();
}

/**
 * Runs "Task Two", which for the Analogue demo is a small "Volume Bar/Slider"
 * task. The user can use the joystick to increase or decrease the volume, and
 * the colour of the framebuffer used to render that volume bar is accordingly
 * calculated and written in this function. This function however has a bug
 * in its volume selection, allowing the user to exceed the max volume by 1,
 * and accidentally writing outside the framebuffer by 1 value.
 *
 * Even though this function is not exported, we must not declare this function
 * as static, as otherwise the PCC modifying trick used to recover from CHERI
 * exceptions without compartmentalisation will not work correctly. Likewise,
 * we return a bool to ensure consistent PCC tricks.
 */
bool analogue_task_two()
{
	// Control the volume bar via joystick input
	uint8_t joystick = callbacks.joystick_read();
	if (joystick_in_direction(joystick, Up) && taskTwoMem->volume > 0)
	{
		taskTwoMem->volume -= 1;
		taskTwoMem->framebuffer[taskTwoMem->volume] = 0;
	}
	else if (joystick_in_direction(joystick, Down) && taskTwoMem->volume <= 20)
	{
		// The above is the buggy line of code - this should be a "<" check
		// to be within the `framebuffer` array bounds, but the "<=" comparison
		// means that a 21st colour will be calculated and written.
		taskTwoMem->volume += 1;
	}

	// Update the frame buffer value for the current volume index, so that
	// any changes to the volume are reflected in the frame buffer (and so LCD)
	if (taskTwoMem->volume == 0)
	{
		return true;
	}
	taskTwoMem->framebuffer[taskTwoMem->volume - 1] =
	  lerp_green_to_red(taskTwoMem->volume, 21);
	return true;
}

/**
 * The entry point for running the "Analogue Pedal" demo.
 * Initialises memory and display information as needed by the two tasks,
 * and then runs tasks one and two in a loop, calling relevant callbacks
 * as is necessary.
 *
 * `initTime` is the time that `run_analogue_pedal_demo` was called at,
 * relative to the times returned by `callbacks.wait`.
 */
void run_analogue_pedal_demo(uint64_t initTime)
{
	// Start the demo in passthrough mode.
	callbacks.uart_send("Automotive demo started!\n");
	callbacks.start();
	send_mode_frame(FixedDemoHeader, DemoModePassthrough);

	// Initialise values in memory
	taskOneMem->acceleration = 0;
	taskOneMem->braking      = 0;
	taskTwoMem->volume       = 15;

	// Draw static LCD graphics, and populate the frame buffer with colours for
	// the initial volume bar.
	callbacks.lcd.draw_img_rgb565(11, 30, 15, 11, soundIconImg15x11);
	outline_volume_bar(10, 45, 20);
	for (uint32_t i = 0; i < 20; ++i)
	{
		taskTwoMem->framebuffer[i] =
		  (i < taskTwoMem->volume) ? lerp_green_to_red(i, 21) : 0;
	}
	callbacks.lcd.draw_str(10,
	                       60,
	                       LucidaConsole_10pt,
	                       "Exceed max volume",
	                       ColorBlack,
	                       ColorDarkGrey);
	callbacks.lcd.draw_str(
	  10, 75, LucidaConsole_10pt, "for a bug!", ColorBlack, ColorDarkGrey);
	callbacks.lcd.draw_str(10,
	                       12,
	                       M3x6_16pt,
	                       "Press the joystick to end the demo.",
	                       ColorBlack,
	                       ColorDarkerGrey);

	// Call task one and task two sequentially in a loop until the user
	// selects to quit the demo by pressing the joystick.
	uint64_t prevTime     = initTime;
	bool     stillRunning = true;
	while (stillRunning)
	{
		analogue_task_one();
		analogue_task_two();

		// Draw the volume bar to the screen each frame using the framebuffer
		uint32_t labelColor = (taskTwoMem->volume > 20) ? ColorRed : ColorWhite;
		callbacks.lcd.draw_str(33,
		                       30,
		                       LucidaConsole_10pt,
		                       "Volume: %u/%u ",
		                       ColorBlack,
		                       labelColor,
		                       (unsigned int)taskTwoMem->volume,
		                       20u);
		draw_volume_bar(10, 45, 20);

		const bool EnoughTimePassed =
		  prevTime > (initTime + callbacks.waitTime * 5);
		const bool JoystickPressed =
		  joystick_in_direction(callbacks.joystick_read(), Pressed);
		if (EnoughTimePassed && JoystickPressed)
		{
			stillRunning = false;
			callbacks.uart_send("Manually ended demo by pressing joystick.");
		}

		prevTime = callbacks.wait(prevTime + callbacks.waitTime);
		callbacks.loop();
	}

	callbacks.uart_send("Automotive demo ended!\n");
}
