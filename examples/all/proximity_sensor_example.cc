// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

// This examples requires a APDS9960 sensor
// (https://www.adafruit.com/product/3595) connected to the qwiic0 connector.

#include "../../libraries/sense_hat.hh"
#include <compartment.h>
#include <ctype.h>
#include <debug.hh>
#include <platform-i2c.hh>
#include <platform-rgbctrl.hh>
#include <thread.h>

const uint8_t ApdS9960Enable = 0x80;
const uint8_t ApdS9960Id     = 0x92;
const uint8_t ApdS9960Ppc    = 0x8E;
const uint8_t ApdS9960CR1    = 0x8F;
const uint8_t ApdS9960Pdata  = 0x9C;

const uint8_t ApdS9960IdExp      = 0xAB;
const uint8_t ApdS9960I2cAddress = 0x39;

#define SENSE_HAT_AVAILABLE false

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "proximity sensor example">;

template<class T>
using Mmio = volatile T *;

static void setup_proximity_sensor(Mmio<OpenTitanI2c> i2c, const uint8_t Addr)
{
	uint8_t buf[2];

	buf[0] = ApdS9960Id;

	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 1, false))
	{
		Debug::log("Failed to write proximity sensor address");
		return;
	}
	bool success = i2c->blocking_read(ApdS9960I2cAddress, buf, 1);
	Debug::Assert(success, "Failed to read proximity sensor ID");

	Debug::log("Proximity sensor ID: {}", buf[0]);

	Debug::Assert(buf[0] == ApdS9960IdExp,
	              "Proximity sensor ID was not expected value of {}, saw {}",
	              ApdS9960IdExp,
	              buf[0]);

	// Disable everything
	buf[0] = ApdS9960Enable;
	buf[1] = 0x0;
	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 2, true))
	{
		Debug::log("Failed to write proximity sensor address");
		return;
	}
	// Wait for all engines to go idle
	thread_millisecond_wait(25);

	// Set PEN (proximity enable) and PON (power on)
	buf[0] = ApdS9960Enable;
	buf[1] = 0x5;
	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 2, true))
	{
		Debug::log("Failed to write proximity sensor address");
		return;
	}
	// Wait for power on
	thread_millisecond_wait(10);

	// Set proximity gain to 8x
	buf[0] = ApdS9960CR1;
	buf[1] = 0x0c;
	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 2, true))
	{
		Debug::log("Failed to write proximity sensor address");
		return;
	}

	// Set proximity pulse length to 4us and pulse count to 16us
	// (experimentially determined, other values may work better!)
	buf[0] = ApdS9960Ppc;
	buf[1] = 0x04;
	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 2, true))
	{
		Debug::log("Failed to write proximity sensor address");
		return;
	}
}

static uint8_t read_proximity_sensor(Mmio<OpenTitanI2c> i2c)
{
	uint8_t buf[1];

	buf[0] = ApdS9960Pdata;
	if (!i2c->blocking_write(ApdS9960I2cAddress, buf, 1, false))
	{
		Debug::log("Failed to write proximity sensor address");
		return 0;
	}
	if (!i2c->blocking_read(ApdS9960I2cAddress, buf, 1))
	{
		Debug::log("Failed to read proximity sensor value");
		return 0;
	}

	return buf[0];
}

void update_sense_hat(SenseHat *senseHat, uint8_t prox)
{
	constexpr uint8_t RedOffset   = 3u; // Minimum R on Sense HAT LEDs.
	constexpr uint8_t ColourRange = SenseHat::Colour::MaxRedValue - RedOffset;
	constexpr uint8_t ProxOffset  = 16u; // Minimum prox to show on LEDs.
	constexpr uint8_t ProxRange   = UINT8_MAX - ProxOffset;
	constexpr uint8_t NumLeds     = 8u * 8u;
	constexpr SenseHat::Colour OffColour = {.red = 0U, .green = 0U, .blue = 0U};

	SenseHat::Colour fb[NumLeds] = {OffColour};
	/* Scale the proximity to the number of LED Matrix pixels. Clamp and
	linearly scale the proximity / brightness scales for a better result. */
	prox = (prox < ProxOffset) ? 0u : prox - ProxOffset;
	uint8_t filled =
	  static_cast<uint8_t>(static_cast<uint64_t>(prox) * NumLeds / ProxRange);
	filled = (filled > NumLeds) ? NumLeds : filled;
	for (uint8_t i = 0; i < filled; i++)
	{
		uint8_t red = static_cast<uint8_t>(
		  static_cast<uint64_t>(i) * ColourRange / NumLeds + RedOffset);
		fb[i] = (SenseHat::Colour){.red = red, .green = 0u, .blue = 0u};
	}
	senseHat->set_pixels(fb);
}

[[noreturn]] void __cheri_compartment("proximity_sensor_example") run()
{
	// Initialise the Sense HAT if we use it in this demo
	SenseHat *senseHat = NULL;
	if (SENSE_HAT_AVAILABLE)
	{
		senseHat = new SenseHat();
	}

	auto i2cSetup = [](Mmio<OpenTitanI2c> i2c) {
		i2c->reset_fifos();
		i2c->host_mode_set();
		i2c->speed_set(1);
	};

	auto i2c0 = MMIO_CAPABILITY(OpenTitanI2c, i2c0);
	i2cSetup(i2c0);
	uint8_t addr;

	auto rgbled = MMIO_CAPABILITY(SonataRgbLedController, rgbled);

	setup_proximity_sensor(i2c0, ApdS9960I2cAddress);

	while (true)
	{
		uint8_t prox = read_proximity_sensor(i2c0);
		Debug::log("Proximity is {}\r", prox);
		rgbled->rgb(SonataRgbLed::Led0, ((prox) >> 3), 0, 0);
		rgbled->rgb(SonataRgbLed::Led1, 0, (255 - prox) >> 3, 0);
		rgbled->update();

		if (SENSE_HAT_AVAILABLE)
		{
			update_sense_hat(senseHat, prox);
		}

		thread_millisecond_wait(100);
	}

	if (SENSE_HAT_AVAILABLE)
	{
		delete senseHat;
	}
}
