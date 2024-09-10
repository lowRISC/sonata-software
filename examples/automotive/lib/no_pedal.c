// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>

#include "no_pedal.h"

// Pointers to the memory that has been allocated for use by the system
// for tasks one and two.
static TaskOne *taskOneMem;
static TaskTwo *taskTwoMem;

// A global flag indicating whether to reset task two's counter variable.
// Used to avoid passing in arguments to `task_two`, as that can break
// the program counter hacks being used in the CHERIoT implementation
// of this demo.
static bool resetCounter = true;

/**
 * Initialise the memory pointers for the "No Pedal" demo.
 *
 * For demo purposes, `taskTwo` should be located directly
 * preceding `taskOne` in memory.
 */
void init_no_pedal_demo_mem(TaskOne *taskOne, TaskTwo *taskTwo)
{
	taskOneMem = taskOne;
	taskTwoMem = taskTwo;
}

/**
 * Runs "Task One", which is the task that sends static pedal acceleration
 * information via the Ethernet callback. This acceleration gets accidentally
 * overwritten by task two when the bug occurs, causing it to send a much
 * larger acceleration than intended.
 */
static void no_pedal_task_one()
{
	callbacks.uart_send("Sending pedal data: acceleration=%u, braking=%u.\n",
	                    (unsigned int)taskOneMem->acceleration,
	                    (unsigned int)taskOneMem->braking);
	const uint64_t FrameData[2] = {taskOneMem->acceleration,
	                               taskOneMem->braking};
	send_data_frame(FrameData, FixedDemoHeader, 2);
}

/**
 * Runs "Task Two", which is some other random task that is writing 1000
 * to a 100-item array, but features a bug that will accidentally write
 * a 101th value out of the array bounds, overwriting task one's memory.
 *
 * The counter static variable used by this function can be reset by
 * setting the `resetCounter` global flag.
 *
 * Even though this function is not exported, we must not declare this function
 * as static, as otherwise the PCC modifying trick used to recover from CHERI
 * exceptions without compartmentalisation will not work correctly. Likewise,
 * we return a bool to ensure consistent PCC tricks.
 */
bool no_pedal_task_two()
{
	static uint64_t counter;
	if (resetCounter)
	{
		counter      = 0;
		resetCounter = false;
	}
	counter += 1;
	callbacks.uart_send("task_two, count = %u\n", (unsigned int)counter);

	// Draw simple pseudocode explaining the bug to the LCD
	callbacks.lcd.draw_str(5,
	                       25,
	                       LucidaConsole_10pt,
	                       "int i = %u;",
	                       ColorBlack,
	                       ColorGrey,
	                       (unsigned int)counter);
	uint32_t textColor = (counter >= 100) ? ColorRed : ColorGrey;
	callbacks.lcd.draw_str(
	  5, 55, LucidaConsole_10pt, "  arr[i] = 1000;", ColorBlack, textColor);

	// The buggy line of code implementing task two - this should be a "<" check
	// to be within the `write` array bounds, but the "<=" comparison check
	// means that a 101th value is accidentally written
	if (counter <= 100)
	{
		taskTwoMem->write[counter] = 1000;
	}
	return true;
}

/**
 * The entry point for running the "No Pedal" demo.
 * Initialises memory and display information as needed by the two tasks,
 * and then runs tasks one and two in a loop, calling relevant callbacks
 * as is necessary.
 *
 * `initTime` is the time that `run_no_pedal_demo` was called at, relative
 * to the times returned by `callbacks.wait`.
 */
void run_no_pedal_demo(uint64_t initTime)
{
	// Start the demo in passthrough mode
	callbacks.uart_send("Automotive demo started!\n");
	callbacks.start();
	send_mode_frame(FixedDemoHeader, DemoModePassthrough);

	// Initialise the car with 15 acceleration, to be overwritten.
	taskOneMem->acceleration = 15;
	taskOneMem->braking      = 0;
	taskOneMem->speed        = 0;
	resetCounter             = true;

	// Draw pseudocode display information to the LCD.
	callbacks.lcd.draw_str(
	  5, 10, LucidaConsole_10pt, "int arr[100];", ColorBlack, ColorGrey);
	callbacks.lcd.draw_str(
	  5, 40, LucidaConsole_10pt, "if (i <= 100) {", ColorBlack, ColorGrey);
	callbacks.lcd.draw_str(
	  5, 70, LucidaConsole_10pt, "}", ColorBlack, ColorGrey);

	// Call task one and task two sequentially in a loop for 175 iterations.
	uint64_t prevTime = initTime;
	for (uint32_t i = 0; i < 175; i++)
	{
		no_pedal_task_one();
		no_pedal_task_two();
		prevTime = callbacks.wait(prevTime + callbacks.waitTime);
		callbacks.loop();
	}

	callbacks.uart_send("Automotive demo ended!\n");
}
