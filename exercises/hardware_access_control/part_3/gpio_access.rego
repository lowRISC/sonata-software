# Copyright lowRISC Contributors.
# SPDX-License-Identifier: Apache-2.0

package gpio_access

# Checks only `gpio_access` has access to the GPIO MMIO
default only_gpio_access_has_access := false
only_gpio_access_has_access {
  data.compartment.mmio_allow_list("gpio", { "gpio_access" })
}

# Checks only `blinky_dynamic` and `led_walk_dynamic` have access
# to the `gpio_access` compartment.
default whitelisted_compartments_only := false
whitelisted_compartments_only {
  only_gpio_access_has_access
  data.compartment.compartment_allow_list("gpio_access", {
    "led_walk_dynamic",
    "blinky_dynamic"
  })
}
