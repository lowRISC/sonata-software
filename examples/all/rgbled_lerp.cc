// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <ctype.h>
#include <platform-rgbctrl.hh>
#include <thread.h>

// The max brightness of the RGB LEDs
static constexpr uint8_t MaxBrightness = 25;
// The number of milliseconds per RGB LED update
static constexpr uint32_t MsecPerUpdate = 150;

[[noreturn]] void __cheri_compartment("rgbled_lerp") lerp_rgbleds()
{
	// Initialise the LED RGB driver.
	auto rgbled = MMIO_CAPABILITY(SonataRgbLedController, rgbled);

	bool    increasing = true;
	uint8_t lerpT      = 0;
	while (true)
	{
		// Update the RGB values by linearly interpolating
		rgbled->rgb(SonataRgbLed::Led0, lerpT, MaxBrightness - lerpT, 0);
		rgbled->rgb(SonataRgbLed::Led1, 0, MaxBrightness - lerpT, lerpT);
		rgbled->update();

		// Progress one timestep, increasing or decreasing the lerp variable.
		thread_millisecond_wait(MsecPerUpdate);
		if (increasing && lerpT == MaxBrightness)
		{
			increasing = false;
			continue;
		}
		if (!increasing && lerpT == 0)
		{
			increasing = true;
			continue;
		}
		lerpT = increasing ? ++lerpT : --lerpT;
	}
}
