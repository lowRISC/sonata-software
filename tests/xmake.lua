-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Tests")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

includes(path.join(sdkdir, "lib"))
includes("../common.lua")

option("board")
    set_default("sonata")

library("uart_tests")
    set_default(false)
    add_deps("debug")
    add_files("uart_tests.cc")

compartment("test_runner")
    add_deps("debug", "uart_tests")
    add_files("test_runner.cc")

firmware("sonata_test_suite")
    add_deps("freestanding", "test_runner")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "test_runner",
                priority = 20,
                entry_point = "run_tests",
                stack_size = 0x400,
                trusted_stack_frames = 3
            },
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
