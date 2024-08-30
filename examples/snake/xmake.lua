-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

compartment("snake") 
  add_deps("lcd", "debug")
  add_files("snake.cc")

firmware("snake_demo")
    add_deps("freestanding", "snake")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "snake",
                priority = 2,
                entry_point = "snake",
                stack_size = 0x1000,
                trusted_stack_frames = 2
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
