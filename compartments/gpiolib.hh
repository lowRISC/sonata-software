// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <optional>

struct LedHandle;

__cheri_compartment("gpiolib") auto aquire_led(uint8_t value)
  -> std::optional<LedHandle *>;
__cheri_compartment("gpiolib") void release_led(LedHandle *);
__cheri_compartment("gpiolib") bool toggle_led(LedHandle *);
