-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Software")
sdkdir = "cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

includes(path.join(sdkdir, "lib"))

option("board")
    set_default("sonata")

function convert_to_uf2(target)
    local firmware = target:targetfile()
    local binary_file = firmware .. ".bin"
    os.execv("llvm-objcopy", {"-Obinary", firmware, binary_file })
    -- 0x00101000 is the sonata software entry address
    os.execv("uf2conv", { binary_file, "-b0x00101000", "-co", firmware .. ".uf2" })
end


compartment("led_walk_raw")
    add_deps("debug")
    add_files("compartments/led_walk_raw.cc")

compartment("echo")
    add_files("compartments/echo.cc")

compartment("lcd_test")
    add_files("third_party/display_drivers/core/lcd_base.c")
    add_files("third_party/display_drivers/core/m3x6_16pt.c")
    add_files("third_party/display_drivers/st7735/lcd_st7735.c")
    add_files("compartments/lcd_test.cc")

compartment("i2c_example")
    add_deps("debug")
    add_files("compartments/i2c_example.cc")

compartment("gpiolib")
    -- This compartment uses C++ thread-safe static initialisation and so
    -- depends on the C++ runtime.
    add_deps("cxxrt")
    add_files("compartments/gpiolib.cc")

compartment("led_walk")
    add_files("compartments/led_walk.cc")

compartment("proximity_sensor_example")
    add_deps("debug")
    add_files("compartments/proximity_sensor_example.cc")

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

-- A firmware image that only walks LEDs
firmware("sonata_led_demo")
    add_deps("freestanding", "debug")
    add_deps("led_walk", "gpiolib")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk",
                priority = 1,
                entry_point = "start_walking",
                stack_size = 0x400,
                trusted_stack_frames = 3
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
                stack_size = 0x200,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
