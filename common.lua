-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

function convert_to_uf2(target)
    local firmware = target:targetfile()
    os.execv("llvm-strip", { firmware, "-o", firmware .. ".strip" })
    os.execv("uf2conv", { firmware .. ".strip", "-b0x00000000", "-f0x6CE29E60", "-co", firmware .. ".slot1.uf2" })
    os.execv("uf2conv", { firmware .. ".strip", "-b0x10000000", "-f0x6CE29E60", "-co", firmware .. ".slot2.uf2" })
    os.execv("uf2conv", { firmware .. ".strip", "-b0x20000000", "-f0x6CE29E60", "-co", firmware .. ".slot3.uf2" })
end
