-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

-- Compartments used for the automotive demo firmware
compartment("heartbleed")
    add_deps("lcd", "debug")
    add_files("heartbleed.cc", "../common.c")

-- CHERIoT version of Heartbleed Demo Firmware
firmware("heartbleed_cheriot")
    add_deps("freestanding", "heartbleed")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "heartbleed",
                priority = 2,
                entry_point = "entry",
                stack_size = 0x800,
                trusted_stack_frames = 2
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
