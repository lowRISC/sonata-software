-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Examples")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

includes(path.join(sdkdir, "lib"))
includes("../libraries")
includes("../common.lua")

option("board")
    set_default("sonata-prerelease")

includes("all", "snake", "automotive")

-- A simple demo using only devices on the Sonata board
firmware("sonata_simple_demo")
    add_deps("freestanding", "led_walk_raw", "echo", "lcd_test")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "echo",
                priority = 1,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "lcd_test",
                priority = 2,
                entry_point = "lcd_test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

-- A demo that expects additional devices such as I2C devices
firmware("sonata_demo_everything")
    add_deps("freestanding", "led_walk_raw", "echo", "lcd_test", "i2c_example")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "echo",
                priority = 1,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "lcd_test",
                priority = 2,
                entry_point = "lcd_test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            },
            {
                compartment = "i2c_example",
                priority = 2,
                entry_point = "run",
                stack_size = 0x300,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

-- Demo that does proximity test as well as LCD screen, etc for demos.
firmware("sonata_proximity_demo")
    add_deps("freestanding", "led_walk_raw", "echo", "lcd_test", "proximity_sensor_example")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "echo",
                priority = 1,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "lcd_test",
                priority = 2,
                entry_point = "lcd_test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            },
            {
                compartment = "proximity_sensor_example",
                priority = 2,
                entry_point = "run",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

firmware("proximity_test")
    add_deps("freestanding", "proximity_sensor_example")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "proximity_sensor_example",
                priority = 2,
                entry_point = "run",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)


-- Demo that uses a Sense HAT's LED Matrix
firmware("sense_hat_leds")
    add_deps("freestanding", "led_walk_raw", "lcd_test", "rgbled_lerp", "sense_hat_demo")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "lcd_test",
                priority = 2,
                entry_point = "lcd_test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            },
            {
                compartment = "rgbled_lerp",
                priority = 2,
                entry_point = "lerp_rgbleds",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "sense_hat_demo",
                priority = 2,
                entry_point = "test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

-- Demo that does proximity test as well as LCD screen, etc for demos.
firmware("leds_and_lcd")
    add_deps("freestanding", "led_walk_raw", "lcd_test", "rgbled_lerp")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
            {
                compartment = "lcd_test",
                priority = 2,
                entry_point = "lcd_test",
                stack_size = 0x1000,
                trusted_stack_frames = 1
            },
            {
                compartment = "rgbled_lerp",
                priority = 2,
                entry_point = "lerp_rgbleds",
                stack_size = 0x200,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
