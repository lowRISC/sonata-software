-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

compartment("flying_game")
  add_deps("lcd", "debug")
  add_files("flying_game.cc")

firmware("flying_game_demo")
    add_deps("freestanding", "flying_game")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "flying_game",
                priority = 2,
                entry_point = "flying_game",
                stack_size = 0x1000,
                trusted_stack_frames = 2
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
