// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef AUTOMOTIVE_MENU_H
#define AUTOMOTIVE_MENU_H

#include <stdint.h>

typedef enum DemoApplication
{
	DemoAnaloguePedal = 0,
	DemoDigitalPedal  = 1,
	DemoJoystickPedal = 2,
	DemoNoPedal       = 3,
} DemoApplication;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
	DemoApplication select_demo();
#ifdef __cplusplus
}
#endif //__cplusplus

#endif // AUTOMOTIVE_MENU_H
