-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

function convert_to_uf2(target)
    local firmware = target:targetfile()
    os.execv("llvm-strip", { firmware, "-o", firmware .. ".strip" })
    os.execv("uf2conv", { firmware .. ".strip", "-f0x6CE29E60", "-co", firmware .. ".uf2" })
end
