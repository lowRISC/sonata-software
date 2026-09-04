// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_capretaddr:
//   "return address is a valid, executable capability"
//
// DECLARE_TEST — returns 0 on PASS.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-capretaddr">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_capretaddr.c"

int __cheri_compartment("cc_capretaddr") run_test_capretaddr(void)
{
	return run_test_clang_purecap_capretaddr();
}
