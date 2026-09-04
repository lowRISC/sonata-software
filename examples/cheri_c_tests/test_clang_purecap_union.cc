// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_union:
//   "unions containing capabilities: tag cleared by integer word write"
//
// DECLARE_TEST — returns 0 on PASS.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-union">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_union.c"

int __cheri_compartment("cc_union") run_test_union(void)
{
	return run_test_clang_purecap_union();
}
