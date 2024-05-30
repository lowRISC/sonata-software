# Overview

This repository contains software, build flows and examples for the Sonata System (https://github.com/lowRISC/sonata-system).

If you want to use the web-based environment start at [Use a web-based environment](#use-a-web-based-environment). For local setups only Windows requires specific instructions, Nix handles everything you need on Linux and OSX. So if you're not using Windows jump straight to [Installing Nix](#installing-nix).

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
Nix is required to follow the instructions below.
We recommend installing Nix with the Determinate Systems' [`nix-installer`](https://github.com/DeterminateSystems/nix-installer):

```sh
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install
```

For more indepth instructions, follow the guide on [the zero to nix site](https://zero-to-nix.com/start/install).

*If you've downloaded nix through another method, make sure the experimental features ["flakes"](https://nixos.wiki/wiki/Flakes) and ["nix-command"](https://nixos.wiki/wiki/Nix_command) are enabled.*

*To use Nix from the terminal you'll need to open up a new terminal for it to be added to your path.*

*To obtain a Linux environment on Windows, you can choose to start a virtual machine or use Windows Subsystem for Linux (WSL). Microsoft provides [a detailed guide on how to install WSL](https://learn.microsoft.com/en-us/windows/wsl/install).*

### Setup Cache

Nix will build everything from source, which can take quite some time unless you have access to cached binaries.
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

There is also an environment with the sonata simulator included:
```
nix develop .#env-with-sim
sonata-simulator --help
```

If you see the warning that substituter is ignored, cancel the process with Ctrl+C and check to see that [trusted-users is setup properly](#Linux). Nix can and will build everything from source if it can't find a cached version, so letting it continue will cause LLVM-Cheriot to be built from scratch on your machine.

## Use a web-based environment

A [Github Codespaces](https://github.com/features/codespaces) web-based development environment is available.
As codespaces provides a limited amount of free compute and storage (60 hours per month) a local environment is likely preferred for serious development however the codespace offers an excellent way to test out Sonata development with minimal friction.

To use it you will need a (free) github account, you can sign-up at [https://github.com/signup](https://github.com/signup) if you need one.

To create the codespace go to the sonata-software repository https://github.com/lowrisc/sonata-software, click on '<> Code' and choose the 'Codespaces' tab. You should see a green 'Create codespace on main' button. Click this and a codespace will be setup.

It'll take a few minutes to create, and you will eventually see a vscode UI. In the bottom panel (which has the terminal) you'll see a + icon in the top right of the panel. Click this and you should see a new terminal open that gives the prompt 'Sonata /workspaces/sonata-software>'. This is the same shell environment you get with the local setup described above.

You can now follow the first build instructions below but you don't need to checkout the repository (the github codespace has done that for you).

You have a limited allowance for running codespaces. To avoid running out you can delete or stop your codespace from [https://github.com/codespaces](https://github.com/codespaces). Click the three horizontal dots next to the codespace name and choose either 'Delete' or 'Stop Codespace'.

## Your first build

Checkout this repository (not required under github codespaces) and cd into it:
```sh
git clone --recurse-submodule \
    https://github.com/lowRISC/sonata-software.git
cd sonata-software
```

Configure and run build:

```sh
xmake
```

After running this you should see the build run to completion and report success, the critical lines indicating a successful build are:

```sh
Converted to uf2, output size: 74752, start address: 0x101000
Wrote 74752 bytes to build/cheriot/cheriot/release/sonata_simple_demo.uf2
[100%]: build ok, spent 6.827s
```

*Note, the output size may differ.*

## Next steps

The [lab from the recent hackathon](hackathon.md) gives a good introduction to some CHERIoT RTOS features and how to develop CHERIoT RTOS applications for Sonata.

## Running with the simulator

When using the `env-with-sim` environment (see above) a Sonata simulator binary is available.
The `scripts/run_sim.sh` script can be used to run it against a binary, e.g. to run the binary built above in the simulator:

```sh
./scripts/run_sim.sh build/cheriot/cheriot/release/sonata_simple_demo
```

UART output can be seen in the uart0.log file.
This can be observed using `tail -f` which will monitor the file and output as soon as something is written to the UART.
Note with the simulator running in the foreground this will need to be run in another terminal

```sh
tail -f uart0.log
```

## License

Unless otherwise noted, everything in the repository is covered by the [Apache License](https://www.apache.org/licenses/LICENSE-2.0.html), Version 2.0. See the [LICENSE](https://github.com/lowRISC/sonata-software/blob/main/LICENSE) file for more information on licensing.
