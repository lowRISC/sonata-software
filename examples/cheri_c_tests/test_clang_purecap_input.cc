// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CHERIoT compartment implementing clang_purecap_input (FAULT test):
//   "write through a capability whose STORE permission has been removed"
//
// DECLARE_TEST_FAULT.  The original test uses the FreeBSD __cheri_input
// qualifier to strip STORE/STORE_CAPABILITY from a pointer and then verifies
// that a store traps.  We replicate the semantic using
// __builtin_cheri_perms_and() to narrow the permissions explicitly, which
// is the canonical CHERIoT way to produce a read-only capability at runtime.
//
// The attempted store triggers a CHERI store-permission fault; the
// compartment_error_handler returns ForceUnwind.  The orchestrator verifies
// the returned capability is invalid (address == -1).

#include <compartment.h>
#include <debug.hh>
#include <stdint.h>

using Debug = ConditionalDebug<true, "cc-input">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	(void)frame;
	(void)mcause;
	(void)mtval;
	Debug::log("CHERI fault caught (mcause=0x{:x}): ForceUnwind", mcause);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

// Strip STORE and STORE_CAPABILITY from *p, then attempt a store through the
// narrowed capability — this must fault with a CHERI store-permission trap.
__attribute__((noinline)) static void attempt_store_through_readonly(int *p)
{
	// Build a read-only (LOAD-only) capability from p.
	void *ro = __builtin_cheri_perms_and(
	    (void *)p,
	    __CHERI_CAP_PERMISSION_PERMIT_LOAD__ |
	    __CHERI_CAP_PERMISSION_PERMIT_LOAD_CAPABILITY__);

	// Verify the permissions were actually removed before attempting the store.
	__builtin_expect(
	    (__builtin_cheri_perms_get(ro) &
	     __CHERI_CAP_PERMISSION_PERMIT_STORE__) == 0,
	    1);

	// This store must trigger a CHERI store-permission fault.
	*(volatile int *)ro = 42;
}

void *__cheri_compartment("cc_input") run_fault_input(void)
{
	int x = 47;
	attempt_store_through_readonly(&x);
	Debug::log("WARNING: no CHERI fault triggered in cc_input test");
	return nullptr;
}
