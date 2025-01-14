# Exploring CHERIoT RTOS

## CHERIoT RTOS orientation
All the software in this repository runs on the CHERIoT RTOS, which is pulled in at the root of this repository as a submodule named [`cheriot-rtos`].
The [CHERIoT Programmer's Guide] contains most of what a programmer would need to know to use the RTOS.

[`cheriot-rtos`]: ../cheriot-rtos
[CHERIoT Programmer's Guide]: https://cheriot.org/book/

The different boards supported can be found in [`cheriot-rtos/sdk/boards/`](../cheriot-rtos/sdk/boards), of particular interest will be the Sonata's description in `sonata.json`.
More on board descriptions can be found in [`cheriot-rtos/docs/BoardDescriptions.md`](../cheriot-rtos/docs/BoardDescriptions.md).
The drivers (structures that map onto a peripherals' MMIO) add functionality can be found in [`cheriot-rtos/sdk/include/platform/`](../cheriot-rtos/sdk/include/platform/); the Sonata/Sunburst specific peripherals can be found in `sunburst/` within the aforementioned directory.

To explore the various utility libraries available, look through [`cheriot-rtos/sdk/include/`](../cheriot-rtos/sdk/include/).
When first starting to explore capabilities, the [`CHERI::Capability`](../cheriot-rtos/sdk/include/cheri.hh) class is useful for pointer introspection.

## Build system

The CHERIoT RTOS uses [xmake][] as it's build system.
The main rules you'll use are *compartment* and *library*, for creating compartments and libraries, as well as the *firmware* rule for creating a firmware image.
Documentation on these can be found in the CHERIoT RTOS' readme under '[building firmware images][]'.
For examples of using these rules, look at a root `xmake.lua` file, such as [`examples/xmake.lua`], and in the subdirectories it includes with the `includes` function.
*Note, we run an additional `convert_to_uf2` function on our firmware images to create UF2 files in this repository.*

[xmake]: https://xmake.io/
[building firmware images]: ../cheriot-rtos/README.md#building-firmware-images
[`examples/xmake.lua`]: ../examples/xmake.lua


## Building an upstream CHERIoT RTOS example

The examples in [`cheriot-rtos/examples`](../cheriot-rtos/examples) provide a nice tour of the different ways compartments can interact.
These can be built by pointing xmake to the example one wants to build, as shown below:

```sh
# Run from the root of the sonata-software repository
xmake -P cheriot-rtos/examples/05.sealing/
```

### Where's my UF2?

If you've followed the '[running software on the FPGA]' guide, you'll expect UF2 files as part of the build artefacts but these aren't automatically created in the [`cheriot-rtos`][] repository.
Thankfully, this repository includes a `./scripts/elf-to-uf2.sh` script that converts an ELF into a firmware a UF2 file.

[running software on the FPGA]: ./guide/running-software.md#running-on-the-sonata-fpga

```sh
xmake -P cheriot-rtos/examples/05.sealing/
scripts/elf-to-uf2.sh build/cheriot/cheriot/release/sealing
```
