// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

// This examples requires a APDS9960 sensor
// (https://www.adafruit.com/product/3595) connected to the qwiic0 connector.

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

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "proximity sensor example">;

template<class T>
using Mmio = volatile T *;

static void setup_proximity_sensor(Mmio<OpenTitanI2c> i2c, const uint8_t Addr)
{
	uint8_t buf[2];

	buf[0] = ApdS9960Id;

	i2c->write(ApdS9960I2cAddress, buf, 1, false);
	bool success = i2c->read(ApdS9960I2cAddress, buf, 1);
	Debug::Assert(success, "Failed to read proximity sensor ID");

	Debug::log("Proximity sensor ID: {}", buf[0]);

	Debug::Assert(buf[0] == ApdS9960IdExp,
	              "Proximity sensor ID was not expected value of {}, saw {}",
	              ApdS9960IdExp,
	              buf[0]);

	// Disable everything
	buf[0] = ApdS9960Enable;
	buf[1] = 0x0;
	i2c->write(ApdS9960I2cAddress, buf, 2, true);
	// Wait for all engines to go idle
	thread_millisecond_wait(25);

	// Set PEN (proximity enable) and PON (power on)
	buf[0] = ApdS9960Enable;
	buf[1] = 0x5;
	i2c->write(ApdS9960I2cAddress, buf, 2, true);
	// Wait for power on
	thread_millisecond_wait(10);

	// Set proximity gain to 8x
	buf[0] = ApdS9960CR1;
	buf[1] = 0x0c;
	i2c->write(ApdS9960I2cAddress, buf, 2, true);

	// Set proximity pulse length to 4us and pulse count to 16us
	// (experimentially determined, other values may work better!)
	buf[0] = ApdS9960Ppc;
	buf[1] = 0x04;
	i2c->write(ApdS9960I2cAddress, buf, 2, true);
}

static uint8_t read_proximity_sensor(Mmio<OpenTitanI2c> i2c)
{
	uint8_t buf[1];

	buf[0] = ApdS9960Pdata;
	i2c->write(ApdS9960I2cAddress, buf, 1, false);
	if (!i2c->read(ApdS9960I2cAddress, buf, 1))
	{
		Debug::log("Failed to read proximity sensor value");
		return 0;
	}

	return buf[0];
}

[[noreturn]] void __cheri_compartment("proximity_sensor_example") run()
{
	auto i2cSetup = [](Mmio<OpenTitanI2c> i2c) {
		i2c->reset_fifos();
		i2c->set_host_mode();
		i2c->set_speed(1);
	};

	auto i2c0 = MMIO_CAPABILITY(OpenTitanI2c, i2c0);
	i2cSetup(i2c0);
	uint8_t addr;

	auto rgbled = MMIO_CAPABILITY(SonataRGBLEDCtrl, rgbled);

	setup_proximity_sensor(i2c0, ApdS9960I2cAddress);

	while (true)
	{
		uint8_t prox = read_proximity_sensor(i2c0);
		Debug::log("Proximity is {}\r", prox);
		rgbled->set_rgb(((prox) >> 3), 0, 0, 0);
		rgbled->set_rgb(0, (255 - prox) >> 3, 0, 1);
		rgbled->update();

		thread_millisecond_wait(100);
	}
}
