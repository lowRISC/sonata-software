// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>

#include "automotive_common.h"
#include "digital_pedal.h"

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
 * Initialise the memory pointers for the "Digital Pedal" demo.
 *
 * For demo purposes, `taskTwo` should be located directly
 * preceding `taskOne` in memory.
 */
void init_digital_pedal_demo_mem(TaskOne *taskOne, TaskTwo *taskTwo)
{
	taskOneMem = taskOne;
	taskTwoMem = taskTwo;
}

/**
 * Runs "Task One", which is the task that sends the current accelerator
 * pedal information via the Ethernet callback. For the digital pedal,
 * this sends 100 if the pedal is pressed, or 0 if not. These saved
 * acceleration values accidentally get overwritten by task two when the
 * bug occurs, causing it to send a much larger acceleration than intended.
 */
static void digital_task_one()
{
	// Transmit the previously measured pedal information to the car.
	callbacks.uart_send("Sending pedal data: acceleration=%u, braking=%u.\n",
	                    (unsigned int)taskOneMem->acceleration,
	                    (unsigned int)taskOneMem->braking);
	const uint64_t FrameData[2] = {taskOneMem->acceleration,
	                               taskOneMem->braking};
	send_data_frame(FrameData, FixedDemoHeader, 2);

	// Read the next pedal information to send - this is a digital input, so
	// we just use 100 if the pedal is pressed, or 0 if it is not.
	taskOneMem->acceleration = callbacks.digital_pedal_read() ? 100 : 0;
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
bool digital_task_two()
{
	// Determine based on joystick input whether the user wishes to
	// manually trigger the bug in the demo or not.
	uint8_t    joystick = callbacks.joystick_read();
	const bool EnoughTimePassed =
	  callbacks.time() > (lastInputTime + 3 * callbacks.waitTime);
	const bool JoystickMoved = joystick_in_direction(joystick, Up) ||
	                           joystick_in_direction(joystick, Down);
	if (EnoughTimePassed && JoystickMoved)
	{
		isBugged      = !isBugged;
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
 * The entry point for running the "Digital Pedal" demo.
 * Initialises memory and display information as needed by the two tasks,
 * and then runs tasks one and two in a loop, calling relevant callbacks
 * as is necessary.
 *
 * `initTime` is the time that `run_digital_pedal_demo` was called at,
 * relative to the times returned by `callbacks.wait`.
 */
void run_digital_pedal_demo(uint64_t initTime)
{
	// Start the demo in simulation mode
	callbacks.uart_send("Automotive demo started!\n");
	callbacks.start();
	send_mode_frame(FixedDemoHeader, DemoModeSimulated);

	// Initialise the car with no acceleration.
	taskOneMem->acceleration = 0;
	taskOneMem->braking      = 0;
	taskOneMem->speed        = 0;
	isBugged                 = false;

	// Draw static demo operation/usage information to the LCD
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
	                       ColorGrey);

	// Call task one and task two sequentially in a loop until the user
	// selects to quit the demo by pressing the joystick
	uint64_t prevTime     = initTime;
	bool     stillRunning = true;
	while (stillRunning)
	{
		digital_task_one();
		digital_task_two();

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
