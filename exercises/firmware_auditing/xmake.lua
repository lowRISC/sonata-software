-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

-- Part 1
compartment("sealed_capability")
    add_files("part_1/sealed_capability.cc")

firmware("firmware_auditing_part_1")
    add_deps("freestanding", "sealed_capability")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "sealed_capability",
                priority = 2,
                entry_point = "do_things",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
