// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "gpio_access.hh"
#include <debug.hh>
#include <thread.h>
#include <vector>

using Debug = ConditionalDebug<true, "Blinky Dynamic">;

static constexpr uint32_t LedIdx = 7;

void __cheri_compartment("blinky_dynamic") start_blinking()
{
	auto ledOpt = aquire_led(LedIdx);
	Debug::Assert(ledOpt.has_value(), "LED {} couldn't be aquired", LedIdx);
	auto led = ledOpt.value();

	while (true)
	{
		bool success = toggle_led(led);
		Debug::Assert(success, "Failed to toggle an LED");
		thread_millisecond_wait(500);
	}
}
