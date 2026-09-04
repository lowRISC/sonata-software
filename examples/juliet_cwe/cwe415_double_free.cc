// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CWE-415: Double Free
//
// This compartment allocates heap memory, frees it, attempts a second free,
// and then accesses the freed memory.  The CHERIoT allocator safely rejects
// the second free (returns -EPERM without panicking), and the subsequent
// memory access is caught by the CHERI load barrier / tag check.
//
// CHERIoT protection layers:
//   1. Software (allocator): The second heap_free call on `p` (reloaded from
//      volatile — tag already cleared by the load barrier after the first free)
//      returns -EPERM.  The allocator detects the double-free attempt without
//      faulting.
//   2. Hardware (CHERI): The subsequent dereference of `p` via volatile reload
//      goes through the load barrier.  The tag is cleared (chunk is freed),
//      so the store raises a CHERI tag exception.
//
// The compartment_error_handler returns ForceUnwind, so the orchestrator
// sees an invalid return capability.

#include "juliet_cwe.h"
#include <compartment.h>
#include <debug.hh>
#include <stdlib.h>
#include <thread.h>

using Debug = ConditionalDebug<true, "CWE-415 Double-Free">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	Debug::log(
	  "CHERI exception caught: mcause={} mtval={} pcc={}", mcause, mtval, frame->pcc);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

void *test_cwe415_double_free(int)
{
	Timeout t{5};
	// volatile forces every access to `p` to reload from memory (load barrier).
	void *volatile p = heap_allocate(&t, MALLOC_CAPABILITY, 16);
	if (p == nullptr)
	{
		Debug::log("heap_allocate returned nullptr — allocation failed");
		return nullptr;
	}
	// First free: quarantines the chunk, updates the TBRE bitmap.
	heap_free(MALLOC_CAPABILITY, p);
	// Second free: `p` is reloaded from volatile memory — the load barrier
	// clears its tag since the chunk is now in quarantine.  heap_free receives
	// an untagged capability and returns -EPERM (allocator-level detection).
	heap_free(MALLOC_CAPABILITY, p);
	// Demonstrate that hardware also prevents any subsequent use: accessing
	// the freed memory through the volatile-reloaded (tag-cleared) pointer
	// raises a CHERI tag exception.
	*(volatile char *)p = 0; // tag cleared → CHERI tag exception → ForceUnwind
	return nullptr;          // unreachable
}
