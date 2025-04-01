// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <debug.hh>
#include <platform-ethernet.hh>
#include <platform-gpio.hh>
#include <platform-pwm.hh>
#include <thread.h>

#include "../../../libraries/lcd.hh"

#include "../lib/automotive_common.h"

#include "common.hh"

using Debug = ConditionalDebug<true, "Automotive-Receive">;
using namespace CHERI;
using namespace sonata::lcd;
using SonataPwm = SonataPulseWidthModulation::General;

#define PWM_MAX_DUTY_CYCLE 255 // Max is 100%.

// For our car, at least 25/255 duty cycle is needed to overcome
// inertia and drive the motor, so we set "zero" to be 20.
#define PWM_MIN_DUTY_CYCLE 20

// How often the main loop is updated. This includes the simulated speed
// when applicable, writing to the display and polling for Ethernet packets.
#define DELTA_TIME_MSEC 80

// This is what we define the highest possible acceleration value to be,
// and thus the highest speed that we can give to the car. This is a linear
// mapping under PWM but is unlikely to be accurate to the car itself.
#define MODEL_CAR_MAX_SPEED 200
#define MODEL_CAR_MIN_SPEED 0

// These simulation constants are all arbitrary numbers to get a right
// feeling, and are not necessarily realistic values.
#define MODEL_CAR_ENGINE_HORSEPOWER 500
#define MODEL_CAR_BRAKING_FORCE MODEL_CAR_ENGINE_HORSEPOWER
#define MODEL_CAR_AIR_DENSITY 1
#define MODEL_CAR_DRAG_COEFFICIENT 1
#define MODEL_CAR_REFERENCE_AREA 5
#define MODEL_CAR_FRICTION_COEFFICIENT 40

// Implement fixed-point arithmetic in the sim by accumulating our accelerating
// and decelerating forces, and then make integer changes when a multiple of
// this divider has been accumulated. We use 1000 for 1000 Msec.
#define SIM_DIVIDER 1000

#define BACKGROUND_COLOUR Color::Black
#define SEGMENT_OFF_COLOUR static_cast<Color>(0x0C0C0C)
#define TEXT_BRIGHT_COLOUR Color::White
#define TEXT_DIMMED_COLOUR static_cast<Color>(0x8F8F8F)
#define TEXT_DARK_COLOUR static_cast<Color>(0x808080)

// A struct to store information of the car, used in both operating modes.
struct CarInfo
{
	uint64_t acceleration;
	uint64_t braking;
	uint64_t speed;
};

// Driver structs/classes
EthernetDevice           *ethernet;
SonataLcd                *lcd;
volatile SonataGpioBoard *gpio;

// Sets the operating mode that the demo is running in. Default is passthrough.
DemoMode operatingMode = DemoModePassthrough;

/**
 * A function for busy-waiting whilst also constantly polling for the joystick
 * to be pressed. If the joystick is pressed at any time, it means that the
 * current car state values should be reset.
 *
 * `endTime` is the cycle/time that should be waited until.
 * `flagReset` is a boolean outparameter that will be set true if the joystick
 * is pressed.
 *
 * Returns the new current cycle/time after waiting.
 */
uint64_t wait_with_input(const uint64_t EndTime, bool *flagReset)
{
	uint64_t currentTime = rdcycle64();
	while (currentTime < EndTime)
	{
		currentTime = rdcycle64();
		*flagReset |= gpio->read_joystick().is_pressed();
	}
	return currentTime;
}

/**
 * Polls for and attempts to receive an Ethernet frame for the purposes of the
 * automotive demo. This expects frames of a specified format, and does not
 * perform much validation on these frames.
 *
 * `carInfo` is the model car's information, to be updated with packet data.
 */
void receive_ethernet_frame(CarInfo *carInfo)
{
	// Poll for a frame
	Debug::log("Polling for ethernet frame...");
	std::optional<EthernetDevice::Frame> maybeFrame = ethernet->receive_frame();
	if (!maybeFrame.has_value())
	{
		return;
	}
	Debug::log("Received a frame with some value!");

	// Parse the Ethernet Header information into the frame
	DemoFrame frame;
	uint32_t  index = 0;
	for (; index < 14; index++)
	{
		frame.header.macDestination[index] = maybeFrame->buffer[index];
	}

	// Parse the first bit to determine if the frame is a demo mode frame or
	// a pedal data frame.
	frame.type = static_cast<FrameType>(maybeFrame->buffer[index++]);
	switch (frame.type)
	{
		case FrameDemoMode:
			// Receive demo mode frame; set the operation mode and clear LCD
			frame.data.mode = static_cast<DemoMode>(maybeFrame->buffer[index]);
			operatingMode   = frame.data.mode;
			Debug::log("Received a mode frame with mode {}",
			           static_cast<unsigned int>(operatingMode));
			lcd->clean(BACKGROUND_COLOUR);
			break;

		case FramePedalData:
			// Receive pedal data frame; parse the info into the CarInfo struct
			for (unsigned char &__capability i : frame.data.pedalData)
			{
				i = maybeFrame->buffer[index++];
			}
			carInfo->acceleration = 0;
			carInfo->braking      = 0;
			for (uint32_t i = 0; i < 8; ++i)
			{
				uint32_t shift = (7 - i) * 8;
				carInfo->acceleration |=
				  (static_cast<uint64_t>(frame.data.pedalData[i])) << shift;
				carInfo->braking |=
				  (static_cast<uint64_t>(frame.data.pedalData[i + 8])) << shift;
			}
			Debug::log("Received a pedal data frame with acceleration {}",
			           static_cast<unsigned int>(carInfo->acceleration));
			break;

		default:
			Debug::log("Error: Unknown frame type!");
			return;
	}
}

/**
 * Signals the car with a speed value corresponding to the current value of
 * `carInfo->speed`. This speed is compared with the defined speed ranges
 * and mapped to a corresponding PWM output, and this PWM is then
 * signalled to drive the car motor by the corresponding amount.
 *
 * `carInfo` is the current car information, to use to signal the car.
 */
void pwm_signal_car(CarInfo *carInfo)
{
	// Clamp the car speed between its min and max values.
	uint32_t carSpeed = MIN(carInfo->speed, MODEL_CAR_MAX_SPEED);
	carSpeed          = MAX(carSpeed, MODEL_CAR_MIN_SPEED);

	// Linearly transform the car's speed to the proportionally equivalent
	// duty cycle representation in the valid duty cycle range.
	constexpr uint32_t DutyRange   = PWM_MAX_DUTY_CYCLE - PWM_MIN_DUTY_CYCLE;
	constexpr uint32_t SpeedRange  = MODEL_CAR_MAX_SPEED - MODEL_CAR_MIN_SPEED;
	const uint32_t     SpeedOffset = carSpeed - MODEL_CAR_MIN_SPEED;
	// We must * DUTY_RANGE before we / SPEED_RANGE as we don't have floats
	const uint32_t PwmDutyCycle =
	  (SpeedOffset * DutyRange) / SpeedRange + PWM_MIN_DUTY_CYCLE;
	auto pwm = MMIO_CAPABILITY(SonataPwm, pwm);
	pwm->get<0>()->output_set(PWM_MAX_DUTY_CYCLE, PwmDutyCycle);
}

/**
 * When the Automotive demo is operating in simulation mode, this function can
 * be called with the car's current information to carry out one frame / time
 * step of a basic Eulerian physics simulation to roughly simulate an actual
 * car's operation as it drives.
 *
 * `carInfo` is the current car information, used as inputs and as the output
 * of the simulation.
 *
 * Define `SIM_DEBUG_PRINT` to log all the simulation debug values.
 */
void update_speed_estimate(CarInfo *carInfo)
{
	static uint32_t partialAccelAccum;
	static uint32_t partialDecelAccum;

#ifdef SIM_DEBUG_PRINT
	Debug::log("SimulationPartialAccumDivider: {}", SIM_DIVIDER);
	Debug::log("DeltaTimeMsec: {}", DELTA_TIME_MSEC);
	Debug::log("Horsepower: {}", MODEL_CAR_ENGINE_HORSEPOWER);
	Debug::log("Braking force: {}", MODEL_CAR_BRAKING_FORCE);
	Debug::log("AirDensity: {}", MODEL_CAR_AIR_DENSITY);
	Debug::log("DragCoefficient: {}", MODEL_CAR_DRAG_COEFFICIENT);
	Debug::log("ReferenceArea: {}", MODEL_CAR_REFERENCE_AREA);
	Debug::log("Acceleration: {}", (int)carInfo->acceleration);
	Debug::log("CarSpeed: {}", (int)carInfo->speed);
	Debug::log("PartialAccelAccum: {}", (int)partialAccelAccum);
	Debug::log("PartialDecelAccum: {}", (int)partialDecelAccum);
	Debug::log("---");
#endif // SIM_DEBUG_PRINT

	// Calculate forward driving acceleration force
	partialAccelAccum +=
	  (MODEL_CAR_ENGINE_HORSEPOWER * carInfo->acceleration) / DELTA_TIME_MSEC;
	// Calculate backward resistance decelaration forces
	partialDecelAccum +=
	  (MODEL_CAR_AIR_DENSITY * MODEL_CAR_DRAG_COEFFICIENT *
	   MODEL_CAR_REFERENCE_AREA * carInfo->speed * carInfo->speed) /
	  DELTA_TIME_MSEC;
	partialDecelAccum +=
	  (MODEL_CAR_BRAKING_FORCE * carInfo->braking) / DELTA_TIME_MSEC;

	// Apply accumulated fixed-point forces
	if (partialAccelAccum > SIM_DIVIDER)
	{
		carInfo->speed += partialAccelAccum / SIM_DIVIDER;
		partialAccelAccum = partialAccelAccum % SIM_DIVIDER;
	}
	if (partialDecelAccum > SIM_DIVIDER)
	{
		const uint32_t Decrease = partialDecelAccum / SIM_DIVIDER;
		carInfo->speed -= MIN(Decrease, carInfo->speed);
		partialDecelAccum = partialDecelAccum % SIM_DIVIDER;
	}

	// Reset partial accumulation information when the car is at rest.
	if (carInfo->speed == 0 && carInfo->acceleration == 0)
	{
		partialAccelAccum = 0;
		partialDecelAccum = 0;
	}
}

// Stores information about a given segment in a Seven Segment display.
struct Segment
{
	Point pos;
	bool  vertical;
};

// Defines the seven segments of a seven-segment display, with a hardcoded size
// where each segment is 5x15.
static const Segment SevenSegments[7] = {
  {{5, 0}, false},  // A         ## <-- A
  {{0, 5}, true},   // B    B-> #  #
  {{20, 5}, true},  // C        #  # <- C
  {{5, 20}, false}, // D         ## <-- D
  {{0, 25}, true},  // E    E-> #  #
  {{20, 25}, true}, // F        #  # <- F
  {{5, 40}, false}, // G         ## <-- G
};

// Defines the seven-segment representation of all digits 0 through 9 as
// bytes with each bit corresponding to a segment in sequence.
static const uint8_t NumberSevenSegments[10] = {
  0b01110111, // 0 = A,B,C,E,F,G
  0b00100100, // 1 = C,F
  0b01011101, // 2 = A,C,D,E,G
  0b01101101, // 3 = A,C,D,F,G
  0b00101110, // 4 = B,C,D,F
  0b01101011, // 5 = A,B,D,F,G
  0b01111011, // 6 = A,B,D,E,F,G
  0b00100101, // 7 = A,C,F
  0b01111111, // 8 = A,B,C,D,E,F,G
  0b01101111, // 9 = A,B,C,D,F,G
};

/**
 * Displays a seven segment number on the LCD display.
 *
 * `pos` is the top-left corner of the seven segment number to draw.
 * `colour` is the colour to draw the enabled segments in.
 * `background_colour` is the colour to draw the disabled segments in.
 * `segments` is a byte, where each bit being set to 1 corresponds to one of
 * the corresponding 7 segments being enabled. The MSB does nothing. Segment
 * definitions are based on positions in the `SevenSEgments` array.
 */
void display_seven_segment_digit(Point   pos,
                                 Color   colour,
                                 Color   backgroundColour,
                                 uint8_t segments)
{
	for (uint8_t segmentNum = 0; segmentNum < 7; ++segmentNum)
	{
		const Segment Info     = SevenSegments[segmentNum];
		const Point   Position = {pos.x + Info.pos.x, pos.y + Info.pos.y};
		const Size    RectSize = {(Info.vertical) ? 5u : 15u,
                               (Info.vertical) ? 15u : 5u};
		Color rectColour = backgroundColour;
		if (segments & (1u << segmentNum))
		{
			rectColour = colour;
		}
		Rect segmentRect = Rect::from_point_and_size(Position, RectSize);
		lcd->fill_rect(segmentRect, rectColour);
	}
}

/**
 * Displays a speedometer as a 3-digit seven segment number on the LCD display.
 *
 * `pos` is the top-left corner of the speedometer to draw.
 * `colour` is the colour to draw the speedometer in.
 * `inactiveColour` is the colour to draw inactive speedometer segments in.
 * `carSpeed` is the current speed information to display on the speedometer.
 */
void display_speedometer(Point    pos,
                         Color    colour,
                         Color    inactiveColour,
                         uint64_t carSpeed)
{
	const uint32_t Speed     = MIN(carSpeed, 999);
	const uint32_t Digits[3] = {
	  Speed % 10, (Speed / 10) % 10, (Speed / 100) % 10};

	// Find the first (leading) non-zero number for formatting.
	size_t firstNonZero = 0;
	for (size_t i = 3; i > 0; --i)
	{
		if (Digits[i - 1] != 0)
		{
			firstNonZero = i - 1;
			break;
		}
	}

	bool drawEmptyNum = false;
	for (size_t i = 0; i < 3; i++)
	{
		Point   numPos   = {pos.x + 70 - 35 * i, pos.y};
		uint8_t segments = drawEmptyNum ? 0 : NumberSevenSegments[Digits[i]];
		display_seven_segment_digit(numPos, colour, inactiveColour, segments);
		drawEmptyNum |= (i >= firstNonZero); // Remove leading 0s
	}
}

/**
 * Updates the state of the demo for a single frame, performing actions specific
 * to when the demo is operating in passthrough mode. This involves directly
 * passing the acceleration to the speed, and updating the LCD accordingly.
 *
 * `carInfo` is simply the state of the car at the current time.
 * `centre` is the centre of the LCD screen, to use for displaying.
 */
void update_demo_passthrough(CarInfo *carInfo, Point centre)
{
	// Directly pass through acceleration to speed.
	carInfo->speed = carInfo->acceleration;
	Debug::log("Current acceleration is {}", carInfo->acceleration);

	// Draw speed information to the LCD
	const Point LabelPos = {centre.x - 18, centre.y - 50};
	lcd->draw_str(LabelPos,
	              "Speed",
	              BACKGROUND_COLOUR,
	              TEXT_BRIGHT_COLOUR,
	              Font::LucidaConsole_10pt);
	const Color SpeedColor = (carInfo->speed >= 50) ? Color::Red : Color::White;
	display_speedometer({centre.x - 48, centre.y - 30},
	                    SpeedColor,
	                    SEGMENT_OFF_COLOUR,
	                    carInfo->speed);
}

/**
 * Updates the state of the demo for a single frame, performing actions specific
 * to when the demo is operating in simulation mode. This involves using the
 * car's speed and acceleration to update its new speed, and updating the
 * LCD accordingly.
 *
 * `carInfo` is simply the state of the car at the current time.
 * `centre` is the centre of the LCD screen, to use for displaying.
 */
void update_demo_simulation(CarInfo *carInfo, Point centre)
{
	update_speed_estimate(carInfo);

	// Format the current acceleration into a string for display
	char accelerationStr[50];
	memcpy(accelerationStr, "Acceleration: ", 14);
	size_t_to_str_base10(
	  &accelerationStr[14], static_cast<size_t>(carInfo->acceleration), 0, 10);

	// Display acceleration & speed information to the LCD
	Color accelerationColor =
	  (carInfo->acceleration > 100) ? Color::Red : TEXT_DIMMED_COLOUR;
	const Point AccelerationLabelPos = {centre.x - 70, centre.y - 56};
	const Point SpeedLabelPos        = {centre.x - 20, centre.y - 36};
	const Point SpeedometerPos       = {centre.x - 48, centre.y - 18};
	Color       speedColor           = TEXT_BRIGHT_COLOUR;
	if (65 < carInfo->speed & carInfo->speed < 75)
	{
		speedColor = Color::Green;
	}
	else if (carInfo->speed >= 75)
	{
		speedColor = Color::Red;
	}
	lcd->draw_str(AccelerationLabelPos,
	              accelerationStr,
	              BACKGROUND_COLOUR,
	              accelerationColor,
	              Font::LucidaConsole_10pt);
	lcd->draw_str(SpeedLabelPos,
	              "Speed:",
	              BACKGROUND_COLOUR,
	              TEXT_BRIGHT_COLOUR,
	              Font::LucidaConsole_10pt);
	display_speedometer(
	  SpeedometerPos, speedColor, SEGMENT_OFF_COLOUR, carInfo->speed);
}

/**
 * The main update loop for the receiving board in the automotive demo.
 *
 * Define `AUTOMOTIVE_WAIT_FOR_ETHERNET` to force the receiving demo to
 * also wait for a good Ethernet link. Otherwise it just progresses
 * even if no connections are seen.
 */
[[noreturn]] void main_demo_loop()
{
	// Initialise car info struct to store car parameters.
	CarInfo carInfo = {.acceleration = 0, .braking = 0, .speed = 0};

	// Calculate LCD information
	const Size  DisplaySize   = lcd->resolution();
	const Point Centre        = {DisplaySize.width / 2, DisplaySize.height / 2};
	const Point ResetLabelPos = {Centre.x - 55, Centre.y + 42};

	// Pre-compute timing information to control the demo speed.
	constexpr uint32_t CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
	constexpr uint32_t WaitTime = DELTA_TIME_MSEC * CyclesPerMillisecond;
	uint64_t           prevTime = rdcycle64();

	// Main loop, updates each frame.
	while (true)
	{
		receive_ethernet_frame(&carInfo);
		pwm_signal_car(&carInfo);
		lcd->draw_str(ResetLabelPos,
		              "Press the joystick to reset!",
		              BACKGROUND_COLOUR,
		              TEXT_DARK_COLOUR,
		              Font::M3x6_16pt);

		// Update according to the operation mode, and display to LCD
		if (operatingMode == DemoModeSimulated)
		{
			update_demo_simulation(&carInfo, Centre);
		}
		else
		{ // Default to passthrough mode
			update_demo_passthrough(&carInfo, Centre);
		}

		// Check whether to reset the car's state using GPIO joystick input
		bool flagReset = false;
		prevTime       = wait_with_input(prevTime + WaitTime, &flagReset);
		if (flagReset)
		{
			carInfo = {.acceleration = 0, .braking = 0, .speed = 0};
		}
	}
}

/**
 * The thread entry point for the receiving part of the automotive demo.
 * Initialises relevant Ethernet, LCD and GPIO drivers, and starts the
 * main infinite loop.
 *
 * If `AUTOMOTIVE_WAIT_FOR_ETHERNET` is defined, then the receiving board
 * of the demo will not progress to its main loop until a good physical
 * Ethernet link is detected (i.e. both boards are connected & powered).
 */
[[noreturn]] void __cheri_compartment("automotive_receive") entry()
{
	// Initialise ethernet driver for use via callback
	ethernet = new EthernetDevice();
	ethernet->mac_address_set({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB});
#ifdef AUTOMOTIVE_WAIT_FOR_ETHERNET
	while (!ethernet->phy_link_status())
	{
		thread_millisecond_wait(50);
	}
#endif // AUTOMOTIVE_WAIT_FOR_ETHERNET

	// Initialise the LCD & GPIO drivers
	lcd = new SonataLcd();
	lcd->clean(BACKGROUND_COLOUR);
	gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);

	// Start the main loop
	main_demo_loop();

	// Driver cleanup
	delete ethernet;
	delete lcd;
}
