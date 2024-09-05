# Running Sonata Software

You can either run software [on the sonata FPGA board](#running-on-the-sonata-fpga)
or [in the sonata simulator](#running-in-the-simulator).

## Running in the simulator

In the [getting started guide][], you entered the default environment with `nix develop`.
Because you now want to use the simulator, you need to enter the environment that includes the simulator:

```sh
nix develop .#env-with-sim
```

[getting started guide]: ../getting-started.md

This will pull the simulator into your path as `sonata-simulator`.
There's a convenience script, `scripts/run_sim.sh`, for calling the simulator.
You simply point the script to a built ELF file and it will run the firmware in the simulator.
*The ELF file is the build artefact with the same name as the firmware image and no extension.*
Note, the simulator will never terminate, so you will have to <kbd>Ctrl</kbd>+<kbd>C</kbd> to terminate the simulator.

```sh
./scripts/run_sim.sh build/cheriot/cheriot/release/sonata_simple_demo
```

UART output can be seen in the `uart0.log` file, which should appear in the directory the simulator was run from.
This can be observed using `tail -f` which will monitor the file and output as soon as something is written to the UART.
Note with the simulator running in the foreground this will need to be run in another terminal:

```sh
tail -f uart0.log
```

## Running on the Sonata FPGA

Before running the FPGA, you may need to put the latest RP2040 firmware and bitstream on your board, which you can do by following the instructions on the [sonata-system release page][].

[sonata-system release page]: https://github.com/lowRISC/sonata-system/releases

Any builds of software in this repository will also produce a UF2 file containing the built firmware.
When the Sonata FPGA is plugged into the computer, it should show up as a drive called 'SONATA'.
On my computer, it can be found at `/run/media/$USER/SONATA`.
You can copy the built UF2 file into this drive for the firmware to be loaded and run.

```sh
cp build/cheriot/cheriot/release/sonata_simple_demo.uf2 "/run/media/$USER/SONATA/"
sync # This `sync` command is rarely necessary.
```

*On Windows it's likely easier to use the file explorer to copy the UF2 to the `SONATA` drive.*
*Look for the Linux section below `This PC`.*

To see the UART console logs, attach to `/dev/ttyUSB2` at a Baud rate of 921,600 with your favourite terminal.

```sh
picocom /dev/ttyUSB2 -b 921600 --imap lfcrlf
```

*On Windows, we recommend to use [PuTTY](https://www.putty.org/) to connect to serial ports.*
*Select "Serial" as "Connection type", put the COM port in the "Serial line" text field, and set "Speed" to 921600.*
*To find out what serial ports are available, you can open Device Manager and all connected serial ports are listed under "Ports (COM & LPT)" section.*

When running the `sonata_simple_demo.uf2`, you should see the following console output as well as some flashing LEDs and LCD activity.
This UART output only gets printed once, so you may need to press the reset button (SW5) to see the output if you attach your console after programming.

```
bootloader: Loading software from flash...
bootloader: Booting into program, hopefully.
Led Walk Raw: Look pretty LEDs!
```
