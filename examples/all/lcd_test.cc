// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <thread.h>

#include "../../libraries/lcd.hh"
#include "lowrisc_logo.h"

/// Thread entry point.
void __cheri_compartment("lcd_test") lcd_test()
{
	using namespace sonata::lcd;

	auto lcd      = SonataLcd();
	auto screen   = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());
	auto logoRect = screen.centered_subrect({105, 80});
	lcd.draw_image_rgb565(logoRect, lowriscLogo105x80);
	lcd.draw_str({1, 1}, "Hello world!", Color::White, Color::Black);

	while (true)
	{
		thread_millisecond_wait(500);
	}
}
