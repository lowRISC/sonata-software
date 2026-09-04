// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// Juliet CWE Test Orchestrator
//
// This is the single thread entry point for the juliet_cwe_tests firmware.
// It calls each Juliet CWE inner compartment in sequence and verifies that
// each one triggered a CHERI hardware exception and was correctly unwound by
// the CHERIoT RTOS switcher (ForceUnwind path).
//
// A correctly unwound compartment call returns an invalid capability with
// address() == -1.  Any other return (valid capability, or address != -1)
// indicates the expected CHERI exception did not fire.
//
// Output matches the test_runner.py convention:
//   "All tests finished" → PASS
//   "Test(s) Failed"     → FAIL

#include "juliet_cwe.h"
#include <cheri.hh>
#include <compartment.h>
#include <debug.hh>
#include <stdint.h>
#include <thread.h>

using namespace CHERI;
using Debug = ConditionalDebug<true, "Juliet CWE tests">;

/**
 * Check that a call to an inner compartment resulted in ForceUnwind.
 *
 * On a successful ForceUnwind, the CHERIoT switcher returns an untagged
 * capability whose address field is set to -1.  Anything else means the
 * CHERI exception did not fire (the vulnerability was not caught).
 *
 * @param name   Short CWE label for diagnostic output.
 * @param ret    Return value from the inner compartment call.
 * @returns true if the compartment faulted and unwound as expected.
 */
static bool
check_compartment_faulted(const char *name, Capability<void> ret)
{
	if (ret.is_valid())
	{
		Debug::log(
		  "FAIL: {} returned a valid capability {} — CHERI exception did not fire",
		  name,
		  ret);
		return false;
	}
	if (ret.address() != static_cast<ptraddr_t>(-1UL))
	{
		Debug::log(
		  "FAIL: {} unwound but return address is {} (expected -1)", name, ret.address());
		return false;
	}
	Debug::log("PASS: {} raised CHERI exception, switcher performed ForceUnwind", name);
	return true;
}

/**
 * Run all five Juliet CWE tests and report pass/fail via UART.
 *
 * This is the sole thread entry point for the juliet_cwe_tests firmware.
 * CHERIoT RTOS threads must not return; the function loops forever after
 * printing the final result.
 */
[[noreturn]] void __cheri_compartment("juliet_orchestrator") run_juliet_tests()
{
	bool allPassed = true;

	Debug::log("=== Juliet CWE CHERIoT Enforcement Tests ===");

	// ── CWE-121: Stack Buffer Overflow ──────────────────────────────────────
	Debug::log("-- CWE-121: Stack Buffer Overflow --");
	allPassed &= check_compartment_faulted("CWE-121", test_cwe121_bof(0));

	// ── CWE-122: Heap Buffer Overflow ───────────────────────────────────────
	Debug::log("-- CWE-122: Heap Buffer Overflow --");
	allPassed &= check_compartment_faulted("CWE-122", test_cwe122_heap_bof(0));

	// ── CWE-416: Use After Free ─────────────────────────────────────────────
	Debug::log("-- CWE-416: Use After Free --");
	allPassed &= check_compartment_faulted("CWE-416", test_cwe416_uaf(0));

	// ── CWE-415: Double Free ────────────────────────────────────────────────
	Debug::log("-- CWE-415: Double Free (use-after-double-free) --");
	allPassed &= check_compartment_faulted("CWE-415", test_cwe415_double_free(0));

	// ── CWE-190: Integer Overflow → OOB Write ───────────────────────────────
	Debug::log("-- CWE-190: Integer Overflow to Out-of-Bounds Write --");
	allPassed &= check_compartment_faulted("CWE-190", test_cwe190_int_overflow(0));

	// ── Summary ─────────────────────────────────────────────────────────────
	Debug::log("=== Summary: {} / 5 tests passed ===", allPassed ? 5 : 0);
	if (allPassed)
	{
		// test_runner.py matches this substring to declare success.
		Debug::log("All tests finished");
	}
	else
	{
		// test_runner.py matches this substring to declare failure.
		Debug::log("Test(s) Failed");
	}

	// CHERIoT RTOS threads must not return; spin until the simulation ends.
	while (true)
	{
		Timeout t{100};
		thread_sleep(&t);
	}
}
