// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// Cambridge CHERI C Test Suite — CHERIoT RTOS Orchestrator
//
// Calls each test compartment in sequence and reports pass/fail.
//
// Normal tests  (DECLARE_TEST): compartment returns int; 0 == PASS.
// Fault tests (DECLARE_TEST_FAULT): compartment ForceUnwinds; the switcher
//   returns an untagged capability with address() == -1.
//
// Output matches test_runner.py convention:
//   "All tests finished" → PASS
//   "Test(s) Failed"     → FAIL

#include "cheri_c_compartments.h"
#include <cheri.hh>
#include <compartment.h>
#include <debug.hh>
#include <stdint.h>
#include <thread.h>

using namespace CHERI;
using Debug = ConditionalDebug<true, "CHERI-C tests">;

// ── helpers ─────────────────────────────────────────────────────────────────

static bool check_normal(const char *name, int ret)
{
	if (ret == 0)
	{
		Debug::log("PASS: {}", name);
		return true;
	}
	Debug::log("FAIL: {} — {} assertion(s) failed", name, ret);
	return false;
}

static bool check_fault(const char *name, Capability<void> ret)
{
	if (!ret.is_valid() &&
	    ret.address() == static_cast<ptraddr_t>(-1UL))
	{
		Debug::log("PASS: {} — CHERI fault fired, ForceUnwind confirmed", name);
		return true;
	}
	if (ret.is_valid())
	{
		Debug::log("FAIL: {} — returned a valid capability; expected CHERI fault",
		           name);
	}
	else
	{
		Debug::log("FAIL: {} — ForceUnwind but address is {} (expected -1)",
		           name,
		           ret.address());
	}
	return false;
}

// ── thread entry point ───────────────────────────────────────────────────────

[[noreturn]] void __cheri_compartment("cc_orchestrator") run_cheri_c_tests()
{
	bool allPassed = true;
	int  passed    = 0;
	int  total     = 0;

	Debug::log("=== Cambridge CHERI-C Test Suite (CHERIoT RTOS) ===");

	// ── Normal tests ────────────────────────────────────────────────────────

	Debug::log("-- clang_purecap_init: global pointer initialisation --");
	{
		bool ok = check_normal("clang_purecap_init", run_test_init());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_capcmp: pointer comparison --");
	{
		bool ok = check_normal("clang_purecap_capcmp", run_test_capcmp());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_atomic: C11 _Atomic types --");
	{
		bool ok = check_normal("clang_purecap_atomic", run_test_atomic());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_capretaddr: return address is a capability --");
	{
		bool ok = check_normal("clang_purecap_capretaddr", run_test_capretaddr());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_funptr: function pointers --");
	{
		bool ok = check_normal("clang_purecap_funptr", run_test_funptr());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_intcap: __intcap_t arithmetic --");
	{
		bool ok = check_normal("clang_purecap_intcap", run_test_intcap());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_null: NULL pointer representation --");
	{
		bool ok = check_normal("clang_purecap_null", run_test_null());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_smallint: small-integer in low pointer bits --");
	{
		bool ok = check_normal("clang_purecap_smallint", run_test_smallint());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_union: capability tag cleared by integer write --");
	{
		bool ok = check_normal("clang_purecap_union", run_test_union());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_union_struct: union-of-structs with caps --");
	{
		bool ok = check_normal("clang_purecap_union_struct", run_test_union_struct());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	// ── Fault tests ─────────────────────────────────────────────────────────

	Debug::log("-- clang_purecap_array: OOB array → CHERI length fault --");
	{
		bool ok = check_fault("clang_purecap_array", run_fault_array());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_badcall: non-function call → CHERI exec fault --");
	{
		bool ok = check_fault("clang_purecap_badcall", run_fault_badcall());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_input: write through read-only cap → store fault --");
	{
		bool ok = check_fault("clang_purecap_input", run_fault_input());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	Debug::log("-- clang_purecap_output: read through write-only cap → load fault --");
	{
		bool ok = check_fault("clang_purecap_output", run_fault_output());
		allPassed &= ok;
		if (ok) passed++;
		total++;
	}

	// ── Summary ─────────────────────────────────────────────────────────────

	Debug::log("=== Summary: {} / {} tests passed ===", passed, total);
	if (allPassed)
	{
		Debug::log("All tests finished");
	}
	else
	{
		Debug::log("Test(s) Failed");
	}

	// CHERIoT RTOS threads must not return.
	while (true)
	{
		Timeout t{100};
		thread_sleep(&t);
	}
}
