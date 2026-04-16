-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Automotive Demo Legacy Version")

includes("../../legacy_drivers")
set_toolchains("legacy-gcc")

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
    add_deps("legacy_drivers")

-- Automotive demo: Sending Firmware (1st board) (non-CHERIoT version)
legacy_firmware("automotive_demo_send_legacy")
    add_deps("lcd_st7735_lib_am", "automotive_lib", "legacy_drivers")
    add_files("send.c")
