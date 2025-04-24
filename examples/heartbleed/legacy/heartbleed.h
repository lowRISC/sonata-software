// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef _HEARTBLEED_H_
#define _HEARTBLEED_H_

#include <stdarg.h>
#include <stdint.h>

#include <gpio.h>
#include <lcd_st7735.h>
#include <m3x6_16pt.h>
#include <sonata_system.h>
#include <spi.h>
#include <timer.h>

#include "lcd.h"

// Define our own GPIO_IN_DBNC as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_IN_DBNC_AM GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_IN_DBNC_REG))

#define CPU_TIMER_HZ (40 * 1000 * 1000) // 40 MHz

static uart_t uart0;

/**
 * A function that writes a string to the UART console, wrapping a call to
 * `vsnprintf` such that (at least) unsigned integer formatting specifier
 * arguments can be provided alongside a format string.
 */
void write_to_uart(const char *format, ...)
{
	// Disable interrupts whilst outputting on UART to prevent the output for
	// RX IRQ from happening simultaneously
	arch_local_irq_disable();
	char    buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);
	putstr(buffer);
	arch_local_irq_enable();
}

#define LOG_PREFIX "Heartbleed"
#define DEBUG_LOG(...)                                                         \
	if (DEBUG_DEMO)                                                            \
	{                                                                          \
		write_to_uart((LOG_PREFIX));                                           \
		write_to_uart(__VA_ARGS__);                                            \
	}

#define rdcycle64 get_mcycle

/**
 * @brief Read the current GPIO joystick state.
 *
 * @returns  The current joystick state as a byte, where each of the 5 least
 * significant bits corresponds to a given joystick input.
 */
uint8_t read_joystick()
{
	return ((uint8_t)(read_gpio(GPIO_IN_DBNC_AM) >> 8u) & 0x1f);
}

// Possible GPIO inputs for the joystick, and which GPIO bit they correspond to
enum JoystickDir
{
	JoystickLeft    = 1 << 0,
	JoystickUp      = 1 << 1,
	JoystickPressed = 1 << 2,
	JoystickDown    = 1 << 3,
	JoystickRight   = 1 << 4,
};

/**
 * Checks whether a given joystick input is in a certain direction or not
 * (where direction can also be "Pressed", i.e. the joystick is pressed down).
 *
 * `joystick` is the state of the joystick packed in the bits of a byte.
 * `direction` is the specific direction bit to test for.
 */
bool joystick_in_direction(uint8_t joystick, enum JoystickDir direction)
{
	return (joystick & ((uint8_t)direction)) > 0;
};

bool joystick_is_pressed(uint8_t joystick)
{
	return (joystick & JoystickPressed) > 0;
}

#endif //_HEARTBLEED_H_
