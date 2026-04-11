-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Heartbleed Demo Legacy Version")
includes("../../legacy_drivers")

set_defaultarchs("cheriot")
set_defaultplat("cheriot")
includes("../../../common.lua")

set_toolchains("legacy-gcc")

-- Heartbleed demo: Sending Firmware (non-CHERIoT version)
legacy_firmware("heartbleed_demo_legacy")
    add_deps("lcd_st7735_lib_am", "legacy_drivers")
    add_includedirs(
        "../../../third_party/display_drivers/src/core/",
        "../../../third_party/display_drivers/src/st7735/",
        "../../../third_party/sonata-system/sw/legacy/common/"
    )
    add_ldflags("-specs=nosys.specs", {force = true})
    add_files(
        "../../../third_party/sonata-system/vendor/cheriot_debug_module/tb/prog/syscalls.c"
    )
    -- Keep the "-g3 -O0" flags as an example of a legacy flow that supports using a debugger
    add_files("heartbleed.c", "../common.c", {force = {cxflags = {"-g3", "-O0"}}})
