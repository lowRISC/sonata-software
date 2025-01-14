<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Sonata software exercises

## Building the exercises

Assuming you've run through the [getting started guide][], you will have ran `xmake -P examples` and built the example firmware images.
To build all the exercises, simply substitute `examples` for `exercises`.

[getting started guide]: ../doc/getting-started.md

```sh
xmake -P exercises
```

## Exercises

Currently, there are two exercises:
 - [Hardware Access Control](./hardware_access_control/)
 - [Firmware Auditing](./firmware_auditing/)
