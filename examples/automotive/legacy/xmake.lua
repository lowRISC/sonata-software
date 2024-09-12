-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Automotive Demo Legacy Version")

-- The directory containing this script
local scriptdir = os.scriptdir()
-- The root of the `sonata-software` repository
local rootdir = path.join(scriptdir, '../../..')
-- The directory containing the legacy baremetal build files (link script, runtime, drivers)
local legacy_dir = path.join(rootdir, "third_party/sonata-system/sw/legacy")

-- This unnecessary boilerplate is used so that you can run `xmake -P examples/automotive/legacy`
-- and then run `xmake -P examples` without xmake complaining. If this is not included, and no
-- options are provided, then xmake will by default determine your local architecture/platform,
-- and save this in the xmake config for future runs - which is wrong for the cross-compilation
-- used in `examples/xmake.lua`, even though these are labelled as different projects. This
-- ensures the xmake config is not altered without users knowing.
set_defaultarchs("cheriot")
set_defaultplat("cheriot")

-- Setup the GCC toolchain for building non-CHERI baremetal programs
toolchain("legacy-gcc")
    set_kind("standalone")
    set_toolset("cc", "riscv32-unknown-elf-gcc")
    set_toolset("cxx", "riscv32-unknown-elf-g++")
    set_toolset("objcopy", "riscv32-unknown-elf-objcopy")
    set_toolset("ld", "riscv32-unknown-elf-gcc")
    set_toolset("as", "riscv32-unknown-elf-gcc")
    set_toolset("ar", "riscv32-unknown-elf-ar")

    -- Set up flags needed in the toolchain
    on_load(function (toolchain)
        local linkscript = path.join(legacy_dir, "link.ld")
        -- Flags used for C/C++, assembly and the linker
        local default_flags = {
            "-march=rv32imc",
            "-mabi=ilp32",
            "-mcmodel=medany",
            "-Wall",
            "-fvisibility=hidden"
        }
        -- C/C++ flags
        toolchain:add("cxflags", default_flags, {force = true})
        toolchain:add("cflags", default_flags)
        -- Assembly flags
        toolchain:add("asflags", default_flags)
        -- Linker flags
        toolchain:add("ldflags", default_flags)
        toolchain:add("ldflags", "-ffreestanding -g -nostartfiles -T " .. linkscript)
    end)
toolchain_end()

-- Helper function to define legacy firmware (running without CHERI or CHERIoT-RTOS, baremetal)
-- Useful for comparing between CHERI and non-CHERI for e.g. demos/exercises
function legacy_firmware(name)

    -- Create the firmware target; remains open on return, so the caller can 
    -- add more rules to it as they like.
    target(name)
        set_kind("binary")
        set_arch("rv32imc")
        set_plat("ilp32")
        set_languages("gnu99", "gnuxx11")
        -- Add the basic C runtime to the baremetal firmware
        add_files(
            path.join(legacy_dir, "common/crt0.S"), 
            path.join(legacy_dir, "common/gpio.c"), 
            path.join(legacy_dir, "common/i2c.c"), 
            path.join(legacy_dir, "common/pwm.c"), 
            path.join(legacy_dir, "common/rv_plic.c"), 
            path.join(legacy_dir, "common/sonata_system.c"), 
            path.join(legacy_dir, "common/spi.c"), 
            path.join(legacy_dir, "common/timer.c"), 
            path.join(legacy_dir, "common/uart.c")
        )
end

set_toolchains("legacy-gcc")

-- Legacy-specific LCD library for using the display drivers
target("lcd_st7735_lib_am")
    set_kind("static")
    local lcd_dir = path.join(rootdir, "third_party/display_drivers")
    add_files(
        path.join(lcd_dir, "core/lcd_base.c"),
        path.join(lcd_dir, "core/lucida_console_10pt.c"),
        path.join(lcd_dir, "core/lucida_console_12pt.c"),
        path.join(lcd_dir, "core/m3x6_16pt.c"),
        path.join(lcd_dir, "st7735/lcd_st7735.c")
    )

-- The automotive library shared between the legacy & CHERIoT firmware
target("automotive_lib")
    set_kind("static")
    add_files(
        "../lib/automotive_common.c",
        "../lib/automotive_menu.c",
        "../lib/no_pedal.c",
        "../lib/joystick_pedal.c",
        "../lib/digital_pedal.c",
        "../lib/analogue_pedal.c"
    )

-- Automotive demo: Sending Firmware (1st board) (non-CHERIoT version)
legacy_firmware("automotive_demo_send_legacy")
    add_deps("lcd_st7735_lib_am", "automotive_lib")
    add_files("send.c", "ksz8851.c", "lcd.c", "adc.c")
