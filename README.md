# Overview

This repository contains software, build flows and examples for the Sonata System (https://github.com/lowRISC/sonata-system).

## Installing Nix

The Nix package manager is used to create reproducible builds and consistent development environments.
We recommend installing Nix with the Determinate Systems' [`nix-installer`](https://github.com/DeterminateSystems/nix-installer):

```sh
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install
```

For more indepth instructions, follow the guide on [the zero to nix site](https://zero-to-nix.com/start/install).

*If you've downloaded nix through another method, make sure the experimental features ["flakes"](https://nixos.wiki/wiki/Flakes) and ["nix-command"](https://nixos.wiki/wiki/Nix_command) are enabled.*

*To use Nix from the terminal you'll need to open up a new terminal for it to be added to your path.*

*To obtain a Linux environment on Windows, you can choose to start a virtual machine or use Windows Subsystem for Linux (WSL). Microsoft provides [a detailed guide on how to install WSL](https://learn.microsoft.com/en-us/windows/wsl/install).*

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

For Linux users, there is also an environment with the sonata simulator included:
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

## License

Unless otherwise noted, everything in the repository is covered by the [Apache License](https://www.apache.org/licenses/LICENSE-2.0.html), Version 2.0. See the [LICENSE](https://github.com/lowRISC/sonata-software/blob/main/LICENSE) file for more information on licensing.
