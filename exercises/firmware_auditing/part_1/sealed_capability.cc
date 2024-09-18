// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <platform-gpio.hh>
#include <thread.h>

static constexpr uint32_t LedIdx  = 7;
static constexpr size_t   ArrSize = 32;

void __cheri_compartment("sealed_capability") do_things()
{
	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);

	uint32_t arr[ArrSize];
	// Comment the above line and uncomment the below line to allocate on the
	// heap!
	// uint32_t *arr = new uint32_t[ArrSize];

	for (size_t i = 0; i < ArrSize; i++)
	{
		arr[i] = i;
	}

	while (true)
	{
		gpio->led_toggle(LedIdx);
		thread_millisecond_wait(500);
	}
}
