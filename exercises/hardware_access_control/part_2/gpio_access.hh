// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <optional>

struct LedHandle;

__cheri_compartment("gpio_access") auto acquire_led(uint8_t value)
  -> std::optional<LedHandle *>;
__cheri_compartment("gpio_access") void release_led(LedHandle *);
__cheri_compartment("gpio_access") bool toggle_led(LedHandle *);
