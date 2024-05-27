// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-gpio.hh>
#include <thread.h>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "led walk compartment">;

static constexpr uint32_t num_leds = 8;

/// Thread entry point.
void __cheri_compartment("led_walk") start_walking()
{
	Debug::log("Look pretty LEDs!");

	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);

	int  count     = 0;
	bool switch_on = true;
	while (true)
	{
		if (switch_on)
		{
			gpio->led_on(count);
		}
		else
		{
			gpio->led_off(count);
		};
		thread_millisecond_wait(500);
		switch_on = (count == num_leds - 1) ? !switch_on : switch_on;
		count     = (count < num_leds - 1) ? count + 1 : 0;
	}
}
