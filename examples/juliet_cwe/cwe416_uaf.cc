// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CWE-416: Use After Free
//
// This compartment allocates heap memory, frees it, and then dereferences the
// stale pointer.  CHERIoT's load barrier clears the capability tag when the
// freed pointer is reloaded from memory (volatile), so the subsequent store
// through the now-untagged capability raises a CHERI tag exception.
//
// CHERIoT protection mechanism:
//   1. heap_free marks the chunk as quarantined and updates the TBRE bitmap.
//   2. The hardware load barrier fires on the next volatile load of `p`,
//      clears its tag (because the bitmap shows the chunk is freed), and
//      returns an untagged capability.
//   3. Storing through an untagged capability raises a CHERI tag exception.
//
// Reference: allocator-test.cc test_preflight() demonstrates this same pattern
// and confirms the load barrier operates immediately after heap_free.

#include "juliet_cwe.h"
#include <compartment.h>
#include <debug.hh>
#include <stdlib.h>
#include <thread.h>

using Debug = ConditionalDebug<true, "CWE-416 Use-After-Free">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	Debug::log(
	  "CHERI exception caught: mcause={} mtval={} pcc={}", mcause, mtval, frame->pcc);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

void *test_cwe416_uaf(int)
{
	Timeout t{5};
	// volatile forces every read of `p` to go through the load barrier.
	void *volatile p = heap_allocate(&t, MALLOC_CAPABILITY, 16);
	if (p == nullptr)
	{
		Debug::log("heap_allocate returned nullptr — allocation failed");
		return nullptr;
	}
	heap_free(MALLOC_CAPABILITY, p); // frees the allocation; TBRE bitmap updated
	// The next read of `p` from volatile memory goes through the load barrier.
	// The load barrier checks the TBRE bitmap, sees the chunk is freed, and
	// clears the tag on the capability it returns.
	// Storing through the now-untagged capability → CHERI tag exception.
	*(volatile int *)p = 42; // tag cleared → CHERI exception
	return nullptr;          // unreachable
}
