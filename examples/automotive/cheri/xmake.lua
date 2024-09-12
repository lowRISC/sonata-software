-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

-- Compartments used for the automotive demo firmware
compartment("automotive_send")
    add_deps("lcd", "debug")
    add_files(
        "../lib/automotive_common.c", 
        "../lib/automotive_menu.c", 
        "../lib/no_pedal.c", 
        "../lib/joystick_pedal.c", 
        "../lib/digital_pedal.c", 
        "../lib/analogue_pedal.c"
    )
    add_files("send.cc")

compartment("automotive_receive")
    add_deps("lcd", "debug")
    add_files("../lib/automotive_common.c")
    add_files("receive.cc")

-- Automotive demo: Sending Firmware (1st board) (CHERIoT version)
firmware("automotive_demo_send_cheriot")
    add_deps("freestanding", "automotive_send")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "automotive_send",
                priority = 2,
                entry_point = "entry",
                stack_size = 0x1000,
                trusted_stack_frames = 3
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

-- Automotive Demo: Receiving Firmware (2nd board)
firmware("automotive_demo_receive")
    add_deps("freestanding", "automotive_receive")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "automotive_receive",
                priority = 2,
                entry_point = "entry",
                stack_size = 0x1000,
                trusted_stack_frames = 5
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
