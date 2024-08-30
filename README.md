# Sonata Software

This repository contains software, build flows and examples for the [Sonata System][].

[sonata system]: https://github.com/lowRISC/sonata-system

For a guide on how to get up and running on building software for the sonata board see the [getting started guide][].
After you are all set up, take a look at the [hardware access control exercise][].

[getting started guide]: ./doc/getting-started.md
[hardware access control exercise]: ./exercises/hardware_access_control/README.md

## Orientation

You are in the [`sonata-software`][] repository.
This repository wraps the [`lowrisc/cheriot-rtos`][], adding some Sonata specific demonstration code on top of the CHERIoT stack.
The [`lowrisc/cheriot-rtos`][], included in this repository as a submodule, is a fork of the upstream [`CHERIoT-Platform/cheriot-rtos`][].
[`CHERIoT-Platform/cheriot-rtos`][] contains the CHERIoT software stack; it is well documented by the [CHERIoT Programmer's Guide][].
*The [`lowrisc/cheriot-rtos`][] fork only exists to hold fresh patches that aren't quite ready to be upstreamed to [`CHERIoT-Platform/cheriot-rtos`][] but will be.*

Other repositories of note:
- [`sonata-system`][]: holds the Sonata system RTL and bootloader which come together to generate the bitstream.
- [`sonata-rp2040`][]: The firmware that is running on the Sonata's RP2040 microcontroller.
- [`CHERIoT-Platform/llvm-project`][]: The CHERIoT toolchain.
- [`cheriot-audit`][]: A tool to explore and verify the relationships between CHERIoT RTOS components.
- [`CHERIoT-Platform/book`][]: The source of the [CHERIoT Programmer's Guide][].

For hardware documentation, see the [Sonata system book][].

[`sonata-software`]: https://github.com/lowRISC/sonata-software
[`lowrisc/cheriot-rtos`]: https://github.com/lowRISC/cheriot-rtos
[`CHERIoT-Platform/cheriot-rtos`]: https://github.com/CHERIoT-Platform/cheriot-rtos
[`sonata-system`]: https://github.com/lowRISC/sonata-system
[`sonata-rp2040`]: https://github.com/newaetech/sonata-rp2040
[`CHERIoT-Platform/llvm-project`]: https://github.com/CHERIoT-Platform/llvm-project
[`cheriot-audit`]: https://github.com/CHERIoT-Platform/cheriot-audit
[`CHERIoT-Platform/book`]: https://github.com/CHERIoT-Platform/book
[CHERIoT Programmer's Guide]: https://cheriot.org/book/
[Sonata system book]: https://lowrisc.org/sonata-system/


## License

Unless otherwise noted, everything in the repository is covered by the [Apache License](https://www.apache.org/licenses/LICENSE-2.0.html), Version 2.0. See the [LICENSE](https://github.com/lowRISC/sonata-software/blob/main/LICENSE) file for more information on licensing.
