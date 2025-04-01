// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <optional>

struct LedHandle;
typedef CHERI_SEALED(LedHandle *) SealedLedHandle;

__cheri_compartment("gpio_access") auto acquire_led(uint8_t value)
  -> std::optional<SealedLedHandle>;
__cheri_compartment("gpio_access") void release_led(SealedLedHandle);
__cheri_compartment("gpio_access") bool toggle_led(SealedLedHandle);
