# Getting started guide

This guide tells you how to get started with the Sonata board.
If you have any issues in following this guide please contact the Sunburst Team at [info@lowrisc.org](mailto:info@lowrisc.org).

The Sonata software build environment can be setup under Windows, macOS and Linux.
We use a tool called [Nix](https://nixos.org/) to manage the build environment on all platforms.
You need to install Nix but don't need to know anything else about it to follow these instructions.

You also need to setup the Sonata board itself with the latest release.
Read the [updating the sonata system guide](https://lowrisc.github.io/sonata-system/doc/guide/updating-system.html) for instructions on how to do this.
You only need to follow the first two steps listed there.

Only Windows requires specific instructions, Nix handles everything you need on Linux and macOS.
So if you're not using Windows jump straight to [Installing Nix](#installing-nix).

## Windows-specific setup

To obtain a Linux environment on Windows, you can choose to start a virtual machine or use Windows Subsystem for Linux (WSL).
Microsoft provides [a detailed guide on how to install WSL](https://learn.microsoft.com/en-us/windows/wsl/install).
For latest systems this would just be a single command:

```bat
wsl --install
```

> You might need to enable virtualisation in the BIOS if it's not enabled by default.

If you are running the command without admin privileges, user account control (UAC) popups will appear a few times asking to allow changes to be made to the device.
Click "yes" to approve.

After the command's completion, it should say that Ubuntu is installed.
Reboot your machine for the changes to take effect.
After rebooting, Ubuntu should be available in your start menu.
Click it to start.
For the first time, it prompts you to select a Unix username and password.
Follow the Linux (Ubuntu) steps for the rest of this guide.

> ℹ️ If you have installed your WSL a long time ago, `systemd` may not have been enabled by default.
> It is recommended to enable `systemd`.
> See https://learn.microsoft.com/en-us/windows/wsl/systemd.

## Installing Nix

The Nix package manager is used to create reproducible builds and consistent development environments.
For Linux systems it's recommended the following command from the official [documentation](https://nixos.org/download/):
```sh
sh <(curl --proto '=https' --tlsv1.2 -L https://nixos.org/nix/install) --daemon
```
For other systems such as macOS, Windows(WSL2), Docker and others, follow the official [documentation](https://nixos.org/download/).

To use Nix from the terminal you need to open up a new terminal for it to be added to your path.

Make sure the experimental features ["flakes"](https://nixos.wiki/wiki/Flakes) and ["nix-command"](https://nixos.wiki/wiki/Nix_command) are enabled
by adding the following to `/etc/nix/nix.conf` or `~/.config/nix/nix.conf`:
```console
experimental-features = nix-command flakes
```

### Setup Nix cache

To make use of the lowRISC Nix cache, so you don't have to rebuild binaries yourself, you need to mark yourself as a trusted user.
To do this, you add your username to the trusted users in `/etc/nix/nix.conf`, e.g. `trusted-users = root username`.
*You can also add all users from a certain group instead of a single user by using an `@` symbol before the group name, e.g. `@sudo` or `@wheel`.*

> ℹ️ For Ubuntu users (including WSL users), this means adding this line to the `/etc/nix/nix.conf`:
> ```
> trusted-users = root @sudo
> ```
>
> You need to restart the nix-daemon afterwards for the change to be picked up:
> ```sh
> sudo systemctl restart nix-daemon
> ```

> ℹ️ For macOS users, this means adding this line to the `/etc/nix/nix.conf`:
> ```
> trusted-users = root @admin
> ```
>
> You then need to restart your Mac for the changes to take effect.

## Enter the CHERIoT development environment

Running the following will put you in a shell environment with all the applications needed to build the CHERIoT RTOS.

```sh
# Enter the shell with
nix develop github:lowRISC/sonata-software
# Exit the shell with
exit
```

These applications are layered on top of your usual environment.
You can see what was added with `echo $PATH`.

```
do you want to allow configuration setting 'extra-substituters' to be set to 'https://nix-cache.lowrisc.org/public/' (y/N)? y
do you want to permanently mark this value as trusted (y/N)? y
do you want to allow configuration setting 'extra-trusted-public-keys' to be set to 'nix-cache.lowrisc.org-public-1:O6JLD0yXzaJDPiQW1meVu32JIDViuaPtGDfjlOopU7o=' (y/N)? y
do you want to permanently mark this value as trusted (y/N)? y
warning: ignoring untrusted substituter 'https://nix-cache.lowrisc.org/public/', you are not a trusted user.
```

If you see the warning that substituter is ignored, cancel the process with Ctrl+C and check to see that [trusted-users is setup properly](#setup-nix-cache).
Nix can and will build everything from source if it can't find a cached version, so letting it continue will cause LLVM-CHERIoT to be built from scratch on your machine.

## Your first build

Clone the Sonata software repository, *making sure to recursively clone submodules as well*, then navigate into it.

```sh
git clone --branch v1.1 \
    --recurse-submodule \
    https://github.com/lowRISC/sonata-software.git
cd sonata-software
```

Note a particular branch is specified, this must match your release, the release notes will tell you which branch you should use.

Enter the nix development development environment if you haven't already.
*Note that because we are in the repository we don't need to provide any arguments to `nix develop`.*

```sh
nix develop
```

Then build the examples with the following command.

```sh
xmake -P examples
```

After running this you should see the build run to completion and report success, the critical lines indicating a successful build are:

```sh
Converted to uf2, output size: 92672, start address: 0x20000000
Wrote 92672 bytes to ../build/cheriot/cheriot/release/proximity_test.slot3.uf2
Converted to uf2, output size: 126976, start address: 0x20000000
Wrote 126976 bytes to ../build/cheriot/cheriot/release/snake_demo.slot3.uf2
Converted to uf2, output size: 161792, start address: 0x20000000
Wrote 161792 bytes to ../build/cheriot/cheriot/release/sonata_simple_demo.slot3.uf2
[100%]: build ok, spent 0.986s
```

*Note output size and times may differ.*

If you have got a successful build, congratulations!
Your environment is ready to go for Sonata software development.
Get in touch with lowRISC on [info@lowrisc.org](mailto:info@lowrisc.org) if you have any issues.

For reference the full output (from a build run on a Linux machine) looks like:

```console
$ xmake build -P examples
checking for platform ... cheriot
checking for architecture ... cheriot
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/mvdmaas/repos/sw-sonata/cheriot-rtos/sdk/firmware.ldscript.in ... ok
[ 29%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 29%]: cache compiling.release automotive/lib/automotive_common.c
[ 30%]: cache compiling.release automotive/lib/no_pedal.c
[ 31%]: cache compiling.release automotive/lib/joystick_pedal.c
[ 31%]: cache compiling.release automotive/lib/digital_pedal.c
[ 32%]: cache compiling.release automotive/lib/analogue_pedal.c
[ 32%]: cache compiling.release all/i2c_example.cc
[ 32%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 32%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 32%]: cache compiling.release snake/snake.cc
[ 32%]: cache compiling.release all/rgbled_lerp.cc
[ 32%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 32%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 32%]: cache compiling.release automotive/lib/automotive_menu.c
[ 32%]: cache compiling.release automotive/cheri/send.cc
[ 33%]: cache compiling.release all/lcd_test.cc
[ 34%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 34%]: cache compiling.release all/led_walk_raw.cc
[ 35%]: cache compiling.release all/echo.cc
[ 36%]: cache compiling.release ../third_party/display_drivers/src/core/lcd_base.c
[ 36%]: cache compiling.release ../third_party/display_drivers/src/core/m3x6_16pt.c
[ 37%]: cache compiling.release ../third_party/display_drivers/src/core/lucida_console_10pt.c
[ 37%]: cache compiling.release ../third_party/display_drivers/src/core/lucida_console_12pt.c
[ 38%]: cache compiling.release ../third_party/display_drivers/src/st7735/lcd_st7735.c
[ 39%]: cache compiling.release ../libraries/lcd.cc
[ 39%]: cache compiling.release automotive/lib/automotive_common.c
[ 40%]: cache compiling.release automotive/cheri/receive.cc
[ 41%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
Not overriding global variable alignment for _ZL10memTaskTwo since it has a section assigned.Not overriding global variable alignment for _ZL18memAnalogueTaskTwo since it has a section assigned.
[ 42%]: cache compiling.release ../cheriot-rtos/sdk/core/loader/boot.cc
[ 43%]: cache compiling.release ../cheriot-rtos/sdk/core/allocator/main.cc
[ 43%]: compiling.release ../cheriot-rtos/sdk/core/loader/boot.S
[ 43%]: cache compiling.release ../cheriot-rtos/sdk/lib/atomic/atomic4.cc
[ 44%]: cache compiling.release ../cheriot-rtos/sdk/lib/locks/locks.cc
[ 44%]: cache compiling.release ../cheriot-rtos/sdk/lib/locks/semaphore.cc
[ 45%]: cache compiling.release ../cheriot-rtos/sdk/lib/crt/cz.c
[ 46%]: cache compiling.release ../cheriot-rtos/sdk/lib/crt/arith64.c
[ 46%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 47%]: cache compiling.release ../cheriot-rtos/sdk/lib/compartment_helpers/claim_fast.cc
[ 48%]: cache compiling.release ../cheriot-rtos/sdk/lib/compartment_helpers/check_pointer.cc
[ 48%]: cache compiling.release ../cheriot-rtos/sdk/lib/atomic/atomic1.cc
[ 49%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memcmp.c
[ 50%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memcpy.c
[ 50%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memset.c
[ 51%]: compiling.release ../cheriot-rtos/sdk/core/switcher/entry.S
[ 51%]: compiling.release ../cheriot-rtos/sdk/core/token_library/token_unseal.S
[ 52%]: cache compiling.release ../cheriot-rtos/sdk/lib/debug/debug.cc
[ 53%]: cache compiling.release all/proximity_sensor_example.cc
[ 53%]: linking compartment rgbled_lerp.compartment
[ 54%]: linking compartment echo.compartment
[ 55%]: linking library lcd.library
[ 55%]: linking library crt.library
[ 56%]: linking library atomic4.library
[ 56%]: linking library freestanding.library
[ 57%]: linking privileged library cheriot.token_library.library
[ 62%]: linking compartment lcd_test.compartment
[ 63%]: linking library locks.library
[ 67%]: linking library atomic1.library
[ 68%]: linking library compartment_helpers.library
[ 70%]: linking privileged compartment automotive_demo_receive.scheduler.compartment
[ 70%]: linking privileged compartment cheriot.allocator.compartment
[ 71%]: linking privileged compartment automotive_demo_send_cheriot.scheduler.compartment
[ 72%]: linking privileged compartment leds_and_lcd.scheduler.compartment
[ 72%]: linking privileged compartment proximity_test.scheduler.compartment
[ 73%]: linking privileged compartment snake_demo.scheduler.compartment
[ 74%]: linking privileged compartment sonata_demo_everything.scheduler.compartment
[ 74%]: linking privileged compartment sonata_proximity_demo.scheduler.compartment
[ 75%]: linking privileged compartment sonata_simple_demo.scheduler.compartment
[ 81%]: linking library debug.library
[ 82%]: linking compartment automotive_receive.compartment
[ 83%]: linking compartment automotive_send.compartment
[ 84%]: linking compartment led_walk_raw.compartment
[ 84%]: linking compartment proximity_sensor_example.compartment
[ 85%]: linking compartment snake.compartment
[ 86%]: linking compartment i2c_example.compartment
[ 89%]: linking firmware ../build/cheriot/cheriot/release/automotive_demo_receive
[ 89%]: Creating firmware report ../build/cheriot/cheriot/release/automotive_demo_receive.json
[ 89%]: Creating firmware dump ../build/cheriot/cheriot/release/automotive_demo_receive.dump
Converted to uf2, output size: 128000, start address: 0x0
Wrote 128000 bytes to ../build/cheriot/cheriot/release/automotive_demo_receive.slot1.uf2
Converted to uf2, output size: 128000, start address: 0x10000000
Wrote 128000 bytes to ../build/cheriot/cheriot/release/automotive_demo_receive.slot2.uf2
Converted to uf2, output size: 128000, start address: 0x20000000
Wrote 128000 bytes to ../build/cheriot/cheriot/release/automotive_demo_receive.slot3.uf2
[ 90%]: linking firmware ../build/cheriot/cheriot/release/automotive_demo_send_cheriot
[ 91%]: linking firmware ../build/cheriot/cheriot/release/leds_and_lcd
[ 91%]: linking firmware ../build/cheriot/cheriot/release/sonata_simple_demo
[ 92%]: linking firmware ../build/cheriot/cheriot/release/proximity_test
[ 93%]: linking firmware ../build/cheriot/cheriot/release/sonata_proximity_demo
[ 93%]: linking firmware ../build/cheriot/cheriot/release/snake_demo
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/automotive_demo_send_cheriot.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/automotive_demo_send_cheriot.dump
[ 91%]: Creating firmware report ../build/cheriot/cheriot/release/leds_and_lcd.json
[ 91%]: Creating firmware dump ../build/cheriot/cheriot/release/leds_and_lcd.dump
[ 91%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_simple_demo.json
[ 91%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_simple_demo.dump
[ 92%]: Creating firmware report ../build/cheriot/cheriot/release/proximity_test.json
[ 92%]: Creating firmware dump ../build/cheriot/cheriot/release/proximity_test.dump
[ 94%]: linking firmware ../build/cheriot/cheriot/release/sonata_demo_everything
[ 93%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_proximity_demo.json
[ 93%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_proximity_demo.dump
[ 93%]: Creating firmware report ../build/cheriot/cheriot/release/snake_demo.json
[ 93%]: Creating firmware dump ../build/cheriot/cheriot/release/snake_demo.dump
[ 94%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_demo_everything.json
[ 94%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_demo_everything.dump
Converted to uf2, output size: 146432, start address: 0x0
Wrote 146432 bytes to ../build/cheriot/cheriot/release/automotive_demo_send_cheriot.slot1.uf2
Converted to uf2, output size: 92672, start address: 0x0
Wrote 92672 bytes to ../build/cheriot/cheriot/release/proximity_test.slot1.uf2
Converted to uf2, output size: 162816, start address: 0x0
Wrote 162816 bytes to ../build/cheriot/cheriot/release/leds_and_lcd.slot1.uf2
Converted to uf2, output size: 146432, start address: 0x10000000
Wrote 146432 bytes to ../build/cheriot/cheriot/release/automotive_demo_send_cheriot.slot2.uf2
Converted to uf2, output size: 161792, start address: 0x0
Wrote 161792 bytes to ../build/cheriot/cheriot/release/sonata_simple_demo.slot1.uf2
Converted to uf2, output size: 175104, start address: 0x0
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_proximity_demo.slot1.uf2
Converted to uf2, output size: 92672, start address: 0x10000000
Wrote 92672 bytes to ../build/cheriot/cheriot/release/proximity_test.slot2.uf2
Converted to uf2, output size: 175104, start address: 0x0
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_demo_everything.slot1.uf2
Converted to uf2, output size: 126976, start address: 0x0
Converted to uf2, output size: 146432, start address: 0x20000000
Converted to uf2, output size: 162816, start address: 0x10000000
Wrote 126976 bytes to ../build/cheriot/cheriot/release/snake_demo.slot1.uf2
Wrote 146432 bytes to ../build/cheriot/cheriot/release/automotive_demo_send_cheriot.slot3.uf2
Wrote 162816 bytes to ../build/cheriot/cheriot/release/leds_and_lcd.slot2.uf2
Converted to uf2, output size: 161792, start address: 0x10000000
Wrote 161792 bytes to ../build/cheriot/cheriot/release/sonata_simple_demo.slot2.uf2
Converted to uf2, output size: 92672, start address: 0x20000000
Wrote 92672 bytes to ../build/cheriot/cheriot/release/proximity_test.slot3.uf2
Converted to uf2, output size: 175104, start address: 0x10000000
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_proximity_demo.slot2.uf2
Converted to uf2, output size: 175104, start address: 0x10000000
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_demo_everything.slot2.uf2
Converted to uf2, output size: 162816, start address: 0x20000000
Wrote 162816 bytes to ../build/cheriot/cheriot/release/leds_and_lcd.slot3.uf2
Converted to uf2, output size: 126976, start address: 0x10000000
Wrote 126976 bytes to ../build/cheriot/cheriot/release/snake_demo.slot2.uf2
Converted to uf2, output size: 161792, start address: 0x20000000
Wrote 161792 bytes to ../build/cheriot/cheriot/release/sonata_simple_demo.slot3.uf2
Converted to uf2, output size: 175104, start address: 0x20000000
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_proximity_demo.slot3.uf2
Converted to uf2, output size: 175104, start address: 0x20000000
Wrote 175104 bytes to ../build/cheriot/cheriot/release/sonata_demo_everything.slot3.uf2
Converted to uf2, output size: 126976, start address: 0x20000000
Wrote 126976 bytes to ../build/cheriot/cheriot/release/snake_demo.slot3.uf2
[100%]: build ok, spent 13.04s
warning: ./cheriot-rtos/sdk/xmake.lua:116: unknown language value 'c2x', it may be 'c23'
warning: add -v for getting more warnings ..
```

If you're following this guide as preparation for a workshop, you are now all set up and don't need to go any further.
With a successful software build you can now try [running software](./guide/running-software.md).
