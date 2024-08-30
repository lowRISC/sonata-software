# CHES CHERIoT Workshop Preparation

In advance of the CHES Affiliated Sunburst Workshop, it's essential you prepare your environment so any issues can be sorted in advance of the day. If you have any issues in following this guide please contact the Sunburst Team on [ches-2024@lowrisc.org](mailto:ches-2024@lowrisc.org).

The Sonata software build environment can be setup under Windows, OSX and Linux.

We use a tool called [Nix](https://nixos.org/) to manage the build environment on all platforms. You will install it but don't need to know anything about it to follow these instructions.

Only Windows requires specific instructions, Nix handles everything you need on Linux and OSX. So if you're not using Windows jump straight to 'Installing Nix'.

## Windows Specific Setup

To obtain a Linux environment on Windows, you can choose to start a virtual machine or use Windows Subsystem for Linux (WSL).

Microsoft provides a detailed guide on how to install WSL: https://learn.microsoft.com/en-us/windows/wsl/install. For latest systems this would just be a single command:
```bat
wsl --install
```
You might need to enable virtualisation in the BIOS if it's not enabled by default.

If you are running the command without admin privileges, user account control (UAC) popups will appear a few times asking to allow changes to be made to the device.
Click "yes" to approve.

After the command's completion, it should say that Ubuntu is installed. Reboot your machine to make these changes effective.

After rebooting, Ubuntu should be available in your start menu.
Click it to start. For the first time, it would prompt you to select a Unix username and password.
Follow the Linux (Ubuntu) steps for the rest of this guide.

> ℹ️ If you have installed your WSL a long time ago, systemd may not have been enabled by default.
> It is recommended to use enable systemd.
> See https://learn.microsoft.com/en-us/windows/wsl/systemd.

## Installing Nix

The Nix package manager is used to create reproducible builds and consistent development environments.
We recommend installing Nix with the Determinate Systems' [`nix-installer`](https://github.com/DeterminateSystems/nix-installer):

```sh
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install
```

For more indepth instructions, follow the guide on [the zero to nix site](https://zero-to-nix.com/start/install).

*If you've downloaded nix through another method, make sure the experimental features ["flakes"](https://nixos.wiki/wiki/Flakes) and ["nix-command"](https://nixos.wiki/wiki/Nix_command) are enabled.*

*To use Nix from the terminal you'll need to open up a new terminal for it to be added to your path.*

### Setup Cache

To make use of the lowRISC cache so you don't have to rebuild binaries yourself, you'll need to make sure you're a trusted user.
To do this, you will need to add your user to the trusted users in `/etc/nix/nix.conf`, e.g. `trusted-users = root username`. *You can also add all users from a certain group instead of a single user by using an `@` symbol before the group name, e.g. `@sudo` or `@wheel`.*

> ℹ️ For Ubuntu users (including WSL users), this means adding this line to the `/etc/nix/nix.conf`:
> ```
> trusted-users = root @sudo
> ```
> 
> You'll need to restart the nix-daemon afterwards for the change to be picked up.
> ```sh
> sudo systemctl restart nix-daemon
> ```


> ℹ️ For OSX users, this means adding this line to the `/etc/nix/nix.conf`:
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

These applications are layered on top of your usual environment. You can see what was added with `echo $PATH`.

```
do you want to allow configuration setting 'extra-substituters' to be set to 'https://nix-cache.lowrisc.org/public/' (y/N)? y
do you want to permanently mark this value as trusted (y/N)? y
do you want to allow configuration setting 'extra-trusted-public-keys' to be set to 'nix-cache.lowrisc.org-public-1:O6JLD0yXzaJDPiQW1meVu32JIDViuaPtGDfjlOopU7o=' (y/N)? y
do you want to permanently mark this value as trusted (y/N)? y
warning: ignoring untrusted substituter 'https://nix-cache.lowrisc.org/public/', you are not a trusted user.
```

If you see the warning that substituter is ignored, cancel the process with Ctrl+C and check to see that [trusted-users is setup properly](#Setup-Cache). Nix can and will build everything from source if it can't find a cached version, so letting it continue will cause LLVM-Cheriot to be built from scratch on your machine.


## Your first build

Clone the sonata software repository, *making sure to recursively clone submodules as well*, then navigate into it.
```sh
git clone --recurse-submodule \
    https://github.com/lowRISC/sonata-software.git
cd sonata-software
```

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
Converted to uf2, output size: 147968, start address: 0x2000
Wrote 147968 bytes to build/cheriot/cheriot/release/sonata_simple_demo.uf2
[100%]: build ok, spent 6.827s
```

(Note output size may differ)

If you have got a successful build, congratulations! Your environment is ready to go for Sonata software development. Get in touch with Greg Chadwick on [ches-2024@lowrisc.org](mailto:ches-2024@lowrisc.org) if you have any issues.

For reference the full output (from a build run on a Linux machine) looks like:

```console
$ xmake build -P examples
checking for platform ... cheriot
checking for architecture ... cheriot
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
generating /home/user/tmp/sonata-software/cheriot-rtos/sdk/firmware.ldscript.in ... ok
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 24%]: cache compiling.release all/proximity_sensor_example.cc
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 24%]: cache compiling.release all/gpiolib.cc
[ 24%]: cache compiling.release snake/snake.cc
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 24%]: cache compiling.release all/led_walk.cc
[ 24%]: cache compiling.release ../cheriot-rtos/sdk/lib/cxxrt/guard.cc
[ 25%]: cache compiling.release ../cheriot-rtos/sdk/core/scheduler/main.cc
[ 26%]: cache compiling.release ../cheriot-rtos/sdk/lib/crt/cz.c
[ 27%]: cache compiling.release ../cheriot-rtos/sdk/lib/crt/arith64.c
[ 27%]: cache compiling.release ../cheriot-rtos/sdk/lib/atomic/atomic1.cc
[ 29%]: cache compiling.release all/echo.cc
[ 29%]: compiling.release ../cheriot-rtos/sdk/core/switcher/entry.S
[ 30%]: cache compiling.release ../third_party/display_drivers/core/lcd_base.c
[ 30%]: cache compiling.release ../third_party/display_drivers/core/m3x6_16pt.c
[ 31%]: cache compiling.release ../third_party/display_drivers/st7735/lcd_st7735.c
[ 32%]: cache compiling.release ../libraries/lcd.cc
[ 33%]: cache compiling.release all/lcd_test.cc
[ 33%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memcmp.c
[ 34%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memcpy.c
[ 35%]: cache compiling.release ../cheriot-rtos/sdk/lib/freestanding/memset.c
[ 36%]: cache compiling.release all/i2c_example.cc
[ 36%]: cache compiling.release ../cheriot-rtos/sdk/lib/debug/debug.cc
[ 38%]: cache compiling.release all/led_walk_raw.cc
[ 38%]: compiling.release ../cheriot-rtos/sdk/core/token_library/token_unseal.S
[ 39%]: cache compiling.release ../cheriot-rtos/sdk/lib/locks/locks.cc
[ 39%]: cache compiling.release ../cheriot-rtos/sdk/lib/locks/semaphore.cc
[ 40%]: cache compiling.release ../cheriot-rtos/sdk/lib/atomic/atomic4.cc
[ 41%]: cache compiling.release ../cheriot-rtos/sdk/core/allocator/main.cc
[ 42%]: cache compiling.release ../cheriot-rtos/sdk/lib/compartment_helpers/claim_fast.cc
[ 42%]: cache compiling.release ../cheriot-rtos/sdk/lib/compartment_helpers/check_pointer.cc
[ 43%]: compiling.release ../cheriot-rtos/sdk/core/loader/boot.S
[ 44%]: cache compiling.release ../cheriot-rtos/sdk/core/loader/boot.cc
[ 51%]: linking compartment led_walk.compartment
[ 51%]: linking library cxxrt.library
[ 51%]: linking library crt.library
[ 52%]: linking library lcd.library
[ 51%]: linking library atomic1.library
[ 52%]: linking privileged library cheriot.token_library.library
[ 52%]: linking library atomic4.library
[ 52%]: linking library debug.library
[ 52%]: linking compartment echo.compartment
[ 52%]: linking library freestanding.library
[ 59%]: linking library compartment_helpers.library
[ 69%]: linking compartment gpiolib.compartment
[ 69%]: linking compartment lcd_test.compartment
[ 69%]: linking compartment led_walk_raw.compartment
[ 69%]: linking compartment i2c_example.compartment
[ 69%]: linking compartment proximity_sensor_example.compartment
[ 69%]: linking compartment snake.compartment
[ 69%]: linking library locks.library
[ 80%]: linking privileged compartment sonata_demo_everything.scheduler.compartment
[ 80%]: linking privileged compartment cheriot.allocator.compartment
[ 80%]: linking privileged compartment sonata_led_demo.scheduler.compartment
[ 80%]: linking privileged compartment proximity_test.scheduler.compartment
[ 80%]: linking privileged compartment snake_demo.scheduler.compartment
[ 80%]: linking privileged compartment sonata_simple_demo.scheduler.compartment
[ 80%]: linking privileged compartment sonata_proximity_demo.scheduler.compartment
[ 90%]: linking firmware ../build/cheriot/cheriot/release/sonata_demo_everything
[ 90%]: linking firmware ../build/cheriot/cheriot/release/snake_demo
[ 90%]: linking firmware ../build/cheriot/cheriot/release/sonata_led_demo
[ 90%]: linking firmware ../build/cheriot/cheriot/release/sonata_proximity_demo
[ 90%]: linking firmware ../build/cheriot/cheriot/release/proximity_test
[ 90%]: linking firmware ../build/cheriot/cheriot/release/sonata_simple_demo
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_demo_everything.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_demo_everything.dump
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/snake_demo.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/snake_demo.dump
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_led_demo.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_led_demo.dump
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_proximity_demo.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_proximity_demo.dump
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/proximity_test.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/proximity_test.dump
[ 90%]: Creating firmware report ../build/cheriot/cheriot/release/sonata_simple_demo.json
[ 90%]: Creating firmware dump ../build/cheriot/cheriot/release/sonata_simple_demo.dump
Converted to uf2, output size: 112128, start address: 0x2000
Wrote 112128 bytes to ../build/cheriot/cheriot/release/snake_demo.uf2
Converted to uf2, output size: 92160, start address: 0x2000
Wrote 92160 bytes to ../build/cheriot/cheriot/release/sonata_led_demo.uf2
Converted to uf2, output size: 152576, start address: 0x2000
Wrote 152576 bytes to ../build/cheriot/cheriot/release/sonata_demo_everything.uf2
Converted to uf2, output size: 152576, start address: 0x2000
Wrote 152576 bytes to ../build/cheriot/cheriot/release/sonata_proximity_demo.uf2
Converted to uf2, output size: 89088, start address: 0x2000
Wrote 89088 bytes to ../build/cheriot/cheriot/release/proximity_test.uf2
Converted to uf2, output size: 147968, start address: 0x2000
Wrote 147968 bytes to ../build/cheriot/cheriot/release/sonata_simple_demo.uf2
[100%]: build ok, spent 11.816s
warning: ./cheriot-rtos/sdk/xmake.lua:116: unknown language value 'c2x', it may be 'c89'
warning: add -v for getting more warnings ..
```

### Debug logs

If you want debug logs from the RTOS, configure your build with the following additional options.

```sh
rm -rf build .xmake
xmake config -P examples
    --debug-scheduler=y --debug-locks=y \
    --debug-cxxrt=y --debug-loader=y \
    --debug-token_library=y --debug-allocator=y
xmake -P examples
```

Reconfiguring doesn't always work reliably, so often you will want to delete the `build` and `.xmake` directories when changing the configuration.
