// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <thread.h>

#include "../library/lcd.hh"
#include "lowrisc_logo.h"

/// Thread entry point.
void __cheri_compartment("lcd_test") lcd_test()
{
	using namespace sonata::lcd;

	auto lcd      = SonataLCD();
	auto screen   = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());
	auto logoRect = screen.centered_subrect({105, 80});
	lcd.draw_image_rgb565(logoRect, lowriscLogo105x80);
	lcd.draw_str({1, 1},
	             "Hello world!",
	             sonata::lcd::font::m3x6,
	             Color::White,
	             Color::Black);

	while (true)
	{
		thread_millisecond_wait(500);
	}
}
