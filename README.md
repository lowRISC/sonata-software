# Sonata software

This repository contains software, build flows and examples for the [Sonata System][] running on the [Sonata PCB][].
For a guide on how to get up and running on building software for the sonata board see the [getting started guide][].
After you are all set up, take a look at the [exercises][].

[Sonata System]: https://github.com/lowRISC/sonata-system
[Sonata PCB]: https://github.com/newaetech/sonata-pcb
[getting started guide]: ./doc/getting-started.md
[exercises]: ./exercises/README.md

## Orientation

You are in the [`sonata-software`][] repository.
This repository wraps the [`cheriot-rtos`][], which is included in this repository as a submodule.
[`cheriot-rtos`][] contains the CHERIoT software stack as well as drivers for using the Sonata board; it is well documented by the [CHERIoT Programmer's Guide][].

You do not need this to do the exercises but if you would like more detailed documentation about the hardware, see the [Sonata System book][].

[`sonata-software`]: https://github.com/lowRISC/sonata-software
[`cheriot-rtos`]: https://github.com/CHERIoT-Platform/cheriot-rtos
[CHERIoT Programmer's Guide]: https://cheriot.org/book/
[Sonata System book]: https://lowrisc.org/sonata-system/

## License

Unless otherwise noted, everything in the repository is covered by the [Apache License](https://www.apache.org/licenses/LICENSE-2.0.html), Version 2.0.
See the [LICENSE](https://github.com/lowRISC/sonata-software/blob/main/LICENSE) file for more information on licensing.
