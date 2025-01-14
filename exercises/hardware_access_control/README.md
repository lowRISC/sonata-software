<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Hardware access control exercise

If you haven't already, please go to the '[building the exercises][]' section to see how the exercises are built.

[Building the Exercises]: ../README.md#building-the-exercises

In this exercise we utilise the compartmentalisation available in CHERIoT RTOS to control access to a hardware peripheral: the LEDs.

For this exercise, when the [`xmake.lua`][] build file is mentioned `exercises/hardware_access_control/xmake.lua` is being referred to.

[`xmake.lua`]: ../../exercises/hardware_access_control/xmake.lua

## Part 1

Let's start with the firmware image called `hardware_access_part_1` in the [`xmake.lua`][] file.
This image has two threads running two compartments: `blinky_raw` and `led_walk_raw`.
Compartment `blinky_raw` simply toggles an LED and compartment `led_walk_raw` walks through all the LEDs toggling them as it goes.
The sources of these compartments can be found in [`exercises/hardware_access_control/part_1/`][].

[`exercises/hardware_access_control/part_1/`]: https://github.com/lowRISC/sonata-software/tree/main/exercises/hardware_access_control/part_1

Let's look inside [`blinky_raw`][].
It uses the RTOS' `MMIO_CAPABILITY` macro to get the capability that grants it access to the GPIO MMIO region.
This magic macro will handle adding the MMIO region to the compartment's imports and mapping it to a type, in this case `SonataGpioBoard` (from [`platform-gpio.hh`][]).
*For more information on this macro, see [the drivers section of CHERIoT programmers guide][].*

[`blinky_raw`]: ./part_1/blinky_raw.cc
[the drivers section of CHERIoT programmers guide]: https://cheriot.org/book/top-drivers-top.html#mmio_capabilities
[`platform-gpio.hh`]: ../../cheriot-rtos/sdk/include/platform/sunburst/platform-gpio.hh

This is great!
If you build and [load][running on fpga] the `hardware_access_part_1` firmware on the FPGA, you have flashing LEDs!
What more could one want?

[running on fpga]: ../../doc/guide/running-software.md#running-on-the-sonata-fpga

Well maybe some level of access control.
Currently both `blinky_raw` and `led_walk_raw` have access to all of the GPIO ports, and neither can trust the other compartment isn't toggling the LED when they are not looking.
The keen-eyed among you will have noticed that this is already happening with both toggling user LED 7.

## Part 2

Let's introduce some access control for the LEDs.
To do this, we can create a new compartment `gpio_access` with sole access to the GPIO MMIO region.
This compartment will arbitrate access to the LED outputs by making use of CHERIoT's sealing mechanism.
When a compartment seals a capability, it can no longer be dereferenced or modified until it is unsealed by a compartment with the capability to do so.
The `gpio_access` compartment creates these sealing capabilities as LED *handles* that it can give to other compartments.
These other compartments can't use the handles directly, but can only pass them to `gpio_access` which can unseal them and use them.
In this case, they only point to an `LedHandle` structure that holds the index of an LED.
They are purely used as a proof of LED ownership.
*For more information on sealing, see the [`cheriot-rtos/examples/05.sealing/`][].*

[`cheriot-rtos/examples/05.sealing/`]: ../../cheriot-rtos/examples/05.sealing/

For this part, `blinky_raw` and `led_walk_raw` have been adapted to use this new compartment and renamed `blinky_dynamic` and `led_walk_dynamic`.
You'll notice these compartments use `add_deps` in the [`xmake.lua`][] file to declare that they depend on `gpio_access`.
Take a moment to look at the sources for these compartments in [`exercises/hardware_access_control/part_2/`][].

[`exercises/hardware_access_control/part_2/`]: https://github.com/lowRISC/sonata-software/tree/main/exercises/hardware_access_control/part_2

If you now run the `hardware_access_part_2` firmware on the FPGA, you'll notice only `blinky_dynamic` is toggling it's LED.
Looking at the [UART console from the FPGA][running on fpga], the following message will pop up.

```
hardware_access_control/part_2/led_walk_dynamic.cc:19 Assertion failure in start_walking
LED 0x7 couldn't be acquired
```

`led_walk_dynamic` was run after `blinky_dynamic` because it's thread was given a lower priority in the [`xmake.lua`][].
So when it asked for access to user LED 7, it was denied by `gpio_access` because this LED had already been allocated to `blinky_dynamic`.

Now change `NumLeds` in [`led_walk_dynamic.cc`][] from 8 to 7, then rebuild.
Both compartments should run happily again.
Not only will both compartments run happily, but `led_walk_dynamic` will output the following over the console.

[`led_walk_dynamic.cc`]: ../../exercises/hardware_access_control/part_2/led_walk_dynamic.cc

```
Led Walk Dynamic:           LED 3 Handle: 0x1024d0 (v:1 0x1024d0-0x1024e0 l:0x10 o:0xc p: G RWcgm- -- ---)
Led Walk Dynamic: Destroyed LED 3 Handle: 0x1024d0 (v:0 0x1024d0-0x1024e0 l:0x10 o:0xc p: G RWcgm- -- ---)
Led Walk Dynamic:       New LED 3 Handle: 0x102578 (v:1 0x102578-0x102588 l:0x10 o:0xc p: G RWcgm- -- ---)
```

These come from some superfluous lines in [`led_walk_dynamic.cc`][], which release ownership of user LED 3 only to later reacquire ownership.
You can comment out the line that reacquires the LED:

```cpp
	leds[3] = acquire_led(3).value();
```

When run `led_walk_dynamic` will now fail to toggle user LED 3 because it has relinquished ownership of the LED.

```
hardware_access_control/part_2/led_walk_dynamic.cc:34 Assertion failure in start_walking
Failed to toggle an LED
```

## Part 3

This is great and all, but how do we stop a compartment bypassing `gpio_access` and using `MMIO_CAPABILITY` directly?
In other words, how do we ensure that only `gpio_access` has access to the GPIOs?

Luckily the linker has all the information needed to check which compartments can access the GPIO MMIOs.
It outputs this information in a JSON report with the rest of the build artefacts.
To automate checking this report, we can use CHERIoT Audit which should already be in your path.

CHERIoT Audit allows you to query the JSON report and assert certain rules are followed.
You do this with a language called [Rego][], but don't worry you won't have to learn it for this exercise.
There are some pre-written rules in the [`gpio_access.rego`] module.
Let's first look at `only_gpio_access_has_access`.
It uses `mmio_allow_list` from the compartment package included in CHERIoT Audit to check that only the `gpio_access` compartment has access to the GPIO MMIOs.
If we run this on the part 2 firmware image's JSON report, it will return true.
However, when run against the part 1 firmware image's report it will return false, because the `blinky_raw` and `led_walk_raw` are not in the allow list.

[Rego]: https://www.openpolicyagent.org/docs/latest/policy-language/
[`gpio_access.rego`]: ../../exercises/hardware_access_control/part_3/gpio_access.rego

```sh
# This should return true
cheriot-audit \
    --board cheriot-rtos/sdk/boards/sonata-prerelease.json \
    --module exercises/hardware_access_control/part_3/gpio_access.rego \
    --query "data.gpio_access.only_gpio_access_has_access" \
    --firmware-report "build/cheriot/cheriot/release/hardware_access_part_2.json"
# This should return false
cheriot-audit \
    --board cheriot-rtos/sdk/boards/sonata-prerelease.json \
    --module exercises/hardware_access_control/part_3/gpio_access.rego \
    --query "data.gpio_access.only_gpio_access_has_access" \
    --firmware-report "build/cheriot/cheriot/release/hardware_access_part_1.json"
```

There's a second rule, `whitelisted_compartments_only`, which adds an additional condition that only `led_walk_dynamic` and `blinky_dynamic` can use `gpio_access`.
We can use this to restrict which compartments have access to the GPIO via `gpio_access`.

```sh
cheriot-audit \
    --board cheriot-rtos/sdk/boards/sonata-prerelease.json \
    --module exercises/hardware_access_control/part_3/gpio_access.rego \
    --query "data.gpio_access.whitelisted_compartments_only" \
    --firmware-report "build/cheriot/cheriot/release/hardware_access_part_2.json"
```

The above should return true as both compartments are in the allow list.
Try removing one of the compartments from the allow list given to `compartment_allow_list` in [`gpio_access.rego`][] and check the result of the above command is no longer true.

One can browse the other functions available as part of the compartment package in [CHERIoT Audit's readme][compartment package].

[compartment package]: https://github.com/CHERIoT-Platform/cheriot-audit/blob/main/README.md#the-compartment-package

## Part ∞

Where to go from here...
- There are input devices available through `SonataGpioBoard`.
    You could have a go at adding these to the `gpio_access` compartment.
- The interactions with `ledTaken` global in the `gpio_access` compartment aren't thread safe.
    You could take a look at `cheriot-rtos/examples/06.producer-consumer/` to learn how to use a futex to make it thread safe.
- There is a technical interest group for Sunburst and a technology access programme run by UKRI that lowRISC is helping to adjudicate.
    If you are interested in either of these please reach out to [info@lowrisc.org](mailto:info@lowrisc.org).
