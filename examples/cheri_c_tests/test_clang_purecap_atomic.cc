// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_atomic:
//   "C11 _Atomic types including _Atomic(int*) with pointer operands"
//
// DECLARE_TEST — returns 0 on PASS.
//
// Note: clang_purecap_atomic.c includes <stdlib.h> for NULL/size_t; both
// are available in the CHERIoT freestanding environment.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-atomic">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_atomic.c"

int __cheri_compartment("cc_atomic") run_test_atomic(void)
{
	return run_test_clang_purecap_atomic();
}
