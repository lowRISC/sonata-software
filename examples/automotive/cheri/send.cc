// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-adc.hh>
#include <platform-ethernet.hh>
#include <thread.h>

#include "../../../libraries/lcd.hh"
#include "../../snake/cherry_bitmap.h"

#include "../lib/analogue_pedal.h"
#include "../lib/automotive_common.h"
#include "../lib/automotive_menu.h"
#include "../lib/digital_pedal.h"
#include "../lib/joystick_pedal.h"
#include "../lib/no_pedal.h"

#include "common.hh"

using Debug     = ConditionalDebug<true, "Automotive-Send">;
using SonataAdc = SonataAnalogueDigitalConverter;
using namespace CHERI;
using namespace sonata::lcd;

// When using our model pedal, these are the maximum and minimum values that
// are measured through the ADC from the pedal's full range of motion. We
// hard-code these so that we can linearly transform from this range of ADC
// measurements to acceleration values.
#define PEDAL_MIN_ANALOGUE 310
#define PEDAL_MAX_ANALOGUE 1700

#define BACKGROUND_COLOR Color::Black
#define TEXT_COLOUR Color::White
#define ERROR_COLOUR Color::Red
#define PROTECT_COLOUR Color::Green

// A global flag used by the CHERI compartment error handler to detect when
// a capability violation has occurred, so that in our `loop` callback that
// is called every frame we can display a "CHERI Violation" message to the LCD.
static bool errorSeen = false;

// A global flag used in our `loop` callback so that we only draw the CHERI
// capability violation message once per demo run. Denotes whether the error
// message has been shown yet or not.
static bool errorMessageShown = false;

// Global driver objects for use in callback functionality
EthernetDevice *ethernet;
SonataAdc      *adc;
SonataLcd      *lcd;

/**
 * A very simplified and non-compliant version of vsprintf, which manually
 * handles formatting %u formatting specifiers provided in the format string
 * with the given arguments. This allows debug logging callbacks from both
 * legacy and CHERIoT to use the same string formatting interface.
 *
 * `buffer` is the buffer to write to. It is assumed that the size of this
 * buffer is large enough - no validation occurs.
 * `format` is the formatting string, potentially containing %u specifiers.
 * `args` is the variable list of unsigned integer arguments to format into
 * the string.
 *
 * Returns the actual length of the final formatted string.
 */
size_t vsprintf(char *buffer, const char *format, va_list args)
{
	size_t      strLen      = 0;
	const char *currentChar = format;
	while (*currentChar != '\0')
	{
		if (currentChar[0] == '%' && currentChar[1] == 'u')
		{
			size_t argVal = va_arg(args, unsigned int);
			size_t_to_str_base10(&buffer[strLen], argVal, 0, 0);
			while (buffer[strLen] != '\0')
			{
				strLen++;
			}
			currentChar++;
		}
		else
		{
			buffer[strLen++] = *currentChar;
		}
		currentChar++;
	}
	buffer[strLen] = '\0';
	if (strLen > 0 && buffer[strLen - 1] == '\n')
	{
		buffer[--strLen] = '\0';
	}
	return strLen;
}

/**
 * A function that writes a string to the UART console, based on a provided
 * format string and a list of arguments to format into that string. The
 * format string can only contain the "%u" format specifier, and all variable
 * arguments must accordingly be unsigned integers.
 *
 * `format` is the format string to write, potentially with %u specifiers.
 * Variable arguments are unsigned integers to format into the string.
 */
void write_to_uart(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsprintf(buffer, format, args);
	va_end(args);

	Debug::log(buffer, args);
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
	uint64_t curTime = rdcycle64();
	while (curTime < EndTime)
	{
		curTime = rdcycle64();
	}
	return curTime;
}

/**
 * A callback function to reset the global boolean flags related to CHERI
 * capability violation detection and error display, to be used to reset
 * the state of the CHERI firmware between demos.
 */
void reset_error_seen_and_shown()
{
	errorSeen         = false;
	errorMessageShown = false;
}

/**
 * A callback function which will use the global boolean CHERI capability
 * violation flags to determine conditionally whether to display a CHERI
 * violation message or not, and display it if so. This will only be
 * drawn once, and then `errorMessageShown` will be set true.
 */
void lcd_display_cheri_message()
{
	if (!errorSeen || errorMessageShown)
	{
		return;
	}

	errorMessageShown        = true;
	const Size  DisplaySize  = lcd->resolution();
	const Point Centre       = {DisplaySize.width / 2, DisplaySize.height / 2};
	const Point StringPos[3] = {
	  {Centre.x - 70, Centre.y + 27},
	  {Centre.x - 65, Centre.y + 40},
	  {Centre.x - 20, Centre.y + 50},
	};
	const Point CherryPos      = {Centre.x + 20, Centre.y + 50};
	const Rect CherryImageRect = Rect::from_point_and_size(CherryPos, {10, 10});

	lcd->draw_str(StringPos[0],
	              "Unexpected CHERI capability violation!",
	              BACKGROUND_COLOR,
	              ERROR_COLOUR);
	lcd->draw_str(StringPos[1],
	              "Memory has been safely protected",
	              BACKGROUND_COLOR,
	              PROTECT_COLOUR);
	lcd->draw_str(StringPos[2], "by CHERI.", BACKGROUND_COLOR, PROTECT_COLOUR);
	lcd->draw_image_rgb565(CherryImageRect, cherryImage10x10);
}

/**
 * Formats and draws a string to the LCD display based upon the provided
 * formatting and display information. The string can use formatting
 * specifiers, but only the "%u" formatting specifiers used with unsigned
 * integers are supported.
 *
 * `x` is the X-coordinate on the LCD of the top left of the string.
 * `y` is the Y-coordinate on the LCD of the top left of the string.
 * `font` is the font to render the string with.
 * `format` is the formatting string to write.
 * `backgroundColour` is the colour to use for the string's background.
 * `textColour` is the colour to render the text in.
 * The remaining variable arguments are unsigned integers used to fill
 * in the formatting specifiers in the format string.
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
	va_list args;
	va_start(args, textColour);
	char buffer[1024];
	vsprintf(buffer, format, args);
	va_end(args);

	Font stringFont;
	switch (font)
	{
		case LucidaConsole_10pt:
			stringFont = Font::LucidaConsole_10pt;
			break;
		case LucidaConsole_12pt:
			stringFont = Font::LucidaConsole_12pt;
			break;
		default:
			stringFont = Font::M3x6_16pt;
	}
	const Color BgColor = static_cast<Color>(backgroundColour);
	const Color FgColor = static_cast<Color>(textColour);
	lcd->draw_str({x, y}, buffer, BgColor, FgColor, stringFont);
}

/**
 * A callback function used to clean the LCD display with a given colour.
 *
 * `color` is the 32-bit colour to display.
 */
void lcd_clean(uint32_t color)
{
	lcd->clean(static_cast<Color>(color));
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
	const Rect DrawRect = Rect::from_point_and_size({x, y}, {w, h});
	lcd->fill_rect(DrawRect, static_cast<Color>(color));
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
	lcd->draw_image_rgb565(Rect::from_point_and_size({x, y}, {w, h}), data);
}

/**
 * A callback function used to read the GPIO joystick state.
 *
 * Returns the current joystick state as a byte, where each of the
 * 5 least significant bits corresponds to a given joystick input.
 */
uint8_t read_joystick()
{
	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);
	return static_cast<uint8_t>(gpio->read_joystick());
}

/**
 * A callback function used to read the pedal input as a digital value,
 * when the pedal is plugged into the mikroBUS INT pin under header P7.
 *
 * Returns true if the pedal is pressed down, and false if not.
 */
bool read_pedal_digital()
{
	auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);
	return (gpio->input & (1 << 13)) > 0;
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
	// To allow the user to connect the pedal to any of the analogue Arduino
	// pins read all measurements and take the max.
	const SonataAdc::MeasurementRegister Pins[6] = {
	  SonataAdc::MeasurementRegister::ArduinoA0,
	  SonataAdc::MeasurementRegister::ArduinoA1,
	  SonataAdc::MeasurementRegister::ArduinoA2,
	  SonataAdc::MeasurementRegister::ArduinoA3,
	  SonataAdc::MeasurementRegister::ArduinoA4,
	  SonataAdc::MeasurementRegister::ArduinoA5};
	for (auto pin : Pins)
	{
		maxPedalValue = MAX(maxPedalValue, adc->read_last_measurement(pin));
	}
	Debug::log("Measured Analogue Value: {}", static_cast<int>(maxPedalValue));

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
 * A callback function provided to the Ethernet driver which performs no
 * extra checking of frames, and exists only to satisfy the driver.
 */
bool null_ethernet_callback(uint8_t *buffer, uint16_t length)
{
	return true;
};

/**
 * A callback function to send an ethernet frame which wraps the Ethernet
 * driver's corresponding functionality.
 *
 * `buffer` is a byte array containing the data to be sent via Ethernet.
 * `length` is the length of that array.
 */
void send_ethernet_frame(const uint8_t *buffer, uint16_t length)
{
	// Copy the buffer into the heap to avoid CHERI capability issues
	uint8_t *frameBuf = new uint8_t[length];
	for (uint16_t i = 0; i < length; ++i)
	{
		frameBuf[i] = buffer[i];
	}
	if (!ethernet->send_frame(frameBuf, length, null_ethernet_callback))
	{
		Debug::log("Error sending frame...");
	}
	delete[] frameBuf;
}

/**
 * A CHERI Compartment Error handler for the CHERIoT sending firmware.
 * Upon an error occuring in the compartment, the `mtval` is extracted
 * to determine the cause. If the cause is a bounds or tag violation,
 * it is judged that this is *probably* the intended bug built into
 * the firmware, and as such the global `errorSeen` flag is set to
 * be used by callbacks to write information to the LCD. The program
 * counter is then modified in a quite hacky way to get the program
 * to continue onwards from wherever the violation occurred.
 */
extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	auto [exceptionCode, registerNumber] = extract_cheri_mtval(mtval);
	if (exceptionCode == CauseCode::BoundsViolation ||
	    exceptionCode == CauseCode::TagViolation)
	{
		if (!errorSeen)
		{
			// Two violations - one bound, one tag. Only show an error once
			errorSeen = true;
			Debug::log("Unexpected CHERI capability violation!");
			Debug::log("Memory has been safely protected by CHERI.");
		}
		// Hack: modify the program counter to continue. Do not replicate
		// unless necessary.
		frame->pcc = (void *)((uint32_t *)(frame->pcc) + 1);
		return ErrorRecoveryBehaviour::InstallContext;
	}

	Debug::log("Unexpected CHERI Capability violation. Stopping...");
	return ErrorRecoveryBehaviour::ForceUnwind;
}

// Initialise memory for the tasks used in the automotive demo library code.
// We use linker script sections to ensure that memory is contiguous in the
// worst conceivable way, such that an overwrite to the task two array by
// just one byte directly writes into the task one acceleration value.

__attribute__((
  section(".data.__contiguous.__task_two"))) static TaskTwo memTaskTwo = {
  .write = {0},
};

__attribute__((
  section(".data.__contiguous.__task_one"))) static TaskOne memTaskOne = {
  .acceleration = 0,
  .braking      = 0,
  .speed        = 0,
};

__attribute__((
  section(".data.__contiguous.__analogue_task_two"))) static AnalogueTaskTwo
  memAnalogueTaskTwo = {
    .framebuffer = {0},
};

__attribute__((
  section(".data.__contiguous.__analogue_task_one"))) static AnalogueTaskOne
  memAnalogueTaskOne = {
    .acceleration = 0,
    .braking      = 0,
};

/**
 * The main loop for the sending board in the automotive demo. This
 * simply infinitely dispalys the menu to the user so that they can
 * select a demo, and then loads the selected demo.
 */
[[noreturn]] void main_demo_loop()
{
	while (true)
	{
		switch (select_demo())
		{
			case DemoNoPedal:
				// Run simple timed demo with no pedal & using passthrough
				init_no_pedal_demo_mem(&memTaskOne, &memTaskTwo);
				run_no_pedal_demo(rdcycle64());
				break;
			case DemoJoystickPedal:
				// Run demo using a joystick as a pedal, with passthrough
				init_joystick_demo_mem(&memTaskOne, &memTaskTwo);
				run_joystick_demo(rdcycle64());
				break;
			case DemoDigitalPedal:
				// Run demo using an actual physical pedal but taking
				// a digital signal, using simulation instead of passthrough
				init_digital_pedal_demo_mem(&memTaskOne, &memTaskTwo);
				run_digital_pedal_demo(rdcycle64());
				break;
			case DemoAnaloguePedal:
				// Run demo using an actual physical pedal, taking an
				// analogue signal via an ADC, with passthrough.
				init_analogue_pedal_demo_mem(&memAnalogueTaskOne,
				                             &memAnalogueTaskTwo);
				run_analogue_pedal_demo(rdcycle64());
				break;
		}
	}
}

/**
 * The thread entry point for the sending (buggy) part of the automotive
 * demo. Initialises relevant Ethernet, LCD and ADC drivers, sets the
 * callback functions to be used by the demo library, and then starts
 * the main infinite loop.
 */
void __cheri_compartment("automotive_send") entry()
{
	// Initialise the LCD driver and calculate display information
	lcd               = new SonataLcd();
	Size  displaySize = lcd->resolution();
	Point centre      = {displaySize.width / 2, displaySize.height / 2};
	lcd->clean(BACKGROUND_COLOR);

	// Initialise Ethernet driver for use via callback
	ethernet = new EthernetDevice();
	ethernet->mac_address_set();

	// Wait until a good physical ethernet link to start the demo
	if (!ethernet->phy_link_status())
	{
		Debug::log("Waiting for a good physical ethernet link...\n");
		const Point WaitingStrPos[2] = {
		  {centre.x - 55, centre.y - 5},
		  {centre.x - 30, centre.y + 5},
		};
		lcd->draw_str(WaitingStrPos[0],
		              "Waiting for a good physical",
		              BACKGROUND_COLOR,
		              TEXT_COLOUR);
		lcd->draw_str(
		  WaitingStrPos[1], "ethernet link...", BACKGROUND_COLOR, TEXT_COLOUR);
	}
	while (!ethernet->phy_link_status())
	{
		thread_millisecond_wait(50);
	}

	// Wait an additional 0.25 s to give the receiving board time to setup.
	thread_millisecond_wait(250);

	// Initialise the ADC driver for use via callback
	SonataAdc::ClockDivider adcClockDivider =
	  (CPU_TIMER_HZ / SonataAdc::MinClockFrequencyHz) / 2;
	adc = new SonataAdc(adcClockDivider, SonataAdc::PowerDownMode::None);

	// Adapt the common automotive library for CHERIoT drivers
	constexpr uint32_t CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
	init_lcd(displaySize.width, displaySize.height);
	init_callbacks({
	  .uart_send           = write_to_uart,
	  .wait                = wait,
	  .waitTime            = 120 * CyclesPerMillisecond,
	  .time                = rdcycle64,
	  .loop                = lcd_display_cheri_message,
	  .start               = reset_error_seen_and_shown,
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

	// Begin the main demo loop
	main_demo_loop();

	// Driver cleanup
	delete lcd;
	delete ethernet;
	delete adc;
}
