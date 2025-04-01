# Using Sonata IO

## Overview

The v1.0 Sonata System contains the following peripheral blocks for general purpose use:

 - 5x SPI (2 dedicated and 3 muxable)
 - 3x UART (1 dedicated and 2 muxable)
 - 2x I2C (both muxable)
 - GPIO (LEDs and Switches on the board plus GPIO for all headers other than
   mikroBUS)
 - PWM (6 muxable channels, 1 dedicated channel)
 - ADC (6 dedicated channels)
 - 1x USB Device (dedicated)

Sonata contains a [pinmux][] which allows you to chose which block is connected to a particular physical IO pin.
Each pin has its own unique selection of blocks it can be attached to.
The 'muxable' in the list refers to the blocks that can connect to multiple pins.
Some of the blocks are dedicated, they connect to specific IO, e.g. there is a dedicated SPI block for the Ethernet controller.

## Pinmux Usage

The pinmux has two kinds of selectors:

- Output selectors - One per pin, chooses which block I/O outputs to that pin
- Input selectors - One per block IO, chooses which pin inputs to that block IO

A [pinmux driver][] is available to allow you to manipulate these selectors.
Note there are defined constants for the selectors themselves but not the options within the selectors.
E.g. PMOD0 pin 2 has output selector constant `OutputPin::pmod0_2` but a raw number is used to choose which block I/O is connected to.
The [pinmux documentation][] provides the possible inputs and outputs for each selector.

For example say you wished to connect the PMOD0 SPI to SPI block 1; there are 4 selectors to set.

- PMOD 0 CS (pin 1), COPI (pin 2) and SCK (pin 4) output selectors
- SPI Block 1 CIPO input selector

The code below shows how to use the pinmux driver to do this:

```c++
#include <platform-pinmux.hh>

...

auto pinSinks = SonataPinmux::PinSinks();
auto blockSinks = SonataPinmux::BlockSinks();

pinSinks->get(SonataPinmux::PinSink::pmod0_1).select(2); // cs_0
blockSinks->get(SonataPinmux::BlockInput::spi_1_cipo).select(3); // cipo
pinSinks->get(SonataPinmux::OutputPin::pmod0_2).select(2); // coi
pinSinks->get(SonataPinmux::OutputPin::pmod0_4).select(2); // sclk
pinSinks->get(SonataPinmux::OutputPin::pmod0_9).select(2); // cs_1
pinSinks->get(SonataPinmux::OutputPin::pmod0_10).select(2); // cs_2
```

Following this you can then use the `spi_1` SPI instance to communicate with whatever SPI device is plugged into PMOD0.

## Driver Usage

The various drivers work with a capability that points to the relevant device address range in the memory map.
CHERIoT RTOS provides the [MMIO_CAPABILITY][] macro to easily get a driver instance for a particular device.
The compiler, build system and runtime handle setting up the required capability and providing it to the compartment.

An example usage of the I2C driver can be see in [sonata-software/examples/all/i2c_example.cc][].
The relevant line that creates the driver instance is:

```c++
auto i2c0 = MMIO_CAPABILITY(OpenTitanI2c, i2c0);
```

Similar code can be used to instantiate a driver for all of the peripheral instances and types.

## Pinouts

To determine the mapping between the pin names in the pinmux documentation and the physical headers on the Sonata PCB there are a few pinout diagrams you can use.

- [Arduino Shield][] - pin names have the form `ah_tmpioN` where `N` is the Arduino digital pin number. This is the number in light pink on the linked diagram and labeled as `DN` on the Sonata PCB silkscreen. This matches the numbering seen on Arduino boards on the digital side of the header. The analog pins (A0 - A5) are fixed and connect directly to the ADC.
- [Raspberry PI HAT][] - pin names have the form `rph_gN` where `N` is the GPIO number provided on the pinout diagram. Note this is different to the *physical* pin number, which relates to the physical pin position and is the number written on the Sonata PCB. The physical pin number is the one immediately next to the header in the pinout diagram. For instance `rph_g0` is physical pin 27.
- [PMOD][] - pin names have the form `pmodX_Y` where `X` is the PMOD header (0 on the left, 1 on the right) and `Y` is the pin in the header. The `Y` pin number is the physical pin number (corresponding to the 'Pin' column of the various interface type tables in the linked specification). Note there are no `pmodX_5` and `pmodX_6` pins seen in the pinmux documentation as these are ground and power pins.
- PMOD C - These are the 6 pins in the middle of the large PMOD header, they have the form `pmodc_N` where `N` corresponds to the physical pin number. 1, 2 and 3 are the top pins. `pmodc_1` is the top right pin.
- [mikroBUS][] - pin names have the form `mbN` where `N` is the physical pin number. The mikroBUS specification does not specify specific pin numbers. For Sonata `mb1` is mikroBUS pin `CS`, `mb4` is mikroBUS pin `MOSI`, `mb5` is mikroBUS pin `SDA` and `mb10` is mikroBUS pin `PWM`. The others follow from those. mikroBUS does not define any GPIO pins so there is no GPIO block for this header.

## Peripheral Device Details

The device names used here are those given the CHERIoT RTOS Sonata board description file found in [cheriot-rtos/sdk/boards/sonata-1.1.json][].
For each peripheral type details of the instances available and what pins they can mux to are given below along with a link to the driver file used for that peripheral.

### SPI

Driver: [cheriot-rtos/sdk/include/platform/platform-spi.hh][]

2 of the SPI blocks are fixed.

- `spi_lcd` - Connects to the LCD
- `spi_ethmac` - Connects to the ethernet controller

3 of the SPI instances are muxable.

- `spi0` - SPI flash and SD card
- `spi1` - RPI HAT SPI0, Arduino SPI, PMOD 0 SPI
- `spi2` - RPI HAT SPI1, PMOD 1 SPI, mikroBUS SPI

#### SPI chip-select (CS)

Each SPI block has 4 chip-select lines which are controlled via the `cs` register.
This can be accessed directly by read/writing to the `cs` member of the `SonataSPI` structure.
To see which `cs` bit corresponds to which physical pin consult the [pinmux documentation][].
For example the raspberry PI HAT GPIO 7 pin (`rph_g7`) can be controlled by bit 1 of the SPI block 1 CS register when that is chosen as the output in the `rph_g7` output selector in the pinmux.

### UART

Driver: [cheriot-rtos/sdk/include/platform/platform-uart.hh][]

1 UART is fixed:

- `uart` - The system UART available on the TX/RX0 UART header (P12).
  Connects to FTDI USB UART when jumpers are connected.
  Used as debug log, stdout and sterr in CHERIoT RTOS.

The other 2 UARTs are muxable:

- `uart1` - TX/RX1 UART header (P12), RPI Hat UART, Arduino UART, mikroBUS UART, PMOD 0 UART
- `uart2` - TX/RX1 UART header (P12), PMOD1 UART, RS232, RS485

### I2C

Driver: [cheriot-rtos/sdk/include/platform/platform-i2c.hh][]

Both I2C instances are muxable, however they act differently to the other muxable busses.
As I2C is a low speed shared bus multiple headers can be muxed onto an I2C instance.
So for each i2c blocks it's possible to have all the physical pins driven by it at once.

- `i2c0` - qwiic0 & Arduino I2C, RPI Hat EEPROM I2C, PMOD0 I2C
- `i2c1` - qwiic1, RPI Hat I2C, mikroBUS I2C, PMOD1 I2C

Note that the qwiic0 & Arduino I2C pins are physically wired together on the PCB.

### GPIO

Driver: [cheriot-rtos/sdk/include/platform/platform-gpio.hh][]

All of the GPIO blocks are fixed, in that each are dedicated to a specific set of pins.
For many of those pins there's multiple things they can be muxed to (e.g. on the Raspberry Pi HAT header you can choose between using the SPI to drive the SPI pins or switch them to GPIO).

The available GPIO blocks are:

- `gpio_board` - LEDs and switches on the Sonata PCB
- `gpio_rpi` - Raspberry PI HAT
- `gpio_arduino` - Arduino Shield
- `gpio_pmod0` - PMOD 0
- `gpio_pmod1` - PMOD 1
- `gpio_pmodc` - PMOD C (the middle 6 pins between PMOD 0 and PMOD 1)

### PWM

Driver: [cheriot-rtos/sdk/include/platform/platform-pwm.hh][]

6 PWM channels are muxable, accessible through one RTOS device:

- `pwm` - Muxable between RPI Hat, Arduino, PMOD and mikroBUS

One channel is fixed, accessible through a separate RTOS device:

- `pwm_lcd` - Connected to LCD backlight control

### ADC

Driver: [cheriot-rtos/sdk/include/platform/platform-adc.hh][]

The ADC uses the FPGA's internal XADC.
The XADC is 12-bit with a 1 MSPS sampling rate.
It is directly connected to the 6 Arduino analog pins.
It can tolerate up to 5v input but the measurable range is 0 - 3v.

### USB Device

Driver: [cheriot-rtos/sdk/include/platform/platform-usbdev.hh][]

There is a single fixed USB device:

- `usbdev` - Connected directly to the User USB on the Sonata PCB.

Currently there is no USB stack or full USB examples available.
However there is some test software used for Sonata testing in the sonata-system repository.
[sonata-system/blob/main/sw/cheri/checks/usbdev_check.cc][] will connect to a host as a virtual UART sending some text and echoing back received text.
[sonata-system/blob/main/sw/cheri/common/usbdev-utils.hh][] (used by usbdev_check) provides some basic USB functionality on top of the driver.

[pinmux]: https://lowrisc.github.io/sonata-system/doc/ip/pinmux/index.html
[pinmux documentation]: https://lowrisc.github.io/sonata-system/doc/ip/pinmux/index.html
[pinmux driver]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-pinmux.hh
[MMIO_CAPABILITY]: https://cheriot.org/book/language_extensions.html#_importing_mmio_access
[sonata-software/examples/all/i2c_example.cc]: https://github.com/lowRISC/sonata-software/blob/a88d6b78405743956cc2aa0a5c272e87dc44b11d/examples/all/i2c_example.cc
[cheriot-rtos/sdk/boards/sonata-1.1.json]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/boards/sonata-1.1.json
[Arduino Shield]: https://commons.wikimedia.org/wiki/File:Pinout_of_ARDUINO_Board_and_ATMega328PU.svg
[Raspberry PI HAT]: https://pinout.xyz/
[PMOD]: https://digilent.com/reference/_media/reference/pmod/pmod-interface-specification-1_3_1.pdf
[mikroBUS]: https://download.mikroe.com/documents/standards/mikrobus/mikrobus-standard-specification-v200.pdf
[cheriot-rtos/sdk/include/platform/platform-spi.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-spi.hh
[cheriot-rtos/sdk/include/platform/platform-uart.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-uart.hh
[cheriot-rtos/sdk/include/platform/platform-i2c.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-i2c.hh
[cheriot-rtos/sdk/include/platform/platform-gpio.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-gpio.hh
[cheriot-rtos/sdk/include/platform/platform-pwm.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-pwm.hh
[cheriot-rtos/sdk/include/platform/platform-adc.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-adc.hh
[cheriot-rtos/sdk/include/platform/platform-usbdev.hh]: https://github.com/lowRISC/cheriot-rtos/tree/a4b6ab0a80df3bca696cd9c598b93fa2deebef4d/sdk/include/platform/sunburst/platform-usbdev.hh
[sonata-system/blob/main/sw/cheri/checks/usbdev_check.cc]: https://github.com/lowRISC/sonata-system/tree/0ec0807fb740d8f14dd8a7cc2d8097bc56cbd1e0/sw/cheri/checks/usbdev_check.cc
[sonata-system/blob/main/sw/cheri/common/usbdev-utils.hh]: https://github.com/lowRISC/sonata-system/tree/0ec0807fb740d8f14dd8a7cc2d8097bc56cbd1e0/sw/cheri/common/usbdev-utils.hh
