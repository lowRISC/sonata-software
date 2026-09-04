// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// Shared header for the Juliet CWE CHERIoT RTOS compartment tests.
//
// Each Juliet CWE inner compartment intentionally triggers a CHERI hardware
// exception and has a compartment_error_handler that returns ForceUnwind.
// The orchestrator calls each inner compartment and checks that the return
// capability is invalid (is_valid() == false, address() == -1), confirming
// that the exception fired and the compartment unwound correctly.

#pragma once

#include <cdefs.h>
#include <compartment.h>

// Forward declarations for each CWE inner compartment.
// The __cheri_compartment annotation causes cross-compartment call sequences
// to be generated at every call site; the callee itself is a plain function.

/// CWE-121: Stack Buffer Overflow — writes beyond a stack-allocated array.
__cheri_compartment("cwe121_inner") void *test_cwe121_bof(int);

/// CWE-122: Heap Buffer Overflow — writes beyond a heap-allocated buffer.
__cheri_compartment("cwe122_inner") void *test_cwe122_heap_bof(int);

/// CWE-416: Use After Free — dereferences a freed pointer after revocation.
__cheri_compartment("cwe416_inner") void *test_cwe416_uaf(int);

/// CWE-415: Double Free — frees a pointer twice then accesses the freed memory.
__cheri_compartment("cwe415_inner") void *test_cwe415_double_free(int);

/// CWE-190: Integer Overflow → OOB write via a 32-bit wrap-around index.
__cheri_compartment("cwe190_inner") void *test_cwe190_int_overflow(int);
