// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../../../third_party/display_drivers/core/lucida_console_10pt.h"
#include "../../../third_party/display_drivers/core/lucida_console_12pt.h"
#include "../../../third_party/display_drivers/core/m3x6_16pt.h"
#include "../../../third_party/display_drivers/st7735/lcd_st7735.h"
#include "../../../third_party/sonata-system/sw/legacy/common/gpio.h"
#include "../../../third_party/sonata-system/sw/legacy/common/rv_plic.h"
#include "../../../third_party/sonata-system/sw/legacy/common/sonata_system.h"
#include "../../../third_party/sonata-system/sw/legacy/common/spi.h"
#include "../../../third_party/sonata-system/sw/legacy/common/timer.h"
#include "../lib/analogue_pedal.h"
#include "../lib/automotive_common.h"
#include "../lib/automotive_menu.h"
#include "../lib/digital_pedal.h"
#include "../lib/joystick_pedal.h"
#include "../lib/no_pedal.h"
#include "adc.h"
#include "ksz8851.h"
#include "lcd.h"

// When using our model pedal, these are the maximum and minimum values that
// are measured through the ADC from the pedal's full range of motion. We
// hard-code these so that we can linearly transform from this range of ADC
// measurements to acceleration values.
#define PEDAL_MIN_ANALOGUE 310
#define PEDAL_MAX_ANALOGUE 1700

#define BACKGROUND_COLOUR ColorBlack
#define TEXT_COLOUR ColorWhite

#define ADC_BASE 0x8000B000

// Define our own GPIO_IN_DBnC as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_IN_DBNC_AM GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_IN_DBNC_REG))

// Global driver structs for use in callback functionality.
static uart_t uart0, uart1;
St7735Context lcd;
Adc           adc;
struct Netif  ethernetInterface;

/**
 * A function that writes a string to the UART console, wrapping a call to
 * `vsnprintf` such that (at least) unsigned integer formatting specifier
 * arguments can be provided alongside a format string.
 */
void write_to_uart(const char *format, ...)
{
	// Disable interrupts whilst outputting on UART to prevent the output for
	// RX IRQ from happening simultaneously
	arch_local_irq_disable();
	char    buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);
	putstr(buffer);
	arch_local_irq_enable();
}

/**
 * Busy waits until a certain time, doing nothing.
 *
 * `EndTime` is the cycle count to wait until.
 *
 * Returns the time (cycle) that `wait` ended on.
 */
uint64_t wait(const uint64_t EndTime)
{
	uint64_t currentTime = get_elapsed_time();
	while (currentTime < EndTime)
	{
		currentTime = get_elapsed_time();
	}
	return currentTime;
}

/**
 * Formats and draws a string to the LCD display based upon the provided
 * formatting and display information. The string can use formatting
 * specifiers as defined by `vsnprintf(3)`.
 *
 * `x` is the X-coordinate on the LCD of the top left of the string.
 * `y` is the Y-coordinate on the LCD of the top left of the string.
 * `font` is the font to render the string with.
 * `format` is the formatting string to write.
 * `backgroundColour` is the colour to use for the string's background.
 * `textColour` is the colour to render the text in.
 * The remaining variable arguments are unsigned integers used to fill in the
 * formatiting specifiers in the format string.
 */
void lcd_draw_str(uint32_t    x,
                  uint32_t    y,
                  LcdFont     font,
                  const char *format,
                  uint32_t    backgroundColour,
                  uint32_t    textColour,
                  ...)
{
	// Format the provided string
	char    buffer[1024];
	va_list args;
	va_start(args, textColour);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);

	Font stringFont;
	switch (font)
	{
		case LucidaConsole_10pt:
			stringFont = lucidaConsole_10ptFont;
			break;
		case LucidaConsole_12pt:
			stringFont = lucidaConsole_12ptFont;
			break;
		default:
			stringFont = m3x6_16ptFont;
	}
	lcd_st7735_set_font(&lcd, &stringFont);
	lcd_st7735_set_font_colors(&lcd, backgroundColour, textColour);
	lcd_st7735_puts(&lcd, (LCD_Point){x, y}, buffer);
}

/**
 * A callback function used to clean the LCD display with a given colour.
 *
 * `color` is the 32-bit colour to display.
 */
void lcd_clean(uint32_t color)
{
	size_t w, h;
	lcd_st7735_get_resolution(&lcd, &h, &w);
	LCD_rectangle rect = {(LCD_Point){0, 0}, w, h};
	lcd_st7735_fill_rectangle(&lcd, rect, color);
}

/**
 * A callback function used to draw a rectangle on the LCD.
 *
 * `x` is the X-position of the top-left corner of the rectangle on the LCD.
 * `y` is the Y-position of the top-left corner of the rectangle on the LCD.
 * `w` is the width of the rectangle.
 * `h` is the height of the rectangle.
 * `color` is the 32-bit colour to fill the rectangle with.
 */
void lcd_fill_rect(uint32_t x,
                   uint32_t y,
                   uint32_t w,
                   uint32_t h,
                   uint32_t color)
{
	LCD_rectangle rect = {(LCD_Point){x, y}, w, h};
	lcd_st7735_fill_rectangle(&lcd, rect, color);
}

/**
 * A callback function used to draw an image to the LCD.
 *
 * `x` is the X-position of the top-left corner of the image on the LCD.
 * `y` is the Y-position of the top-left corner of the image on the LCD.
 * `w` is the width of the image.
 * `h` is the height of the image.
 * `data` is the byte array containing the image data, of size at least
 * of `w` * `h`, where each value is a RGB565 colour value.
 */
void lcd_draw_img(uint32_t       x,
                  uint32_t       y,
                  uint32_t       w,
                  uint32_t       h,
                  const uint8_t *data)
{
	LCD_rectangle rect = {(LCD_Point){x, y}, w, h};
	lcd_st7735_draw_rgb565(&lcd, rect, data);
}

/**
 * A callback function used to read the GPIO joystick state.
 *
 * Returns the current joysttick state as a byte, where each of the
 * 5 least significant bits corresponds to a given joystick input.
 */
uint8_t read_joystick()
{
	return ((uint8_t)read_gpio(GPIO_IN_DBNC_AM)) & 0x1f;
}

/**
 * A callback function used to read the pedal input as a digital value,
 * when the pedal is plugged into the mikroBUS INT pin under header P7.
 *
 * Returns true if the pedal is pressed down, and false if not.
 */
bool read_pedal_digital()
{
	return (read_gpio(GPIO_IN_DBNC_AM) & (1 << 13)) > 0;
}

/**
 * A callback function used to read the pedal input as an analogue value via the
 * ADC driver. Reads the highest analogue value from all analogue inputs to the
 * board, and then uses known measured analogue ranges to linearly transform the
 * measurement into the acceleration range defined for the demo.
 *
 * Returns a value between DEMO_ACCELERATION_PEDAL_MIN and
 * DEMO_ACCELERATION_PEDAL_MAX, corresponding to the analogue pedal input.
 */
uint32_t read_pedal_analogue()
{
	uint32_t maxPedalValue = 0;
	// To allow the user to put the pedal pin in any of the analogue Arduino
	// pins, read all measurements and take the max.
	const AdcSampleStatusRegister Pins[6] = {
	  AdcStatusArduinoA0,
	  AdcStatusArduinoA1,
	  AdcStatusArduinoA2,
	  AdcStatusArduinoA3,
	  AdcStatusArduinoA4,
	  AdcStatusArduinoA5,
	};
	for (uint8_t i = 0; i < 6; ++i) // NOLINT
	{
		maxPedalValue = MAX(maxPedalValue, read_adc(&adc, Pins[i]));
	}
	write_to_uart("Measured Analogue Value: {}", (unsigned int)maxPedalValue);

	// Clamp the measured analogue values between the known minimum and maximum,
	// and then linearly transform the analogue range for our pedal to the
	// range needed for the demo.
	// Linearly transform the analogue range for our pedal to the range needed
	// for the demo. Clamp values between min and max analogue.
	uint32_t pedal = 0;
	if (maxPedalValue > PEDAL_MAX_ANALOGUE)
	{
		pedal = PEDAL_MAX_ANALOGUE - PEDAL_MIN_ANALOGUE;
	}
	else if (maxPedalValue > PEDAL_MIN_ANALOGUE)
	{
		pedal = maxPedalValue - PEDAL_MIN_ANALOGUE;
	}
	pedal *= (DEMO_ACCELERATION_PEDAL_MAX - DEMO_ACCELERATION_PEDAL_MIN);
	pedal /= (PEDAL_MAX_ANALOGUE - PEDAL_MIN_ANALOGUE);
	pedal += DEMO_ACCELERATION_PEDAL_MIN;
	return pedal;
}

/**
 * An empty callback function that does nothing.
 */
void null_callback(){};

/**
 * A callback function to send an ethernet frame, which wraps the Ethernet
 * driver's corresponding functionality.
 *
 * `buffer` is a byte array containing the data to be sent via Ethernet.
 * `length` is the length of that array.
 */
void send_ethernet_frame(const uint8_t *buffer, uint16_t length)
{
	struct Fbuf buf = {
	  (void *)buffer,
	  (uint8_t)length,
	};
	if (!ksz8851_output(&ethernetInterface, &buf))
	{
		write_to_uart("Error sending frame...\n");
	}
}

// Initialise memory for the tasks used in the automotive demo library code.
// We use linker script sections to ensure that memory is contiguous in the
// worst conceivable way, such that an overwrite to the task two array by
// just one byte directly writes into the task one acceleration value.

__attribute__((section(".data.__contiguous.__task_two"))) // NO-FORMAT
static TaskTwo memTaskTwo = {
  .write = {0},
};

__attribute__((section(".data.__contiguous.__task_one"))) // NO-FORMAT
static TaskOne memTaskOne = {
  .acceleration = 12,
  .braking      = 2,
  .speed        = 0,
};

__attribute__((section(".data.__contiguous.__analogue_task_two"))) // NO-FORMAT
static AnalogueTaskTwo memAnalogueTaskTwo = {
  .framebuffer = {0},
};

__attribute__((section(".data.__contiguous.__analogue_task_one"))) // NO-FORMAT
static AnalogueTaskOne memAnalogueTaskOne = {
  .acceleration = 12,
  .braking      = 2,
};

/**
 * The main loop for the sending board in the automotive demo. This
 * simply infinitely displays the menu to the user so that they can
 * select a demo, and then loads the selected demo.
 */
void main_demo_loop()
{
	while (true)
	{
		switch (select_demo())
		{
			case DemoNoPedal:
				// Run simple timed demo with no pedal & using passthrough
				init_no_pedal_demo_mem(&memTaskOne, &memTaskTwo);
				run_no_pedal_demo(get_elapsed_time());
				break;
			case DemoJoystickPedal:
				// Run demo using a joystick as a pedal, with passthrough
				init_joystick_demo_mem(&memTaskOne, &memTaskTwo);
				run_joystick_demo(get_elapsed_time());
				break;
			case DemoDigitalPedal:
				// Run demo using an actual physical pedal but taking
				// a digital signal, using simulation instead of passthrough
				init_digital_pedal_demo_mem(&memTaskOne, &memTaskTwo);
				run_digital_pedal_demo(get_elapsed_time());
				break;
			case DemoAnaloguePedal:
				// Run demo using an actual physical pedal, taking an
				// analogue signal via an ADC, with passthrough.
				init_analogue_pedal_demo_mem(&memAnalogueTaskOne,
				                             &memAnalogueTaskTwo);
				run_analogue_pedal_demo(get_elapsed_time());
				break;
		}
	}
}

/**
 * The thread entry point for the sending (buggy) part of the automotive
 * demo. Initialises relevant Ethernet, LCD, UART, PLIC, SPI, ADC and
 * timer drivers, sets the callback functions to be used by the demo
 * library, and then starts the main infinite loop.
 */
int main()
{
	// Initialise UART drivers for debug logging
	uart0 = UART_FROM_BASE_ADDR(UART0_BASE);
	uart1 = UART_FROM_BASE_ADDR(UART1_BASE);
	uart_init(uart0);
	uart_init(uart1);

	// Initialise the timer
	timer_init();
	timer_enable(SYSCLK_FREQ / 1000);

	// Initialise LCD display driver
	LCD_Interface lcdInterface;
	spi_t         lcdSpi;
	spi_init(&lcdSpi, LCD_SPI, LcdSpiSpeedHz);
	lcd_init(&lcdSpi, &lcd, &lcdInterface);
	lcd_clean(BACKGROUND_COLOUR);
	const LCD_Point Centre = {lcd.parent.width / 2, lcd.parent.height / 2};

	// Initialise Ethernet support for use via callback
	rv_plic_init();
	spi_t ethernetSpi;
	spi_init(&ethernetSpi, ETH_SPI, /*speed=*/0);
	ethernetInterface.spi = &ethernetSpi;
	uint8_t macSource[6];
	for (uint8_t i = 0; i < 6; i++)
	{
		macSource[i] = FixedDemoHeader.macSource[i];
	}
	ksz8851_init(&ethernetInterface, macSource);

	// Wait until a good physical ethernet link to start the demo
	if (!ksz8851_get_phy_status(&ethernetInterface))
	{
		write_to_uart("Waiting for a good physical ethernet link...\n");
		lcd_draw_str(Centre.x - 55,
		             Centre.y - 5,
		             M3x6_16pt,
		             "Waiting for a good physical",
		             BACKGROUND_COLOUR,
		             TEXT_COLOUR);
		lcd_draw_str(Centre.x - 30,
		             Centre.y + 5,
		             M3x6_16pt,
		             "ethernet link...",
		             BACKGROUND_COLOUR,
		             TEXT_COLOUR);
	}
	while (!ksz8851_get_phy_status(&ethernetInterface))
	{
		wait(get_elapsed_time() + 50);
	}

	// Wait an additional 0.25s to give the receiving board time to setup.
	wait(get_elapsed_time() + 2500);

	// Initialise the ADC driver for use via callback
	AdcClockDivider divider = (SYSCLK_FREQ / ADC_MIN_CLCK_FREQ);
	adc_init(&adc, ADC_FROM_BASE_ADDR(ADC_BASE), divider);

	// Adapt the common automotive library for legacy drivers
	init_lcd(lcd.parent.width, lcd.parent.height);
	init_callbacks((AutomotiveCallbacks){
	  .uart_send           = write_to_uart,
	  .wait                = wait,
	  .waitTime            = 120,
	  .time                = get_elapsed_time,
	  .loop                = null_callback,
	  .start               = null_callback,
	  .joystick_read       = read_joystick,
	  .digital_pedal_read  = read_pedal_digital,
	  .analogue_pedal_read = read_pedal_analogue,
	  .ethernet_transmit   = send_ethernet_frame,
	  .lcd =
	    {
	      .draw_str        = lcd_draw_str,
	      .clean           = lcd_clean,
	      .fill_rect       = lcd_fill_rect,
	      .draw_img_rgb565 = lcd_draw_img,
	    },
	});

	// Verify that, for the purpose of a reproducible error in our demo, task
	// one and task two have been assigned contiguous memories as required.
	write_to_uart("taskTwoMem location: %u\n", (unsigned int)&memTaskTwo);
	write_to_uart("taskTwoMem size: %u\n", sizeof(memTaskTwo));
	write_to_uart("taskOneMem location: %u\n", (unsigned int)&memTaskOne);
	write_to_uart("taskOneMem size: %u\n", sizeof(memTaskOne));
	assert((uint32_t)&memTaskTwo + sizeof(memTaskTwo) == (uint32_t)&memTaskOne);
	assert((uint32_t)&memAnalogueTaskTwo + sizeof(memAnalogueTaskTwo) ==
	       (uint32_t)&memAnalogueTaskOne);

	// Begin the main demo loop
	main_demo_loop();

	// Driver cleanup
	timer_disable();

	asm volatile("wfi");
	return 0;
}
