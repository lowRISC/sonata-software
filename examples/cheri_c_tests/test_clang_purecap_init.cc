// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_init:
//   "global pointer initialisation and function pointer capability checks"
//
// This is a DECLARE_TEST (no fault expected).  Returns 0 on PASS.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-init">;

// Pull in the original Cambridge test.  It includes cheri_c_test.h which,
// because TEST_CUSTOM_FRAMEWORK is defined, includes cheri_c_test_framework.h
// from the include path (add_includedirs in xmake.lua).
#include "../../../cheri-c-tests/clang-purecap/clang_purecap_init.c"

// Compartment entry point.  The BEGIN_TEST/END_TEST macros in the included
// .c file expand to a function named run_test_clang_purecap_init().
int __cheri_compartment("cc_init") run_test_init(void)
{
	return run_test_clang_purecap_init();
}
