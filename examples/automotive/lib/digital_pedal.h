// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef AUTOMOTIVE_DIGITAL_PEDAL_H
#define AUTOMOTIVE_DIGITAL_PEDAL_H

#include <stdint.h>

#include "automotive_common.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
	void init_digital_pedal_demo_mem(TaskOne *taskOne, TaskTwo *taskTwo);
	void run_digital_pedal_demo(uint64_t initTime);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif // AUTOMOTIVE_DIGITAL_PEDAL_H
