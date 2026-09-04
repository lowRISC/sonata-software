// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT RTOS adaptation of the Cambridge CHERI C test framework.
//
// This header is included by cheri_c_test.h when TEST_CUSTOM_FRAMEWORK is
// defined.  It replaces the FreeBSD-specific test machinery (signal handlers,
// test_setup(), assert(3)) with CHERIoT-compatible equivalents:
//
//  - assert / assert_eq / assert_eq_cap  →  Debug::log + test_failures counter
//  - BEGIN_TEST / END_TEST               →  compartment-local int run_test_NAME()
//  - DECLARE_TEST / DECLARE_TEST_FAULT   →  no-ops
//  - faults / test_fault_handler         →  defined here (extern decls in
//                                           cheri_c_test.h are satisfied)
//
// Include path requirements (set via xmake add_includedirs):
//   - the cheri_c_tests/ directory (so <cheri_c_test_framework.h> resolves here)
//   - the cheri-c-tests/  directory  (so "cheri_c_test.h" resolves there)
//
// Debug must be an alias for ConditionalDebug<...> defined by the including
// .cc file *before* the original .c test file is included.

#pragma once

// ── Standard types used by the tests ────────────────────────────────────────
#include <stddef.h>
#include <stdint.h>

// BSD compat: __uint64_t used in clang_purecap_intcap.c
#ifndef __uint64_t
typedef uint64_t __uint64_t;
#endif

// BSD compat: __unused attribute
#ifndef __unused
#define __unused __attribute__((unused))
#endif

// ── Fault-tracking state (defined here, extern-declared in cheri_c_test.h) ──
// Each compartment gets its own copy (one TU per compartment).
// For DECLARE_TEST_FAULT compartments the compartment_error_handler
// increments faults and returns ForceUnwind; the orchestrator then checks
// that the compartment call resulted in a ForceUnwind (address == -1).
volatile int faults = 0;

// cheri_handler is typedef'd in cheri_c_test.h before the #ifdef block.
// We define the variable here to satisfy the extern declaration.
// cheri_handler test_fault_handler = nullptr;  -- defined after the typedef below.
// (We use a forward declaration pattern; the actual variable is at end of header.)

// ── Per-test failure counter ─────────────────────────────────────────────────
// Reset to 0 at the start of each BEGIN_TEST block.
static int test_failures = 0;

// ── assert / assert_eq / assert_eq_cap ──────────────────────────────────────
// Replace stdlib assert with Debug::log + counter so tests continue after
// a failed assertion (rather than aborting).  Debug must be defined by the
// including .cc file before the .c test file is pulled in.

#undef assert
#define assert(x)  do { \
    if (!(x)) { \
        Debug::log("FAIL: assertion `" #x "` failed at line {}", __LINE__); \
        test_failures++; \
    } \
} while(0)

#define assert_eq(a, b) do { \
    auto _a = (long long)(a); \
    auto _b = (long long)(b); \
    if (_a != _b) { \
        Debug::log("FAIL: assert_eq failed at line {}: {} != {}", \
                   __LINE__, _a, _b); \
        test_failures++; \
    } \
} while(0)

#define assert_eq_cap(a, b) do { \
    auto _a = (a); \
    auto _b = (b); \
    if (_a != _b) { \
        Debug::log("FAIL: assert_eq_cap failed at line {}", __LINE__); \
        test_failures++; \
    } \
} while(0)

// ── DECLARE_TEST / DECLARE_TEST_FAULT → no-ops ──────────────────────────────
// cheri_c_testdecls.h (included at end of cheri_c_test.h) expands these.
// We don't need them to do anything.
#define DECLARE_TEST(name, desc)       /* no-op */
#define DECLARE_TEST_FAULT(name, desc) /* no-op */

// ── BEGIN_TEST / END_TEST ────────────────────────────────────────────────────
// Each test .c file contains exactly one BEGIN_TEST(name) … END_TEST block.
// The expansion produces:
//   int run_test_NAME() { test_failures = 0; <body> return test_failures; }
#define BEGIN_TEST(name) \
    int run_test_##name(void) { \
        test_failures = 0;

#define END_TEST \
        return test_failures; \
    }

// ── test_fault_handler definition ───────────────────────────────────────────
// cheri_c_test.h typedef's cheri_handler before the #ifdef block, so by the
// time this header is processed the type exists.  We define the variable
// here; the extern declaration in cheri_c_test.h refers to this definition.
// Using a C-linkage initialiser is fine in a C++ TU compiled from .cc.
cheri_handler test_fault_handler = nullptr;
