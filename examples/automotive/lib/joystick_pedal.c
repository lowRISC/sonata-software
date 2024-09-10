// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>

#include "automotive_common.h"
#include "joystick_pedal.h"

// Pointers to the memory that has been allocated for use by the system
static TaskOne *taskOneMem;
static TaskTwo *taskTwoMem;

// A global flag that indicates whether bugged behaviour is currently being
// triggered or not.
static bool isBugged = false;

// Stores the last time that a joystick input was received for toggling
// bugged behaviour, such that input reading can be adequately spaced.
static uint64_t lastInputTime = 0;

/**
 * Initialise the memory pointers for the "Joystick Pedal" demo.
 *
 * For demo purposes, `taskTwo` should be located directly
 * preceding `taskOne` in memory.
 */
void init_joystick_demo_mem(TaskOne *taskOne, TaskTwo *taskTwo)
{
	taskOneMem = taskOne;
	taskTwoMem = taskTwo;
}

/**
 * Runs "Task One", which is the task that sends the current acceleration
 * information via the Ethernet callback. This task also monitors the joystick
 * input to determine whether it should decrease or increase the speed
 * accordingly. This acceleration gets accidentally overwritten by task two when
 * the bug occurs, causing it to send a much larger acceleration than intended.
 */
static void joystick_task_one()
{
	// Display speed information to the LCD
	callbacks.lcd.draw_str(10,
	                       45,
	                       LucidaConsole_10pt,
	                       "Current speed: %u   ",
	                       ColorBlack,
	                       ColorWhite,
	                       (unsigned int)taskOneMem->acceleration);

	// Transmit the previously measured pedal information to the car.
	callbacks.uart_send("Sending pedal data: acceleration=%u, braking=%u.\n",
	                    (unsigned int)taskOneMem->acceleration,
	                    (unsigned int)taskOneMem->braking);
	const uint64_t FrameData[2] = {taskOneMem->acceleration,
	                               taskOneMem->braking};
	send_data_frame(FrameData, FixedDemoHeader, 2);

	// Read the next pedal information to send - in this case checking for
	// any joystick inputs and modifying the acceleration accordingly.
	uint8_t joystick = callbacks.joystick_read();
	if (joystick_in_direction(joystick, Right) && taskOneMem->acceleration < 99)
	{
		taskOneMem->acceleration += 1;
	}
	else if (joystick_in_direction(joystick, Left) &&
	         taskOneMem->acceleration > 0)
	{
		taskOneMem->acceleration -= 1;
	}
}

/**
 * Runs "Task Two", which is some other random task that is writing to an 100-
 * item array, but features a bug that, when triggered (manually, via the
 * joystick), will accidentally write a 101th value out of the array bounds,
 * overwriting task one's memory.
 *
 * Even though this function is not exported, we must not declare this function
 * as static, as otherwise the PCC modifying trick used to recover from CHERI
 * exceptions without compartmentalisation will not work correctly. Likewise,
 * we return a bool to ensure consistent PCC tricks.
 */
bool joystick_task_two()
{
	// Determine based on joystick input whether the user wishes to
	// manually trigger the bug in this demo or not.
	uint8_t    joystick = callbacks.joystick_read();
	const bool EnoughTimePassed =
	  callbacks.time() > (lastInputTime + 3 * callbacks.waitTime);
	const bool JoystickMoved = joystick_in_direction(joystick, Up) ||
	                           joystick_in_direction(joystick, Down);
	if (EnoughTimePassed && JoystickMoved)
	{
		isBugged = !isBugged;
		if (!isBugged)
		{ // When untriggering the bug, we reset speed to allow re-use
			taskOneMem->acceleration = 15;
		}
		lastInputTime = callbacks.time();
		callbacks.uart_send("Manually triggering/untriggering bug.");
	}

	// Display bug status information to the LCD
	const char *bugStr = isBugged ? "Bug triggered" : "Not triggered";
	callbacks.lcd.draw_str(
	  10, 10, LucidaConsole_10pt, bugStr, ColorBlack, ColorGrey);

	// If flagged to be bugged, use an out-of-bounds index
	uint32_t index = 99;
	if (isBugged)
	{
		index = 100;
	}

	// The buggy line of code implementing task two - this should be a "<" check
	// to be within the `write` array bounds, but the "<=" comparison check
	// means that a 101th value is accidentally written.
	if (index <= 100)
	{
		taskTwoMem->write[index] = 1000;
	}
	return true;
}

/**
 * The entry point for running the "Joystick Pedal" demo.
 * Initialises memory and display information as needed by the two tasks,
 * and then runs tasks one and two in a loop, calling relevant callbacks
 * as is necessary.
 *
 * `initTime` is the time that `run_joystick_demo` was called at, relative
 * to the times returned by `callbacks.wait`.
 */
void run_joystick_demo(uint64_t initTime)
{
	// Start the demo in passthrough mode
	callbacks.uart_send("Automotive demo started!\n");
	callbacks.start();
	send_mode_frame(FixedDemoHeader, DemoModePassthrough);

	// Initialise the car with 15 acceleration, to be changed.
	taskOneMem->acceleration = 15;
	taskOneMem->braking      = 0;
	taskOneMem->speed        = 0;

	// The bugged behaviour must be manually triggered, using the joystick
	isBugged = false;

	// Draw static demo operation/usage information to the LCD
	callbacks.lcd.draw_str(10,
	                       62,
	                       M3x6_16pt,
	                       "Joystick up/down to change speed",
	                       ColorBlack,
	                       ColorDarkGrey);
	callbacks.lcd.draw_str(10,
	                       27,
	                       M3x6_16pt,
	                       "Joystick left/right to trigger bug",
	                       ColorBlack,
	                       ColorDarkGrey);
	callbacks.lcd.draw_str(10,
	                       80,
	                       M3x6_16pt,
	                       "Press the joystick to end the demo.",
	                       ColorBlack,
	                       ColorDarkerGrey);

	// Call task one and task two sequentially in a loop until the
	// user selects to quit the demo by pressing the joystick.
	uint64_t prevTime     = initTime;
	bool     stillRunning = true;
	while (stillRunning)
	{
		joystick_task_one();
		joystick_task_two();

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
