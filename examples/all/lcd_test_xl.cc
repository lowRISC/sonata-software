// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <thread.h>

#include "../../libraries/lcd.hh"
#include "../snake/cherry_bitmap.h"
#include "lowrisc_logo_dark.h"

using namespace sonata::lcd;

// Positions to draw messages on the screen
static constexpr Point TopMessagePos       = {24, 6};
static constexpr Size  TopMessageOffset    = {2, 14};
static constexpr Point BottomMessagePos    = {24, 136};
static constexpr Size  BottomMessageOffset = {77, 0};

/// Thread entry point.
void __cheri_compartment("lcd_test_xl") lcd_test_xl()
{
	// Initialise the LCD
	auto lcd    = SonataLcd(sonata::lcd::internal::LCD_Rotate90);
	auto screen = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());

	// Draw black background
	lcd.clean(Color::Black);

	// Draw the lowRISC logo to the LCD
	auto logoRect = screen.centered_subrect({105, 80});
	lcd.draw_image_rgb565(logoRect, lowriscLogoDark105x80);

	const uint8_t *img = static_cast<const uint8_t *>(cherryImage10x10);

	// Draw the messages & cherry image to the LCD
	lcd.draw_str(TopMessagePos,
	             "Running on",
	             Color::Black,
	             Color::White,
	             Font::LucidaConsole_10pt);
	lcd.draw_str(Point::offset(TopMessagePos, TopMessageOffset),
	             "Sonata XL!",
	             Color::Black,
	             Color::White,
	             Font::LucidaConsole_10pt);
	lcd.draw_str(BottomMessagePos,
	             "Protected by CHERI",
	             Color::Black,
	             Color::White,
	             Font::M3x6_16pt);
	Point imgPos = Point::offset(BottomMessagePos, BottomMessageOffset);
	lcd.draw_image_rgb565(Rect::from_point_and_size(imgPos, {10, 10}), img);

	while (true)
	{
		thread_millisecond_wait(500);
	}
}
