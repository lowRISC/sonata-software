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

-- Part 2
compartment("disable_interrupts")
    add_files("part_2/disable_interrupts.cc")

compartment("bad_disable_interrupts")
    add_files("part_2/bad_disable_interrupts.cc")

firmware("firmware_auditing_part_2")
    add_deps("freestanding", "disable_interrupts", "bad_disable_interrupts")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "disable_interrupts",
                priority = 1,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 1
            }, {
                compartment = "bad_disable_interrupts",
                priority = 2,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 1
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)

-- Part 3
compartment("malloc1024")
    add_files("part_3/malloc1024.cc")

compartment("malloc2048")
    add_files("part_3/malloc2048.cc")

compartment("malloc4096")
    add_files("part_3/malloc4096.cc")

compartment("malloc_many")
    add_files("part_3/malloc_many.cc")

firmware("firmware_auditing_part_3")
    add_deps("freestanding", "malloc1024", "malloc2048", "malloc4096", "malloc_many")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "malloc1024",
                priority = 4,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 2
            }, {
                compartment = "malloc2048",
                priority = 3,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 2
            }, {
                compartment = "malloc4096",
                priority = 2,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 2
            }, {
                compartment = "malloc_many",
                priority = 1,
                entry_point = "entry_point",
                stack_size = 0x200,
                trusted_stack_frames = 2
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
