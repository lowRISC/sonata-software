-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0
--
-- Juliet CWE CHERIoT RTOS compartment-based enforcement tests.
--
-- Each CWE is a separate inner compartment that deliberately triggers a CHERI
-- hardware exception and has a compartment_error_handler returning ForceUnwind.
-- The juliet_orchestrator compartment calls each one and verifies the return
-- capability is invalid (ForceUnwind path), then prints "All tests finished"
-- or "Test(s) Failed" for the test_runner.py parser.

-- ── Inner compartments ───────────────────────────────────────────────────────

-- CWE-121: Stack Buffer Overflow
compartment("cwe121_inner")
    add_files("cwe121_bof.cc")

-- CWE-122: Heap Buffer Overflow
compartment("cwe122_inner")
    add_files("cwe122_heap_bof.cc")

-- CWE-416: Use After Free
compartment("cwe416_inner")
    add_files("cwe416_uaf.cc")

-- CWE-415: Double Free (use-after-double-free)
compartment("cwe415_inner")
    add_files("cwe415_double_free.cc")

-- CWE-190: Integer Overflow → Out-of-Bounds Write
compartment("cwe190_inner")
    add_files("cwe190_int_overflow.cc")

-- ── Orchestrator compartment ─────────────────────────────────────────────────

compartment("juliet_orchestrator")
    add_files("juliet_orchestrator.cc")

-- ── Firmware image ───────────────────────────────────────────────────────────

firmware("juliet_cwe_tests")
    add_deps("freestanding",
             "cwe121_inner",
             "cwe122_inner",
             "cwe416_inner",
             "cwe415_inner",
             "cwe190_inner",
             "juliet_orchestrator")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment      = "juliet_orchestrator",
                priority         = 1,
                entry_point      = "run_juliet_tests",
                stack_size       = 0x1000,
                trusted_stack_frames = 8
            }
        }, {expand = false})
    end)
    after_link(convert_to_uf2)
