// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-gpio.hh>
#include <thread.h>

using Debug = ConditionalDebug<true, "Blinky Raw">;

static constexpr uint32_t LedIdx = 7;

void __cheri_compartment("blinky_raw") start_blinking()
{
	Debug::log("Look a blinking LED!");

	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);

	while (true)
	{
		gpio->led_toggle(LedIdx);
		thread_millisecond_wait(500);
	}
}
