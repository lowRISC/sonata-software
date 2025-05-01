// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

/*
 * Simple demo using the LED Matrix. Can switch between a small 8x8 Conway's
 * Game of Life example from an initial state, and displaying some text
 * sweeping across the LED matrix.
 *
 * Hold down the joystick to switch between the two demos.
 *
 * Refer to the comment on the `sense_hat.hh` library: be careful about using
 * this demo when switching software / bitstream / resetting the FPGA. If used
 * in this context, you might find that the I2C controller on the Sense HAT
 * gets stuck, and so you need to either unplug/replug the Sense HAT or power
 * cycle the FPGA.
 */

#include "../../libraries/sense_hat.hh"
#include "../../third_party/display_drivers/src/core/m3x6_16pt.h"
#include <compartment.h>
#include <debug.hh>
#include <platform-gpio.hh>
#include <thread.h>

/// Expose debugging features unconditionally for this compartment.
using Debug  = ConditionalDebug<true, "Sense HAT">;
using Colour = SenseHat::Colour;

const Colour OnColour  = {.red = Colour::MaxRedValue, .green = 0, .blue = 0};
const Colour OffColour = {.red = 25, .green = 25, .blue = 25};
constexpr uint64_t GolFrameWaitMsec  = 400;
constexpr char     DemoText[]        = "CHERIoT <3 Sonata! ";
constexpr uint64_t TextFrameWaitMsec = 150;

/// Use a custom bitmap for "g" to make presentation slightly cleaner
constexpr uint8_t GBitmap[8] = {
  0x00, //
  0x06, //  ##
  0x05, // # #
  0x07, // ###
  0x04, //   #
  0x03, // ##
  0x00, //
  0x00, //
};

/// Game of Life starting patterns
constexpr bool Mold[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0},
  {0, 0, 1, 0, 1, 0, 1, 0},
  {0, 0, 1, 0, 0, 1, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
};

constexpr bool Octagon2[8][8] = {
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 1, 0, 0, 1, 0, 0},
  {0, 1, 0, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {0, 1, 0, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
};

constexpr bool Mazing[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 1, 0, 1, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0, 1, 0},
  {0, 1, 0, 0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
};

enum class Demo : uint8_t
{
	GameOfLife    = 0,
	ScrollingText = 1,
};

void update_image(const bool state[8][8], Colour fb[64])
{
	// Update the pixel framebuffer based on the current GOL game state
	for (uint8_t i = 0; i < 64u; i++)
	{
		fb[i] = state[i / 8u][i % 8u] ? OnColour : OffColour;
	}
}

uint8_t get_neighbours(bool state[8][8], uint8_t y, uint8_t x)
{
	// Count neighbours
	uint8_t neighbours = 0u;
	for (int8_t ny = static_cast<int8_t>(y - 1); ny <= y + 1; ny++)
	{
		for (int8_t nx = static_cast<int8_t>(x - 1); nx <= x + 1; nx++)
		{
			if ((ny == y && nx == x) || ny < 0 || ny >= 8 || nx < 0 || nx >= 8)
				continue;
			neighbours += state[ny][nx];
		}
	}
	return neighbours;
}

void update_gol_state(bool state[8][8])
{
	bool result[8][8] = {0u};
	// Iterate through cells
	for (uint8_t y = 0u; y < 8u; y++)
	{
		for (uint8_t x = 0u; x < 8u; x++)
		{
			uint8_t neighbours = get_neighbours(
			  state, static_cast<int16_t>(y), static_cast<int16_t>(x));
			// Calculate the new state and store into a temporary result array
			if (state[y][x] && neighbours != 2u && neighbours != 3u)
			{
				result[y][x] = 0u;
			}
			else if (!state[y][x] && neighbours == 3u)
			{
				result[y][x] = 1u;
			}
			else
			{
				result[y][x] = state[y][x];
			}
		}
	}
	// Copy the newly calculated state back into the original 2D array
	memcpy(state, result, sizeof(bool[8][8]));
}

void update_text_state(bool state[8][8], uint32_t *index, uint32_t *column)
{
	uint8_t currentChar = static_cast<uint8_t>(DemoText[*index]);

	/// If at the end of the string, reset to the beginning (wrap around)
	if (currentChar == '\0')
	{
		*index      = 0;
		*column     = 0;
		currentChar = static_cast<uint8_t>(DemoText[0]);
	}

	/// Copy all columns left by 1.
	for (uint8_t x = 0u; x < 7u; x++)
	{
		for (uint8_t y = 0u; y < 8u; y++)
		{
			state[y][x] = state[y][x + 1];
		}
	}

	/// Render the bitmap on the last column
	bool noBitsInColumn = true;
	for (uint8_t y = 1u; y < 8u; y++)
	{
		const uint32_t BitmapAddr = (currentChar - 32u) * 8u + y - 1u;
		const uint8_t  Bit        = 1u << *column;

		/// Retrieve bitmap
		if (currentChar == 'g')
		{
			state[y][7] = (GBitmap[y - 1u] & Bit) >> *column;
		}
		else
		{
			state[y][7] = (m3x6_16ptBitmaps[BitmapAddr] & Bit) >> *column;
		}

		/* (Slightly hacky): detect gaps (columns with no bits) to determine
		when the character ends, so that we can render this monospaced font
		without monospacing. This won't work for every character, but is good
		enough for our purposes. */
		if (state[y][7])
		{
			noBitsInColumn = false;
		}
	}

	/* If a gap is found, or the entire character is rendered, progress to the
	next character to render. */
	if ((noBitsInColumn && (currentChar != ' ')) || (*column == 6u))
	{
		if (++*index > sizeof(DemoText))
		{
			*index = 0;
		}
		*column = 0;
	}
	else
	{
		*column += 1;
	}
}

void initialize_demo(Demo      currentDemo,
                     bool      ledState[8][8],
                     uint32_t *index,
                     uint32_t *column)
{
	switch (currentDemo)
	{
		case Demo::GameOfLife:
			memcpy(ledState, Octagon2, sizeof(bool[8][8]));
			break;
		case Demo::ScrollingText:
			memset(ledState, 0u, sizeof(bool[8][8]));
			*column = 0u;
			*index  = 0u;
			break;
		default:
			Debug::log("Unknown demo type to intialize: {}", currentDemo);
			break;
	}
}

/// Thread entry point.
void __cheri_compartment("sense_hat_demo") test()
{
	Debug::log("Starting Sense HAT test");
	Demo currentDemo = Demo::GameOfLife;

	// Initialise GPIO capability for joystick inputs
	auto gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);

	// Initialise the Sense HAT
	auto senseHat = SenseHat();

	// Initialise a blank LED Matrix.
	Colour fb[64] = {OffColour};
	senseHat.set_pixels(fb);

	// Initialise LED Matrix starting states
	bool     ledState[8][8];
	uint32_t index               = 0;
	uint32_t column              = 0;
	bool     joystickPrevPressed = false;
	initialize_demo(currentDemo, ledState, &index, &column);

	while (true)
	{
		/* If a joystick press is detected during an update, switch the demo
		type. This is not polled in a while waiting between updates, so a press
		can be missed between updates. */
		bool joystickIsPressed = gpio->read_joystick().is_pressed();
		if (!joystickPrevPressed && joystickIsPressed)
		{
			currentDemo =
			  static_cast<Demo>((static_cast<uint8_t>(currentDemo) + 1) % 2);
			thread_millisecond_wait(500);
			initialize_demo(currentDemo, ledState, &index, &column);
			joystickPrevPressed = true;
		}
		if (joystickPrevPressed && !joystickIsPressed)
		{
			joystickPrevPressed = false;
		}

		switch (currentDemo)
		{
			case Demo::GameOfLife:
				/// Every frame, update the game state and LED matrix.
				thread_millisecond_wait(GolFrameWaitMsec);
				update_image(ledState, fb);
				senseHat.set_pixels(fb);
				update_gol_state(ledState);
				break;
			case Demo::ScrollingText:
				/// Every frame, update the scrolling text and LED matrix
				thread_millisecond_wait(TextFrameWaitMsec);
				update_image(ledState, fb);
				senseHat.set_pixels(fb);
				update_text_state(ledState, &index, &column);
				break;
			default:
				thread_millisecond_wait(100UL);
				Debug::log("Unknown demo type: {}", currentDemo);
		}
	}
}
