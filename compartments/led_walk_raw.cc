// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-gpio.hh>
#include <thread.h>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Led Walk Raw">;

static constexpr uint32_t NumLeds = 8;

/// Thread entry point.
void __cheri_compartment("led_walk_raw") start_walking()
{
	Debug::log("Look pretty LEDs!");

	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);

	int  count    = 0;
	bool switchOn = true;
	while (true)
	{
		if (switchOn)
		{
			gpio->led_on(count);
		}
		else
		{
			gpio->led_off(count);
		};
		thread_millisecond_wait(500);
		switchOn = (count == NumLeds - 1) ? !switchOn : switchOn;
		count    = (count < NumLeds - 1) ? count + 1 : 0;
	}
}
