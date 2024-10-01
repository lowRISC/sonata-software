// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <thread.h>

#include "../../libraries/lcd.hh"
#include "../snake/cherry_bitmap.h"
#include "lowrisc_logo.h"

using namespace sonata::lcd;

// Positions to draw messages on the screen
static constexpr Point TopMessagePos       = {12, 8};
static constexpr Point BottomMessagePos    = {38, 114};
static constexpr Size  BottomMessageOffset = {77, 0};

/// Thread entry point.
void __cheri_compartment("lcd_test") lcd_test()
{
	// Initialise the LCD
	auto lcd    = SonataLcd();
	auto screen = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());

	// Draw the lowRISC logo to the LCD
	auto logoRect = screen.centered_subrect({105, 80});
	lcd.draw_image_rgb565(logoRect, lowriscLogo105x80);

	// Make a version of the cherry bitmap with a white background.
	uint8_t cherryImage10x10WhiteBg[200];
	for (uint32_t i = 0; i < 200; i += 2)
	{
		if (cherryImage10x10[i] == 0x00 && cherryImage10x10[i + 1] == 0x00)
		{
			cherryImage10x10WhiteBg[i]     = 0xFF;
			cherryImage10x10WhiteBg[i + 1] = 0xFF;
		}
		else
		{
			cherryImage10x10WhiteBg[i]     = cherryImage10x10[i];
			cherryImage10x10WhiteBg[i + 1] = cherryImage10x10[i + 1];
		}
	}
	const uint8_t *img = static_cast<const uint8_t *>(cherryImage10x10WhiteBg);

	// Draw the messages & cherry image to the LCD
	lcd.draw_str(TopMessagePos,
	             "Running on Sonata!",
	             Color::White,
	             Color::Black,
	             Font::LucidaConsole_10pt);
	lcd.draw_str(BottomMessagePos,
	             "Protected by CHERI",
	             Color::White,
	             Color::Black,
	             Font::M3x6_16pt);
	Point imgPos = {BottomMessagePos.x + BottomMessageOffset.width,
	                BottomMessagePos.y + BottomMessageOffset.height};
	lcd.draw_image_rgb565(Rect::from_point_and_size(imgPos, {10, 10}), img);

	while (true)
	{
		thread_millisecond_wait(500);
	}
}
