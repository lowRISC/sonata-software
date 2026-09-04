// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_funptr:
//   "function pointers: size and callable via capability"
//
// DECLARE_TEST — returns 0 on PASS.
//
// Uses strcmp from <string.h> which is available in the CHERIoT environment.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-funptr">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_funptr.c"

int __cheri_compartment("cc_funptr") run_test_funptr(void)
{
	return run_test_clang_purecap_funptr();
}
