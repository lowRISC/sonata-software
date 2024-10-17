<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Sonata Automotive Demo

## Description

The Sonata automotive demo is a demo designed around an automotive context,
which is run in two different environments - one on the Sonata board running
CHERIoT RTOS with CHERIoT enabled, and another running baremetal on Sonata
without CHERIoT enabled. Further documentation about the demo can be found
in [lib/README.md](./lib/README.md).

This demo assumes the following equipment setup:
  - Two Sonata boards, one running the `automotive_demo_send` firmware target,
  and one running the `automotive_demo_receive` firmware target.
    - The receiving board must be programmed with a bitstream supporting
    CHERIoT and running CHERIoT RTOS.
    - The sending board must be programmed with two bitstreams - one as above
    supporting CHERIoT and running CHERIoT RTOS, and one with CHERIoT disabled,
    running baremetal demo software. This means that the legacy demo must be
    loaded manually - instead, a better idea for running this demo is to build
    the built firmware into the bitstream so that the only step required is
    changing the bitstream and power cycling the board.
  - An ethernet cable connecting the two boards.
  - A pedal providing 3.3V IO analogue input, plugged into one of the Arduino
  Shield Analogue pins on the board with the sending firmware.
  - A model car driven by PWM, with the PWM output plugged into one of the

The demo does feature different applications which can run with varying
amounts of hardware availability, as detailed in the library documentation,
but the minimum requirement is at least the two boards and the connecting
Ethernet cable.

## Building

The cheriot componenents of this demo are built along with the rest of the examples.
However, the legacy component needs to be build seperately.

```sh
xmake -P examples
xmake -P examples/automotive/legacy/
```
