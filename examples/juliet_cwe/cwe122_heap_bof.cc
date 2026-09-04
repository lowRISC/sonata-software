// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CWE-122: Heap Buffer Overflow
//
// This compartment allocates 64 bytes on the heap and then writes at byte
// offset 128 — past the end of the allocation.  CHERIoT sets precise bounds on
// every heap allocation, so the out-of-bounds write raises a CHERI bounds
// exception.  The compartment_error_handler returns ForceUnwind.
//
// CHERIoT protection: heap_allocate returns a capability bounded to exactly the
// requested size (64 bytes).  Accessing c[128] is outside those bounds →
// CHERI bounds exception.

#include "juliet_cwe.h"
#include <compartment.h>
#include <debug.hh>
#include <stdlib.h>
#include <thread.h>

using Debug = ConditionalDebug<true, "CWE-122 Heap BOF">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	Debug::log(
	  "CHERI exception caught: mcause={} mtval={} pcc={}", mcause, mtval, frame->pcc);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

void *test_cwe122_heap_bof(int)
{
	Timeout t{5};
	void   *p = heap_allocate(&t, MALLOC_CAPABILITY, 64);
	if (p == nullptr)
	{
		// Should not happen in normal test runs; if it does the orchestrator
		// will report a valid return and the test will be marked as FAIL.
		Debug::log("heap_allocate returned nullptr — allocation failed");
		return nullptr;
	}
	char *c = static_cast<char *>(p);
	c[128]  = 'X'; // bounds violation: allocation is 64 bytes; index 128 is OOB
	// The line below is unreachable: the CHERI exception fires above.
	heap_free(MALLOC_CAPABILITY, p);
	return nullptr;
}
