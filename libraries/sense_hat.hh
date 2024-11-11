// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

/*
 * A driver used for writing to the Raspberry Pi Sense HAT LED Matrix
 * via an I2C connection.
 *
 * Warning: Aborting the I2C transaction (e.g. resetting the FPGA, switching
 * software slot or bitstream) can cause the in-progress I2C transaction which
 * is writing the entire framebuffer to be interrupted. This can leave the I2C
 * controller on the Sense HAT in a bad state, with no easy mechanism for
 * resetting it via the OpenTitan I2C controller. You should be aware of this
 * if you plan to use and rely on this driver. If the I2C controller does get
 * into a bad state, you must unplug (for a few seconds) and replug the Sense
 * HAT, or power cycle the entire Sonata FPGA. Do not use this driver if you
 * plan to switch software slots / bitstreams regularly.
 */

#include <algorithm>
#include <debug.hh>
#include <utility>

namespace Internal
{
	void __cheri_libcall init_i2c();
} // namespace Internal

class SenseHat
{
	private:
	/// Flag set when we're debugging this driver.
	static constexpr bool DebugSenseHat = true;

	/// Helper for conditional debug logs and assertions.
	using Debug = ConditionalDebug<DebugSenseHat, "Sense HAT">;

	public:
	struct Colour
	{
		// Sense HAT LED Matrix uses RGB 565
		static constexpr uint8_t RedBits   = 5;
		static constexpr uint8_t GreenBits = 6;
		static constexpr uint8_t BlueBits  = 5;

		// Maximum values for each field, calculated from the number of bits
		static constexpr uint8_t MaxRedValue   = (1 << RedBits) - 1;
		static constexpr uint8_t MaxGreenValue = (1 << GreenBits) - 1;
		static constexpr uint8_t MaxBlueValue  = (1 << BlueBits) - 1;

		// Colour fields
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	} __attribute__((packed));

	/**
	 * A constructor for the Sense HAT driver. Initialises the I2C controller
	 * that will communicate with the Sense HAT's I2C Controller to write to
	 * its LED Matrix.
	 */
	SenseHat()
	{
		Internal::init_i2c();
	}

	/**
	 * Set the values of all pixels in the 8x8 LED Matrix on the Sense HAT
	 * via an I2C connection. Will block until the entire array has been
	 * written. Values are read from `pixels8x8` in row-major order (i.e.
	 * all of row 1, then all of row 2, etc.)
	 */
	bool __cheri_libcall set_pixels(Colour pixels8x8[64]);
};
