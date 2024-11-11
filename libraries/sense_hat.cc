// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "sense_hat.hh"
#include <cheri.hh>
#include <platform-i2c.hh>

using namespace CHERI;

/**
 * Helper. Returns a pointer to the I2C device.
 */
[[nodiscard, gnu::always_inline]] static Capability<volatile OpenTitanI2c> i2c()
{
	return MMIO_CAPABILITY(OpenTitanI2c, i2c1);
}

void __cheri_libcall Internal::init_i2c()
{
	/* Increase the reliability of the Sense HAT I2C against controller halts
	in case the I2C Controller gets into a bad state while loading demos. */
	i2c()->control = i2c()->control & ~(OpenTitanI2c::ControlEnableHost |
	                                    OpenTitanI2c::ControlEnableTarget);
	if (i2c()->interrupt_is_asserted(OpenTitanI2cInterrupt::ControllerHalt))
	{
		i2c()->reset_controller_events();
	}

	/* Initialise the I2C controller as normal. */
	i2c()->reset_fifos();
	i2c()->host_mode_set();
	i2c()->speed_set(100);
}

bool __cheri_libcall SenseHat::set_pixels(Colour pixels8x8[64])
{
	uint8_t  writeBuffer[1 + 64 * 3] = {0x0u};
	uint32_t i                       = 0u;
	writeBuffer[i++]                 = 0x00u; // Address
	for (uint8_t row = 0u; row < 8u; row++)
	{
		for (uint8_t column = 0u; column < 8u; column++)
		{
			uint32_t index = row * 8u + column;
			if (pixels8x8[index].red > Colour::MaxRedValue)
			{
				Debug::log("{}:{} exceeds maximum red.", row, column);
				return false;
			}
			writeBuffer[i++] = pixels8x8[index].red;
		}
		for (uint8_t column = 0u; column < 8u; column++)
		{
			uint32_t index = row * 8u + column;
			if (pixels8x8[index].green > Colour::MaxGreenValue)
			{
				Debug::log("{}:{} exceeds maximum green.", row, column);
				return false;
			}
			writeBuffer[i++] = pixels8x8[index].green;
		}
		for (uint8_t column = 0u; column < 8u; column++)
		{
			uint32_t index = row * 8u + column;
			if (pixels8x8[index].blue > Colour::MaxBlueValue)
			{
				Debug::log("{}:{} exceeds maximum blue.", row, column);
				return false;
			}
			writeBuffer[i++] = pixels8x8[index].blue;
		}
	}
	return i2c()->blocking_write(0x46u, writeBuffer, sizeof(writeBuffer), true);
}
