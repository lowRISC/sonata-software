-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0
--
-- Cambridge CHERI C Test Suite for CHERIoT RTOS
--
-- Each Cambridge purecap test is wrapped in its own compartment.  Normal tests
-- (DECLARE_TEST) return int: 0 = PASS.  Fault tests (DECLARE_TEST_FAULT) are
-- expected to ForceUnwind; the orchestrator verifies the return capability is
-- invalid (address == -1).
--
-- Include paths required by every compartment that includes an original .c:
--   "."                    → cheri_c_test_framework.h (angle-bracket include)
--   "../../../cheri-c-tests" → cheri_c_test.h (quoted include from test files)

-- ── Helper: add include paths needed by compartments that wrap .c files ──────
local function add_cheri_c_incdirs()
    add_includedirs(".", "../../../cheri-c-tests")
end

-- ── Normal test compartments (DECLARE_TEST) ──────────────────────────────────

compartment("cc_init")
    add_files("test_clang_purecap_init.cc")
    add_cheri_c_incdirs()

compartment("cc_capcmp")
    add_files("test_clang_purecap_capcmp.cc")
    add_cheri_c_incdirs()

compartment("cc_atomic")
    add_files("test_clang_purecap_atomic.cc")
    add_cheri_c_incdirs()

compartment("cc_capretaddr")
    add_files("test_clang_purecap_capretaddr.cc")
    add_cheri_c_incdirs()

compartment("cc_funptr")
    add_files("test_clang_purecap_funptr.cc")
    add_cheri_c_incdirs()

compartment("cc_intcap")
    add_files("test_clang_purecap_intcap.cc")
    add_cheri_c_incdirs()

compartment("cc_null")
    add_files("test_clang_purecap_null.cc")
    add_cheri_c_incdirs()

compartment("cc_smallint")
    add_files("test_clang_purecap_smallint.cc")
    add_cheri_c_incdirs()

compartment("cc_union")
    add_files("test_clang_purecap_union.cc")
    add_cheri_c_incdirs()

compartment("cc_union_struct")
    add_files("test_clang_purecap_union_struct.cc")
    add_cheri_c_incdirs()

-- ── Fault test compartments (DECLARE_TEST_FAULT) ─────────────────────────────

compartment("cc_array")
    add_files("test_clang_purecap_array.cc")
    add_cheri_c_incdirs()

compartment("cc_badcall")
    add_files("test_clang_purecap_badcall.cc")
    add_cheri_c_incdirs()

-- input/output use standalone .cc (no original .c include — FreeBSD qualifiers)
compartment("cc_input")
    add_files("test_clang_purecap_input.cc")

compartment("cc_output")
    add_files("test_clang_purecap_output.cc")

-- ── Orchestrator compartment ──────────────────────────────────────────────────

compartment("cc_orchestrator")
    add_files("cheri_c_orchestrator.cc")

-- ── Firmware image ───────────────────────────────────────────────────────────

firmware("cheri_c_tests")
    add_deps("freestanding",
             "cc_init",
             "cc_capcmp",
             "cc_atomic",
             "cc_capretaddr",
             "cc_funptr",
             "cc_intcap",
             "cc_null",
             "cc_smallint",
             "cc_union",
             "cc_union_struct",
             "cc_array",
             "cc_badcall",
             "cc_input",
             "cc_output",
             "cc_orchestrator")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment          = "cc_orchestrator",
                priority             = 1,
                entry_point          = "run_cheri_c_tests",
                stack_size           = 0x1000,
                trusted_stack_frames = 20
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
