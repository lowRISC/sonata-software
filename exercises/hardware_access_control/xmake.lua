-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0


-- Part 1
compartment("blinky_raw")
    add_files("part_1/blinky_raw.cc")

compartment("led_walk_raw")
    add_files("part_1/led_walk_raw.cc")

firmware("hardware_access_part_1")
    add_deps("debug", "freestanding")
    add_deps("led_walk_raw", "blinky_raw")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "blinky_raw",
                priority = 2,
                entry_point = "start_blinking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "led_walk_raw",
                priority = 1,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
        }, {expand = false})
    end)
    after_link(convert_to_uf2)


-- Part 2
compartment("gpio_access")
    -- This compartment uses C++ thread-safe static initialisation and so
    -- depends on the C++ runtime.
    add_deps("cxxrt")
    add_files("part_2/gpio_access.cc")

compartment("blinky_dynamic")
    add_deps("gpio_access")
    add_files("part_2/blinky_dynamic.cc")

compartment("led_walk_dynamic")
    add_deps("gpio_access")
    add_files("part_2/led_walk_dynamic.cc")

firmware("hardware_access_part_2")
    add_deps("freestanding", "debug")
    add_deps("led_walk_dynamic", "blinky_dynamic")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "blinky_dynamic",
                priority = 2,
                entry_point = "start_blinking",
                stack_size = 0x400,
                trusted_stack_frames = 3
            },
            {
                compartment = "led_walk_dynamic",
                priority = 1,
                entry_point = "start_walking",
                stack_size = 0x400,
                trusted_stack_frames = 3
            },
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
