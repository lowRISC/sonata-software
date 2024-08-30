<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Hardware Access Control Exercise

## Orientation

You are in the [`sonata-software`][] repository.
This repository wraps the [`lowrisc/cheriot-rtos`][], adding some Sonata specific demonstration code on top of the CHERIoT stack.
The [`lowrisc/cheriot-rtos`][], included in this repository as a submodule, is a fork of the upstream [`microsoft/cheriot-rtos`][].
[`microsoft/cheriot-rtos`][] contains the CHERIoT software stack; it is well documented by the [CHERIoT Programmer's Guide][].
*The [`lowrisc/cheriot-rtos`][] fork only exists to hold fresh patches that aren't quite ready to be upstreamed to [`microsoft/cheriot-rtos`][] but will be.*

Other repositories of note:
- [`sonata-system`][]: holds the Sonata system RTL and bootloader which come together to generate the bitstream.
- [`sonata-rp2040`][]: The firmware that is running on the Sonata's RP2040 microcontroller.
- [`CHERIoT-Platform/llvm-project`][]: The CHERIoT toolchain.
- [`cheriot-audit`][]: A tool to explore and verify the relationships between CHERIoT RTOS components.

For hardware documentation, see the [Sonata system book][].

[`sonata-software`]: https://github.com/lowRISC/sonata-software
[`lowrisc/cheriot-rtos`]: https://github.com/lowRISC/cheriot-rtos
[`microsoft/cheriot-rtos`]: https://github.com/microsoft/cheriot-rtos
[`sonata-system`]: https://github.com/lowRISC/sonata-system
[`sonata-rp2040`]: https://github.com/newaetech/sonata-rp2040
[`CHERIoT-Platform/llvm-project`]: https://github.com/CHERIoT-Platform/llvm-project
[`cheriot-audit`]: https://github.com/CHERIoT-Platform/cheriot-audit
[CHERIoT Programmer's Guide]: https://cheriot.org/book/
[Sonata system book]: https://lowrisc.org/sonata-system/


## The First Build and Load

I'm going to assume you've run through the whole of the [`README.md`](README.md).
In which case, you will have ran `xmake` and built the firmware images specified in [`xmake.lua`][], specifically by the *firmware* rules.
Each of the firmware targets depends on *compartments*, declared with `add_deps`, and has a number of *threads*.
For more information about these rules, see [`cheriot-rtos/README.md`](cheriot-rtos/README.md#building-firmware-images).

[`xmake.lua`]: ./xmake.lua

The only thing this repository adds is a little post-link function, `convert_to_uf2`, that converts the ELF of the firmware image into a UF2 file which can be easily loaded onto the Sonata board.
To do this, just copy the built UF2 to the `SONATA` drive which should have mounted when you plugged the board in.
After doing this, you should see some flashing LEDs!

```sh
xmake
cp build/cheriot/cheriot/release/sonata_simple_demo.uf2 /run/media/hugom/SONATA/
sync
```

*If this load doesn't work, have a look at the [Fallback Firmware Load](#fallback-firmware-load).*

To see the UART console logs, attach to `/dev/ttyUSB2` at a Baud of 115200 with your favourite terminal.

```sh
picocom /dev/ttyUSB2 -b 115200 --imap lfcrlf
```

You should see:

```
bootloader: Loading software from flash...
bootloader: Booting into program, hopefully.
Led Walk Raw: Look pretty LEDs!
```

If you'd like get debug messages from the RTOS Core compartments, you can enable them with the following configuration flags.

```sh
xmake config --board=sonata --debug-scheduler=y \
    --debug-locks=y --debug-cxxrt=y \
    --debug-token_library=y --debug-allocator=y
xmake build
```

You may need to run `rm -rf build/ .xmake/` to remove the debug messages from the configuration.



## Learning about the CHERIoT RTOS

The [CHERIoT Programmer's Guide] contains most of what a programmer would need to know to use the RTOS.
It's rendered from [`cheriot-rtos/docs/`](cheriot-rtos/docs).

The different boards supported can be found in [`cheriot-rtos/sdk/boards/`](cheriot-rtos/sdk/boards), of particular interest will be the Sonata's description in `sonata.json`.
More on board descriptions can be found in [`cheriot-rtos/docs/BoardDescriptions.md`](cheriot-rtos/docs/BoardDescriptions.md).
The structures the map onto the peripherals' memory and add functionality can be found in [`cheriot-rtos/sdk/include/platform/`](cheriot-rtos/sdk/include/platform/); the Sonata/Sunburst specific peripherals can be found in `sunburst/`.

The examples in [`cheriot-rtos/examples`](cheriot-rtos/examples) provide a nice tour of the different ways compartments can interact.
The following is shows how one can build an example to a UF2 file that can be loaded onto the board.

```sh
pushd cheriot-rtos/examples/05.sealing/
xmake -P .
llvm-objcopy -Obinary build/cheriot/cheriot/release/sealing /tmp/sealing.bin
uf2conv /tmp/sealing.bin -b0x00101000 -co /tmp/sealing.uf2
popd
```

To explore the various utility libraries available, look through [`cheriot-rtos/sdk/include/`](cheriot-rtos/sdk/include/).
When starting first starting to explore capabilities, the [`CHERI::Capability`](cheriot-rtos/sdk/include/cheri.hh) class is useful for pointer introspection.

## An Exercise Compartmentalisation

The `sonata_simple_demo` target in [`xmake.lua`][] has three threads,
from three different compartments: `led_walk_raw`, `echo`, and `lcd_test`.
The sources of these can be found in `compartments/`.

The `led_walk_raw` uses `MMIO_CAPABILITY` to get direct access the `gpio`'s MMIO region, but the `led_walk_raw` compartment now has access to all of the GPIO ports.
We may want to limit access to a select number of devices.

To do this, we can create a 'library' compartment with access to the MMIO region and which controls access to the ports. The `gpiolib` compartment is one I made earlier.
It can be found at `compartments/gpiolib.cc`, with the functions available to other compartments declared in `compartments/gpiolib.hh`.

`gpiolib` uses they sealing feature, outlined in the `cheriot-rtos/examples/05.sealing/` example we built earlier, to enable `gpiolib` to hand out opaque access to LEDs.
These can be used by library users to prove ownership of an LED.
The `led_walk` compartment is does the same thing as `led_walk`, but accesses the LEDs through `gpiolib`.
To see this in action, load the `sonata_led_demo` onto your board.


## Where to go from here...

[`led_walk_dynamic`]: exercises/hardware_access_control/led_walk_dynamic.cc

Have a play with the `sonata_led_demo`.
For example, remove the following line in the [`led_walk_dynamic`][] and you will see LED 3 no longer lights up.

```cpp
	leds[3] = aquire_led(3).value();
```

There are input devices available through `SonataGPIO`. You could have a go at adding these to `gpiolib`.

The interactions with `ledTaken` in [`gpiolib`](compartments/gpiolib.cc) aren't thread safe.
You could take a look at `cheriot-rtos/examples/06.producer-consumer/` to learn how to use a futex to make it thread safe.


## Appendix
### Fallback Firmware Load

We have been experiencing some reliability issues with copying firmware UF2 files to the SONATA virtual USB drive. You may find repeating the copy is sufficient to get things working but if you are having trouble we have a temporary backup load feature in the RP2040 firmware that uses the RP2040 USB boot/flash loader (for this to work you must have the `tmp_rpi_rp2_part1_v0.2.uf2` RP2040 firmware from the v0.2 Sonata release: https://github.com/lowRISC/sonata-system/releases/tag/v0.2)
You will need to create a new UF2 from your firmware UF2 with the script in [`scripts/rp2040_uf2/`](scripts/rp2040_uf2/)

```sh
# Script must be executed from the directory it lives in
cd scripts/rp2040_uf2
# Creates a file build/cheriot/cheriot/release/sonata_simple_demo_rp2040.uf2
./make_rp2040_uf2.sh ../../build/cheriot/cheriot/release/sonata_simple_demo.uf2
```

To use the new UF2 restart your board in RP2040 USB mode (hold the 'RP2040 boot' button whilst power cycling the board). Copy the new UF2 (build/cheriot/cheriot/release/sonata_simple_demo_rp2040.uf2 in the example above) to the RP2040 drive and this will load the Sonata firmware.
