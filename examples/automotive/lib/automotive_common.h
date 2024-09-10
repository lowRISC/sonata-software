// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef AUTOMOTIVE_COMMON_H
#define AUTOMOTIVE_COMMON_H

#include <stdbool.h>
#include <stdint.h>

// Possible GPIO inputs for the joystick, and which GPIO bit they correspond to
enum JoystickDir
{
	Left    = 1 << 0,
	Up      = 1 << 1,
	Pressed = 1 << 2,
	Down    = 1 << 3,
	Right   = 1 << 4,
};

// BGR Colours used for LCD display in the automotive demo
typedef enum LCDColor
{
	ColorBlack      = 0x000000,
	ColorWhite      = 0xFFFFFF,
	ColorGrey       = 0xCCCCCC,
	ColorDarkGrey   = 0xA0A0A0,
	ColorDarkerGrey = 0x808080,
	ColorRed        = 0x0000FF,
	ColorGreen      = 0x00FF00,
	ColorBlue       = 0xFF0000,
} LCDColor;

// Fonts available for LCD rendering in the automotive demo
typedef enum LcdFont
{
	M3x6_16pt,          // NOLINT
	LucidaConsole_10pt, // NOLINT
	LucidaConsole_12pt, // NOLINT
} LcdFont;

// Minimal Ethernet Header for sending frames
typedef struct EthernetHeader
{
	uint8_t macDestination[6];
	uint8_t macSource[6];
	uint8_t type[2];
} __attribute__((__packed__)) EthernetHeader;

// The possible "modes" that the receiving board of the automotive demo can
// operate in.
typedef enum DemoMode
{
	/**
	 * Passthrough mode is where the acceleration pedal value received from
	 * the sending board is directly passed-through to the car's speed, and
	 * used to drive the car via PWM. There is no simulation.
	 */
	DemoModePassthrough = 0,

	/**
	 * Simulation mode is used to make digital inputs viable, and is where
	 * the acceleration pedal value is instead used as a driving force in
	 * a Eulerian simulation of the car, where the car's simulated speed
	 * is instead what is used to drive the car via PWM.
	 */
	DemoModeSimulated = 1,
} DemoMode;

// The two types of Ethernet frames that can be sent in the automotive demo.
typedef enum FrameType
{
	// The Demo Mode frame is used to set which mode the receiving board
	// should be running in. It is sent once at the start of each demo.
	FrameDemoMode = 0,

	// The Pedal Data frame is used to send measured pedal data from the sending
	// board to the receiving board. It is sent many times a second during
	// demos.
	FramePedalData = 1,
} FrameType;

/**
 * A struct representing the structure of an Ethernet frame that is transmitted
 * in the automotive demo. Each frame consists of a header, a frame type, and
 * then some data which is either the demo mode to run in, or the transmitted
 * pedal data.
 */
typedef struct DemoFrame
{
	EthernetHeader header;
	FrameType      type;
	union
	{
		DemoMode mode;
		uint8_t  pedalData[16];
	} data;
} DemoFrame;

// A simple struct representing a (two-dimensional) size in LCD screen space.
typedef struct
{
	uint32_t x;
	uint32_t y;
} LcdSize;

/**
 * A struct listing every LCD callback that must be passed to the automotive
 * demo library for the demos to be able to function. It is valid for callbacks
 * to do nothing but they all must at least exist, or the library will attempt
 * to call a function that is not initialised.
 */
typedef struct LcdCallbacks
{
	// A function that renders a string to the LCD
	void (*draw_str)(uint32_t    x, // NOLINT
	                 uint32_t    y,
	                 LcdFont     font,
	                 const char *format,
	                 uint32_t    backgroundColour,
	                 uint32_t    textColour,
	                 ...);
	// A function that fills/cleans the LCD with a given colour.
	void (*clean)(uint32_t color);
	// A function that fills a rectangle on the LCD with a given colour.
	void (*fill_rect)(uint32_t x, // NOLINT
	                  uint32_t y,
	                  uint32_t w,
	                  uint32_t h,
	                  uint32_t color);
	// A function that draws a given RGB565 image to the LCD
	void (*draw_img_rgb565)(uint32_t       x, // NOLINT
	                        uint32_t       y,
	                        uint32_t       w,
	                        uint32_t       h,
	                        const uint8_t *data);
} LcdCallbacks;

/**
 * A struct listing every callback that must be passed to the automotive demo
 * library for the demos to be able to function. It is valid for callbacks to do
 * nothing but they all must at least exist, or the library will attempt to call
 * a function that is not initialised.
 */
typedef struct AutomotiveCallbacks
{
	// A function that sends debug log data via UART.
	void (*uart_send)(const char *format, ...); // NOLINT
	// A function that waits until the given time, relative to times received
	// from the `time` callback.
	uint64_t (*wait)(const uint64_t EndTime);
	// The amount of time to wait between updates. Relative to the times
	// received from the `wait`/`time` callbacks.
	uint64_t waitTime;
	// A function that returns the current time as a 64-bit integer.
	uint64_t (*time)();
	// A callback that is called at the end of every update/frame
	void (*loop)();
	// A callback that is called once at the start of the demo
	void (*start)();
	// A function that reads the joystick information via GPIO
	uint8_t (*joystick_read)(); // NOLINT
	// A function that reads the pedal, as a digital (Boolean) value.
	bool (*digital_pedal_read)(); // NOLINT
	// A function that reads the pedal, as an analogue value.
	uint32_t (*analogue_pedal_read)(); // NOLINT
	// A function that transmits an Ethernet packet to the receiving board
	void (*ethernet_transmit)(const uint8_t *buffer, uint16_t length); // NOLINT
	LcdCallbacks lcd;
} AutomotiveCallbacks;

/**
 * A struct defining the memory layout used by "Task One" in the "No Pedal",
 * "Joystick Pedal", and "Digital Pedal" demos.
 */
typedef struct TaskOne
{
	uint64_t acceleration;
	uint64_t braking;
	uint64_t speed;
} TaskOne;

/**
 * A struct definining the memory layout used by "Task Two" in the "No Pedal",
 * "Joystick Pedal", and "Digital Pedal" demos.
 */
typedef struct TaskTwo
{
	uint64_t write[100];
} TaskTwo;

// Externs to be set when initialising the automotive demo library.
extern LcdSize              lcdSize, lcdCentre;
extern AutomotiveCallbacks  callbacks;
extern const EthernetHeader FixedDemoHeader;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
	void init_lcd(uint32_t width, uint32_t height);
	void init_callbacks(AutomotiveCallbacks automotiveCallbacks);
	bool joystick_in_direction(uint8_t joystick, enum JoystickDir direction);
	void send_data_frame(const uint64_t *data,
	                     EthernetHeader  header,
	                     uint16_t        length);
	void send_mode_frame(EthernetHeader header, DemoMode mode);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif // AUTOMOTIVE_COMMON_H
