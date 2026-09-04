// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// Forward declarations for all CHERI-C test compartments.
//
// Normal tests return int: 0 = PASS, >0 = number of assertion failures.
// Fault tests return void*: expected to ForceUnwind (return is an untagged
// capability with address() == -1), confirming a CHERI exception fired.

#pragma once

#include <cdefs.h>
#include <compartment.h>

// ── Normal tests (DECLARE_TEST) ─────────────────────────────────────────────

/// Global pointer initialisation and function pointer capability checks.
__cheri_compartment("cc_init")         int run_test_init(void);

/// Pointer comparison and derived capability bounds.
__cheri_compartment("cc_capcmp")       int run_test_capcmp(void);

/// C11 _Atomic types with pointer operands.
__cheri_compartment("cc_atomic")       int run_test_atomic(void);

/// Return address is a valid, executable capability.
__cheri_compartment("cc_capretaddr")   int run_test_capretaddr(void);

/// Function pointer size and callable via capability.
__cheri_compartment("cc_funptr")       int run_test_funptr(void);

/// __intcap_t initialisation, arithmetic, comparison.
__cheri_compartment("cc_intcap")       int run_test_intcap(void);

/// NULL pointer representation in purecap.
__cheri_compartment("cc_null")         int run_test_null(void);

/// Small-integer tagging in low bits of an aligned pointer.
__cheri_compartment("cc_smallint")     int run_test_smallint(void);

/// Unions containing capabilities.
__cheri_compartment("cc_union")        int run_test_union(void);

/// Union-of-structs with capability fields passed by value.
__cheri_compartment("cc_union_struct") int run_test_union_struct(void);

// ── Fault tests (DECLARE_TEST_FAULT) ────────────────────────────────────────
// These compartments deliberately trigger CHERI hardware exceptions.
// A correct run ForceUnwinds; the orchestrator verifies the return is invalid.

/// On-stack array accessed out-of-bounds → CHERI length fault.
__cheri_compartment("cc_array")        void *run_fault_array(void);

/// Calling a non-function-pointer → CHERI tag/execute-permission fault.
__cheri_compartment("cc_badcall")      void *run_fault_badcall(void);

/// Write through a read-only (load-only) capability → CHERI store fault.
__cheri_compartment("cc_input")        void *run_fault_input(void);

/// Read through a write-only (store-only) capability → CHERI load fault.
__cheri_compartment("cc_output")       void *run_fault_output(void);

// ── Orchestrator ─────────────────────────────────────────────────────────────
[[noreturn]] __cheri_compartment("cc_orchestrator") void run_cheri_c_tests(void);
