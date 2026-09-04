// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment wrapping clang_purecap_badcall (FAULT test):
//   "calling a non-function-pointer → CHERI tag / execute-permission fault"
//
// DECLARE_TEST_FAULT.  The test calls a function pointer derived from an
// integer (no EXECUTE permission), which immediately triggers a CHERI fault.
// In CHERIoT, compartment_error_handler returns ForceUnwind.  The
// orchestrator verifies the returned capability is invalid (address == -1).

#define TEST_CUSTOM_FRAMEWORK
#include <compartment.h>
#include <debug.hh>

using Debug = ConditionalDebug<true, "cc-badcall">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	(void)frame;
	(void)mcause;
	(void)mtval;
	Debug::log("CHERI fault caught (mcause=0x{:x}): ForceUnwind", mcause);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

#include "../../../cheri-c-tests/clang-purecap/clang_purecap_badcall.c"

void *__cheri_compartment("cc_badcall") run_fault_badcall(void)
{
	run_test_clang_purecap_badcall();
	Debug::log("WARNING: no CHERI fault triggered in clang_purecap_badcall");
	return nullptr;
}
