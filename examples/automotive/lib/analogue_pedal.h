// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef AUTOMOTIVE_ANALOGUE_PEDAL_H
#define AUTOMOTIVE_ANALOGUE_PEDAL_H

#include <stdint.h>

#include "automotive_common.h"

// Define the range of outputs expected from the analogue pedal for this demo
#define DEMO_ACCELERATION_PEDAL_MIN 0
#define DEMO_ACCELERATION_PEDAL_MAX 50
// We clamp at 50 out of 100 so that the speedup can actually be observed

/**
 * A struct defining the memory layout used by "Task One" in the "Analogue
 * Pedal" demo.
 */
typedef struct AnalogueTaskOne
{
	uint64_t acceleration;
	uint64_t braking;
} AnalogueTaskOne;

/**
 * A struct defining the memory layout used by "Task Two" in the "Analogue
 * Pedal" demo. This consists of a "volume" value storing the current
 * volume, and a framebuffer storing the colours that will be drawn to
 * the volume bar next frame.
 */
typedef struct AnalogueTaskTwo
{
	uint64_t volume;
	uint64_t framebuffer[20];
} AnalogueTaskTwo;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
	void init_analogue_pedal_demo_mem(AnalogueTaskOne *taskOne,
	                                  AnalogueTaskTwo *taskTwo);
	void run_analogue_pedal_demo(uint64_t initTime);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif // AUTOMOTIVE_ANALOGUE_PEDAL_H
