// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <platform-gpio.hh>
#include <platform-spi.hh>
#include <utility>

namespace sonata::lcd
{

	extern "C"
	{
#include "../display_drivers/core/m3x6_16pt.h"
#include "../display_drivers/st7735/lcd_st7735.h"
	}

	struct Point
	{
		uint32_t x;
		uint32_t y;

		static const Point ORIGIN;
	};

	inline constexpr const Point Point::ORIGIN{0, 0};

	struct Size
	{
		uint32_t width;
		uint32_t height;
	};

	struct Rect
	{
		uint32_t left;
		uint32_t top;
		uint32_t right;
		uint32_t bottom;

		static Rect from_points(Point a, Point b)
		{
			return {std::min(a.x, b.x),
			        std::min(a.y, b.y),
			        std::max(a.x, b.x),
			        std::max(a.y, b.y)};
		}

		static Rect from_point_and_size(Point point, Size size)
		{
			return {
			  point.x, point.y, point.x + size.width, point.y + size.height};
		}

		Rect centered_subrect(Size size)
		{
			return {(right + left - size.width) / 2,
			        (bottom + top - size.height) / 2,
			        (right + left + size.width) / 2,
			        (bottom + top + size.height) / 2};
		}
	};

	namespace font
	{
		constexpr const Font &m3x6_16pt = m3x6_16ptFont;
	}

	enum class Color : uint32_t
	{
		Black = 0x000000,
		White = 0xFFFFFF,
	};

	class SonataLCD
	{
		private:
		static constexpr uint32_t LcdCsPin  = 0;
		static constexpr uint32_t LcdRstPin = 1;
		static constexpr uint32_t LcdDcPin  = 2;
		static constexpr uint32_t LcdBlPin  = 3;

		/**
		 * Import the Capability helper from the CHERI namespace.
		 */
		template<typename T>
		using Capability = CHERI::Capability<T>;

		/**
		 * Helper.  Returns a pointer to the SPI device.
		 */
		[[nodiscard, gnu::always_inline]] static Capability<volatile SonataSpi>
		spi()
		{
			return MMIO_CAPABILITY(SonataSpi, spi1);
		}

		/**
		 * Helper.  Returns a pointer to the GPIO device.
		 */
		[[nodiscard, gnu::always_inline]] static Capability<volatile SonataGPIO>
		gpio()
		{
			return MMIO_CAPABILITY(SonataGPIO, gpio);
		}

		static inline void set_gpio_output_bit(uint32_t bit, bool value)
		{
			uint32_t output = gpio()->output;
			output &= ~(1 << bit);
			output |= value << bit;
			gpio()->output = output;
		}

		LCD_Interface lcd_intf;
		St7735Context ctx;

		public:
		SonataLCD()
		{
			// Set the initial state of the LCD control pins.
			set_gpio_output_bit(LcdDcPin, false);
			set_gpio_output_bit(LcdBlPin, true);
			set_gpio_output_bit(LcdCsPin, false);

			// Initialise SPI driver.
			spi()->init(false, false, true, false);

			// Reset LCD.
			set_gpio_output_bit(LcdRstPin, false);
			thread_millisecond_wait(150);
			set_gpio_output_bit(LcdRstPin, true);

			// Initialise LCD driverr.
			lcd_intf.handle = nullptr;
			lcd_intf.spi_write =
			  [](void *handle, uint8_t *data, size_t len) -> uint32_t {
				spi()->tx(data, len);
				return len;
			};
			lcd_intf.gpio_write =
			  [](void *handle, bool cs_high, bool dc_high) -> uint32_t {
				set_gpio_output_bit(LcdCsPin, cs_high);
				set_gpio_output_bit(LcdDcPin, dc_high);
				return 0;
			};
			lcd_intf.timer_delay = [](uint32_t ms) {
				thread_millisecond_wait(ms);
			};
			lcd_st7735_init(&ctx, &lcd_intf);

			// Set the LCD orentiation.
			lcd_st7735_set_orientation(&ctx, LCD_Rotate180);

			clean();
		}

		~SonataLCD()
		{
			clean();

			// Hold LCD in reset.
			set_gpio_output_bit(LcdRstPin, false);
			// Turn off backlight.
			set_gpio_output_bit(LcdBlPin, false);
		}

		void clean()
		{
			// Clean the display with a white rectangle.
			lcd_st7735_clean(&ctx);
		}

		Size resolution()
		{
			return {ctx.parent.width, ctx.parent.height};
		}

		void draw_pixel(Point point, Color color)
		{
			lcd_st7735_draw_pixel(
			  &ctx, {point.x, point.y}, static_cast<uint32_t>(color));
		}

		void draw_line(Point a, Point b, Color color)
		{
			if (a.y == b.y)
			{
				uint32_t x1 = std::min(a.x, b.x);
				uint32_t x2 = std::max(a.x, b.x);
				lcd_st7735_draw_horizontal_line(
				  &ctx, {{x1, a.y}, x2 - x1}, static_cast<uint32_t>(color));
			}
			else if (a.x == b.x)
			{
				uint32_t y1 = std::min(a.y, b.y);
				uint32_t y2 = std::max(a.y, b.y);
				lcd_st7735_draw_vertical_line(
				  &ctx, {{a.x, y1}, y2 - y1}, static_cast<uint32_t>(color));
			}
			else
			{
				// We currently only support horizontal and vertical lines.
				panic();
			}
		}

		void draw_image_bgr(Rect rect, const uint8_t *data)
		{
			lcd_st7735_draw_bgr(&ctx,
			                    {{rect.left, rect.top},
			                     rect.right - rect.left,
			                     rect.bottom - rect.top},
			                    data);
		}

		void draw_image_rgb565(Rect rect, const uint8_t *data)
		{
			lcd_st7735_draw_rgb565(&ctx,
			                       {{rect.left, rect.top},
			                        rect.right - rect.left,
			                        rect.bottom - rect.top},
			                       data);
		}

		void fill_rect(Rect rect, Color color)
		{
			lcd_st7735_fill_rectangle(&ctx,
			                          {{rect.left, rect.top},
			                           rect.right - rect.left,
			                           rect.bottom - rect.top},
			                          static_cast<uint32_t>(color));
		}

		void draw_str(Point       point,
		              const char *str,
		              const Font &font,
		              Color       background,
		              Color       foreground)
		{
			lcd_st7735_set_font(&ctx, &font);
			lcd_st7735_set_font_colors(&ctx,
			                           static_cast<uint32_t>(background),
			                           static_cast<uint32_t>(foreground));
			lcd_st7735_puts(&ctx, {point.x, point.y}, str);
		}
	};

} // namespace sonata::lcd
