// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "automotive_common.h"

// Globals to store display size information for ease of use
LcdSize lcdSize, lcdCentre;

// Index of all callbacks available in the automotive demo.
AutomotiveCallbacks callbacks;

/**
 * The fixed Ethernet frame header that is used in the automotive demo. This
 * ensures that all frames sent are broadcast frames, from the MAC source
 * address of 3A:30:25:24:FE:7A, with a type of 0806.
 */
const EthernetHeader FixedDemoHeader = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0x3a, 0x30, 0x25, 0x24, 0xfe, 0x7a},
  {0x08, 0x06},
};

/**
 * Initialises the LCD size information globals with relevant information.
 *
 * `width` and `height` should correspond to the size of the LCD.
 */
void init_lcd(uint32_t width, uint32_t height)
{
	lcdSize.x   = width;
	lcdSize.y   = height;
	lcdCentre.x = width / 2;
	lcdCentre.y = height / 2;
}

/**
 * Initialises the automotive callbacks from the supplied `automotiveCallbacks`
 */
void init_callbacks(AutomotiveCallbacks automotiveCallbacks)
{
	callbacks = automotiveCallbacks;
}

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

/**
 * Sends a pedal data Ethernet frame to the receiving board. This involves
 * preprending the frame with the Ethernet header, casting the 64-bit data
 * into bytes, and computing the frame length.
 *
 * `data` is the array of 64-bit pedal values to transmit.
 * `header` is the Ethernet header to attach to the frame.
 * `length` is the length of the `data` array.
 */
void send_data_frame(const uint64_t *data,
                     EthernetHeader  header,
                     uint16_t        length)
{
	assert(length < (100 / 8));
	uint8_t frameBuf[128];
	uint8_t frameLen = 0;
	// Copy the Ethernet header
	for (uint8_t i = 0; i < 14; ++i)
	{
		frameBuf[frameLen++] = header.macDestination[i];
	}
	// Write the "Pedal Data" type in the frame.
	frameBuf[frameLen++] = FramePedalData;
	// Copy over the pedal data, converting from 64-bit to 8-bit.
	for (uint8_t i = 0; i < length; ++i)
	{
		for (int8_t j = 7; j >= 0; --j)
		{
			frameBuf[frameLen++] = (data[i] >> (8 * j)) & 0xFF;
		}
	}
	// Call the relevant callback to transmit the frame
	callbacks.ethernet_transmit(frameBuf, frameLen);
}

/**
 * Sends a demo mode Ethernet frame to the receiving board. This involves
 * prepending the frame with the Ethernet header, and constructing the frame.
 *
 * `header` is the Ethernet header to attach to the frame.
 * `mode` is the demo mode selection to transmit.
 */
void send_mode_frame(EthernetHeader header, DemoMode mode)
{
	uint8_t frameBuf[128];
	uint8_t frameLen = 0;
	// Copy the Ethernet header
	for (uint8_t i = 0; i < 14; ++i)
	{
		frameBuf[frameLen++] = header.macDestination[i];
	}
	// Write the "Demo Mode" type and selected mode into the frame, and send.
	frameBuf[frameLen++] = FrameDemoMode;
	frameBuf[frameLen++] = mode;
	callbacks.ethernet_transmit(frameBuf, frameLen);
}
