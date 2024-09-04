-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

compartment("automotive_receive")
    add_deps("lcd", "debug")
    add_files("../../third_party/automotive_demo/automotive_common.c")
    add_files("receive.cc")

-- Automotive Demo: Receive Firmware (2nd board)
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
