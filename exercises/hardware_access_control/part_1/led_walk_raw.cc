// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-gpio.hh>
#include <thread.h>

using Debug = ConditionalDebug<true, "Led Walk Raw">;

static constexpr uint32_t NumLeds = 8;

void __cheri_compartment("led_walk_raw") start_walking()
{
	Debug::log("Look walking LEDs!");

	auto gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);

	size_t ledIdx = NumLeds - 1;
	while (true)
	{
		gpio->led_toggle(ledIdx);
		thread_millisecond_wait(500);
		ledIdx = (ledIdx == 0) ? NumLeds - 1 : ledIdx - 1;
	}
}
