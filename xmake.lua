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

compartment("led_walk")
    add_deps("freestanding", "debug")
    add_files("compartments/led_walk.cc")

compartment("echo")
    add_deps("freestanding")
    add_files("compartments/echo.cc")

-- Firmware image for the example.
firmware("sonata_simple_demo")
    add_deps("led_walk", "echo")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk",
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
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
