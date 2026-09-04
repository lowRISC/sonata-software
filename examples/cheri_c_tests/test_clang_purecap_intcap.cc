// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_intcap:
//   "__intcap_t initialisation, arithmetic, comparison"
//
// DECLARE_TEST — returns 0 on PASS.
//
// The test uses __uint64_t (a BSD alias for uint64_t).  Our framework header
// provides the typedef if it is not already defined.

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-intcap">;

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_intcap.c"

int __cheri_compartment("cc_intcap") run_test_intcap(void)
{
	return run_test_clang_purecap_intcap();
}
