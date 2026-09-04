// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_capcmp:
//   "pointer comparison and derived capability bounds"
//
// DECLARE_TEST — returns 0 on PASS.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-capcmp">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_capcmp.c"

int __cheri_compartment("cc_capcmp") run_test_capcmp(void)
{
	return run_test_clang_purecap_capcmp();
}
