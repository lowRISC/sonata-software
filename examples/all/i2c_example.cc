// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <ctype.h>
#include <debug.hh>
#include <platform-i2c.hh>
#include <thread.h>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "i2c example">;

template<class T>
using Mmio = volatile T *;

/// Read from the AS612 Temperature Sensor
static void read_temperature_sensor_value(Mmio<OpenTitanI2c> i2c,
                                          const char        *regName,
                                          const uint8_t      RegIdx)
{
	uint8_t buf[2] = {RegIdx, 0};
	i2c->blocking_write(0x48, buf, 1, false);
	if (i2c->blocking_read(0x48, buf, 2u))
	{
		int16_t regValue = static_cast<int16_t>((buf[0] << 8) | buf[1]);
		int64_t temp     = regValue * 1000000LL / 128;
		Debug::log("The {} readout is {} microdegrees Celcius", regName, temp);
	}
	else
	{
		Debug::log("Could not read the {}", regName);
	}
}

static void id_eeprom_report(Mmio<OpenTitanI2c> i2c, const uint8_t IdAddr)
{
	uint8_t addr[2] = {0};
	i2c->blocking_write(0x50u, addr, 2, true);

	static uint8_t data[0x80];
	// Initialize the buffer to known contents in case of read issues.
	memset(data, 0xddu, sizeof(data));

	if (!i2c->blocking_read(IdAddr, data, sizeof(data)))
	{
		Debug::log("Failed to read EEPROM ID of device at address {}", IdAddr);
	}

	Debug::log("EEPROM ID of device at address {}:", IdAddr);
	for (size_t idx = 0u; idx + 4 < sizeof(data); idx += 4)
	{
		auto asChar = [](uint8_t val) -> char {
			return static_cast<char>(isprint(val) ? val : '.');
		};
		Debug::log("\t{}{}{}{} | {} {} {} {}",
		           asChar(data[idx + 0]),
		           asChar(data[idx + 1]),
		           asChar(data[idx + 2]),
		           asChar(data[idx + 3]),
		           data[idx + 0],
		           data[idx + 1],
		           data[idx + 2],
		           data[idx + 3]);
	}
}

[[noreturn]] void __cheri_compartment("i2c_example") run()
{
	auto i2cSetup = [](Mmio<OpenTitanI2c> i2c) {
		i2c->reset_fifos();
		i2c->host_mode_set();
		i2c->speed_set(100);
	};
	auto i2c0 = MMIO_CAPABILITY(OpenTitanI2c, i2c0);
	auto i2c1 = MMIO_CAPABILITY(OpenTitanI2c, i2c1);
	i2cSetup(i2c0);
	i2cSetup(i2c1);

	id_eeprom_report(i2c0, 0x50);

	read_temperature_sensor_value(i2c1, "temporature sensor configuration", 1);
	while (true)
	{
		read_temperature_sensor_value(i2c1, "temporature", 0);
		thread_millisecond_wait(4000);
	}
}
