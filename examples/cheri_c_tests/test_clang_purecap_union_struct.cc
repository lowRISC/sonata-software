// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_union_struct:
//   "union-of-structs with capability fields passed by value"
//
// DECLARE_TEST — returns 0 on PASS.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-union-struct">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_union_struct.c"

int __cheri_compartment("cc_union_struct") run_test_union_struct(void)
{
	return run_test_clang_purecap_union_struct();
}
